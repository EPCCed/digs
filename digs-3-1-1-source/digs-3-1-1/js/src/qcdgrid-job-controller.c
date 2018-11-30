/***********************************************************************
*
*   Filename:   qcdgrid-job-controller.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Runs on the host system's front-end, submitting the
*               job executable to the batch system, monitoring it and
*               making I/O and status info available to the client
*
*   Contents:   Main function
*
*   Used in:    QCDgrid job submission
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003 The University of Edinburgh
*
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License as
*   published by the Free Software Foundation; either version 2 of the
*   License, or (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
*   MA 02111-1307, USA.
*
*   As a special exception, you may link this program with code
*   developed by the OGSA-DAI project without such code being covered
*   by the GNU General Public License.
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>

#include <globus_gss_assist.h>
#include <globus_io.h>

#include "qcdgrid-js.h"
#include "qcdgrid-client.h"
#include "jobdesc.h"
#include "misc.h"
#include "batch.h"

/*
 * File to log errors and information to. If NULL, log file has not yet
 * been opened and stderr is used instead
 */
FILE *errorLogFile_=NULL;

/***********************************************************************
*   void error(char *template, ...)
*    
*   Error reporting function. If the log file name is known (i.e. job
*   description has been parsed), log error in there. If an error occurs
*   before we reach that stage, have to just print it to stderr and hope
*   it reaches the user somehow
*    
*   Parameters:                                        [I/O]
*
*     template  format string for message, as printf    I
*     ...       optional values to put into message     I
*    
*   Returns: (void)
***********************************************************************/
void error(char *template, ...)
{
    va_list ap;
    char *buffer;
    int size;
    int sizeneeded;

    va_start(ap, template);

    size=1024;
    buffer=malloc(size);
    if (!buffer) return;

    sizeneeded=vsnprintf(buffer, size, template, ap);
    while (sizeneeded>size)
    {
	free(buffer);
	size=sizeneeded+1024;
	buffer=malloc(size);
	if (!buffer) return;
	
	sizeneeded=vsnprintf(buffer, size, template, ap);
    }
    va_end(ap);

    if (!errorLogFile_)
    {
	/* send it to stderr and hope for the best... */
	fprintf(stderr, buffer);
    }
    else
    {
	fprintf(errorLogFile_, buffer);
    }
    free(buffer);
}

/*
 * Number of bytes in the stdout file last time we read it
 */
static int stdoutLastLength_=0;

/*
 * Number of bytes in the stderr file last time we read it
 */
static int stderrLastLength_=0;

static int streamBytesAvailable(int isStderr)
{
    FILE *stream;
    int newlen;

    if (isStderr)
    {
	stream=fopen("stderr", "rb");
    }
    else
    {
	stream=fopen("stdout", "rb");
    }

    if (!stream)
    {
	newlen=0;
    }
    else
    {
	fseek(stream, 0, SEEK_END);
	newlen=ftell(stream);
	fclose(stream);
    }

    if (isStderr)
    {
	newlen-=stderrLastLength_;
    }
    else
    {
	newlen-=stdoutLastLength_;
    }
    if (newlen<0)
    {
	newlen=0;
    }

    return newlen;
}

static int readStream(int isStderr, unsigned char *buffer, int len)
{
    FILE *stream;
    int newlen;

    if (isStderr)
    {
	stream=fopen("stderr", "rb");
    }
    else
    {
	stream=fopen("stdout", "rb");
    }

    if (!stream)
    {
	return 0;
    }

    if (isStderr)
    {
	fseek(stream, stderrLastLength_, SEEK_SET);
	stderrLastLength_+=len;
    }
    else
    {
	fseek(stream, stdoutLastLength_, SEEK_SET);
	stdoutLastLength_+=len;
    }
    
    fread(buffer, len, 1, stream);
    fclose(stream);
    return 1;
}

/*
 * The user's certificate subject. Used to make sure that only the
 * submitter can connect and get job status and output
 */
static char *userIdentity_;

/***********************************************************************
*   int getCredentials()
*    
*   Obtains the current user's certificate subject and stores it in
*   the global userIdentity_ variable for later
*    
*   Parameters:                                             [I/O]
*
*     None
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
static int getCredentials()
{
    gss_cred_id_t credentialHandle = GSS_C_NO_CREDENTIAL;
    OM_uint32 majorStatus;
    OM_uint32 minorStatus;
    gss_name_t credentialName;
    OM_uint32 lifetime;
    gss_cred_usage_t usage;
    gss_OID_set mechanisms;
    gss_buffer_desc buffer;

    majorStatus = globus_gss_assist_acquire_cred(&minorStatus,
						 GSS_C_INITIATE, /* or GSS_C_ACCEPT */
						 &credentialHandle);
    
    if (majorStatus != GSS_S_COMPLETE)
    {
	return 0;
    }

    majorStatus=gss_inquire_cred(&minorStatus, credentialHandle,
				 &credentialName, &lifetime,
				 &usage, &mechanisms);
    if (majorStatus!=GSS_S_COMPLETE)
    {
	return 0;
    }

    majorStatus=gss_export_name(&minorStatus, credentialName,
				&buffer);
    if (majorStatus!=GSS_S_COMPLETE)
    {
	return 0;
    }

    userIdentity_=(char*)buffer.value;
    printf("Name: %s\n", userIdentity_);

    return 1;
}

/*
 * Directory where all files related to this job are stored
 */
char *jobDirectory_;

/*
 * The socket on which we listen for incoming connections from the job
 */
static globus_io_handle_t sock_;

/*
 * The port number on which we are listening for connections from the job
 */
static unsigned short listenPort_;

/*
 * Set if the client is connected to the socket
 */
static volatile int clientConnected_=0;

static volatile int fetchingInput_=0;
static volatile int storingOutput_=0;

/***********************************************************************
*   globus_bool_t jsAuthCallback(void *arg, globus_io_handle_t *handle,
*                                globus_result_t result, char *identity, 
*                                gss_ctx_id_t *context_handle)
*    
*   Authorisation callback used to establish whether the sender of the
*   message is the same user who submitted the job
*    
*   Parameters:                                             [I/O]
*
*     identity  pointer to the sender's certificate subject  I
*
*   Returns: GLOBUS_TRUE
************************************************************************/
static globus_bool_t jsAuthCallback(void *arg, globus_io_handle_t *handle,
				    globus_result_t result, char *identity, 
				    gss_ctx_id_t *context_handle)
{
    /* check it's the same user who submitted the job */
    if (!strcmp(identity, userIdentity_))
    {
	error("Authorising connection for %s\n", identity);
	return GLOBUS_TRUE;
    }
    error("Rejecting connection for %s\n", identity);
    return GLOBUS_FALSE;
}

/***********************************************************************
*   void jsListenCallback(void *arg, globus_io_handle_t *handle,
*                         globus_result_t result)
*    
*   Called by Globus when an incoming connection arrives on our
*   socket. Processes the data received from the job.
*    
*   Parameters:                                             [I/O]
*
*     Not used
*
*   Returns: (void)
************************************************************************/
static void jsListenCallback(void *arg, globus_io_handle_t *handle,
			     globus_result_t result)
{
    globus_io_handle_t sock2;
    globus_result_t errorCode;
    unsigned char msgType;
    int n, o, len;
    static unsigned char buffer[1000];
    FILE *f;

    /*
     * Re-register listen callback to make sure other connections are
     * dealt with
     */
    /* FIXME: check if this is actually necessary */
    if (globus_io_tcp_register_listen(&sock_, jsListenCallback, NULL)!=GLOBUS_SUCCESS)
    {
	error("Error listening on Globus socket\n");
    }

    errorCode=globus_io_tcp_accept(&sock_, GLOBUS_NULL, &sock2);
    if (errorCode==GLOBUS_IO_ERROR_TYPE_AUTHENTICATION_FAILED)
    {
	error("Authentication failed\n");
	return;
    }
	
    if (errorCode==GLOBUS_IO_ERROR_TYPE_SYSTEM_FAILURE)
    {
	error("System error\n");
    }
    if (errorCode!=GLOBUS_SUCCESS)
    {
	error("Error accepting connection on Globus socket\n");
	return;
    }

    clientConnected_++;

    errorCode=globus_io_read(&sock2, &msgType, 1, 1, &o);
    while ((errorCode==GLOBUS_SUCCESS)&&(o>0))
    {
	switch(msgType)
	{
	case QJS_MSG_STDOUT:
	    error("Received STDOUT message...\n");
	    n=streamBytesAvailable(0);
	    error("%d bytes available, sending them\n", n);
	    buffer[0]=(n&0xff);
	    buffer[1]=(n>>8)&0xff;
	    buffer[2]=(n>>16)&0xff;
	    buffer[3]=(n>>24)&0xff;
	    globus_io_write(&sock2, &buffer[0], 4, &o);

	    while (n>0)
	    {
		if (n>=1000)
		{
		    len=1000;
		}
		else
		{
		    len=n;
		}

		if (!readStream(0, buffer, len))
		{
		    error("Error reading stdout stream\n");
		}
		globus_io_write(&sock2, &buffer[0], len, &o);
		n-=len;
	    }
	    break;

	case QJS_MSG_STDERR:
	    error("Received STDERR message...\n");
	    n=streamBytesAvailable(1);
	    error("%d bytes available, sending them\n", n);
	    buffer[0]=(n&0xff);
	    buffer[1]=(n>>8)&0xff;
	    buffer[2]=(n>>16)&0xff;
	    buffer[3]=(n>>24)&0xff;
	    globus_io_write(&sock2, &buffer[0], 4, &o);

	    while (n>0)
	    {
		if (n>=1000)
		{
		    len=1000;
		}
		else
		{
		    len=n;
		}

		if (!readStream(1, buffer, len))
		{
		    error("Error reading stderr stream\n");
		}
		globus_io_write(&sock2, &buffer[0], len, &o);
		n-=len;
	    }
	    break;

	case QJS_MSG_STDIN:
	    globus_io_read(&sock2, buffer, 4, 4, &o);
	    n=(buffer[0])|(buffer[1]<<8)|(buffer[2]<<16)|(buffer[3]<<24);
	    f=fopen("stdin", "a");
	    while (n>0)
	    {
		if (n>=1000)
		{
		    len=1000;
		}
		else
		{
		    len=n;
		}
		globus_io_read(&sock2, buffer, len, len, &o);
		fwrite(buffer, 1, len, f);
		n-=len;
	    }
	    fclose(f);
	    break;

	case QJS_MSG_STATE:
	    if (fetchingInput_)
	    {
		buffer[0]=JOB_FETCHINGINPUT;
	    }
	    else if (storingOutput_)
	    {
		buffer[0]=JOB_STORINGOUTPUT;
	    }
	    else
	    {
		buffer[0]=getJobStatus();
	    }
	    globus_io_write(&sock2, buffer, 1, &o);
	    break;

	case QJS_MSG_OUTFILES:
	    /*
	     * For each one send the length of name followed by the characters of
	     * the name. Finish by sending 0 as a length of name
	     */
	{
	    DIR *dp;
	    struct dirent *ep;
	    int skipit;
	    char *fn;

	    dp=opendir(jobDirectory_);
	    ep=readdir(dp);
	    while (ep)
	    {
		skipit=0;

		/*
		 * Skip special dirs and hidden files
		 */
		if (ep->d_name[0]=='.')
		{
		    skipit=1;
		}
		/*
		 * Skip the following:
		 *   input files
		 *   files to be stored on grid
		 *   files fetched from grid
		 *   core dump*
		 *   internal files (port, exit code, pbsdone, pbsid, pbsstatus, pbs.sh)*
		 * (* maybe have a debug mode that does fetch these ones)
		 */
		/* FIXME: skip staged executable */
		if ((!strcmp(ep->d_name, "port"))||
		    (!strcmp(ep->d_name, "exitcode"))||
		    (!strcmp(ep->d_name, "pbsdone"))||
		    (!strcmp(ep->d_name, "pbsid"))||
		    (!strcmp(ep->d_name, "pbsstatus"))||
		    (!strcmp(ep->d_name, "pbs.sh")))
		{
		    skipit=1;
		}

		if ((!strcmp(ep->d_name, "core"))||
		    (!strncmp(ep->d_name, "core.", 5)))
		{
		    skipit=1;
		}
		    
		fn=getJobDescriptionEntry("store");
		while (fn)
		{
		    if (!strcmp(ep->d_name, fn))
		    {
			skipit=1;
		    }
		    fn=getNextJobDescriptionEntry("store");
		}

		fn=getJobDescriptionEntry("fetch");
		while (fn)
		{
		    fn=getNextJobDescriptionEntry("as");
		    if (!strcmp(ep->d_name, fn))
		    {
			skipit=1;
		    }
		    fn=getNextJobDescriptionEntry("fetch");
		}

		fn=getJobDescriptionEntry("input");
		while (fn)
		{
		    if (!strcmp(ep->d_name, fn))
		    {
			skipit=1;
		    }
		    fn=getNextJobDescriptionEntry("input");
		}

		if (!skipit)
		{
		    len=strlen(ep->d_name);
		    buffer[0]=len&0xff;
		    buffer[1]=(len>>8)&0xff;
		    buffer[2]=(len>>16)&0xff;
		    buffer[3]=(len>>24)&0xff;
		    strcpy(&buffer[4], ep->d_name);
		    globus_io_write(&sock2, buffer, len+4, &o);
		}
		ep=readdir(dp);
	    }
	    buffer[0]=0; buffer[1]=0; buffer[2]=0; buffer[3]=0;
	    globus_io_write(&sock2, buffer, 4, &o);
	}
	    break;

	case QJS_MSG_KEEPALIVE:
	    /* ignore */
	    break;

	default:
	    error("Unrecognised message %d received\n", msgType);
	    break;
	}
	errorCode=globus_io_read(&sock2, &msgType, 1, 1, &o);
    }

    clientConnected_--;
    globus_io_close(&sock2);
}

/***********************************************************************
*   int jsListenForMessages()
*    
*   Opens a TCP/IP socket to listen for messages coming in from the
*   client
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: 0 on failure (never returns if successful)
************************************************************************/
static int jsListenForMessages()
{
    globus_io_attr_t attr;
    globus_result_t errorCode;
    globus_io_secure_authorization_data_t authData;
    int firstPort;
    int lastPort;
    char *globusPortRange;

    /* Default port limits */
    firstPort=16384;
    lastPort=65535;

    /* Respect GLOBUS_TCP_PORT_RANGE variable */
    globusPortRange=getenv("GLOBUS_TCP_PORT_RANGE");
    if (globusPortRange)
    {
	/*
	 * Even when the variable is '50000,52000' getenv returns
	 * '50000 52000'. Strange. I hope it's the same on other
	 * systems.
	 */
	sscanf(globusPortRange, "%d %d", &firstPort, &lastPort);

	/* but not if it's set to insane values */
	if ((firstPort>65535)||(firstPort<1024))
	{
	    firstPort=16384;
	}
	if ((lastPort>65535)||(lastPort<1024))
	{
	    lastPort=65535;
	}
    }

    printf("Port range is %d to %d\n", firstPort, lastPort);

//    finishedReadingOutput_=0;

    if (globus_io_secure_authorization_data_initialize(&authData)
	!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error initialising autorization data structure\n");
	return 0;
    }

    if (globus_io_secure_authorization_data_set_callback(&authData,
							 jsAuthCallback, NULL)
	!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting authorization callback\n");
	return 0;
    }

    if (globus_io_tcpattr_init(&attr)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error initialising TCP attribute structure\n");
	return 0;
    }

    if (globus_io_attr_set_secure_authentication_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHENTICATION_MODE_GSSAPI, 
	    GSS_C_NO_CREDENTIAL)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting authentication mode to GSSAPI\n");
	return 0;
    }

    if (globus_io_attr_set_secure_proxy_mode (&attr, 
					      GLOBUS_IO_SECURE_PROXY_MODE_MANY)
	!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting proxy mode to many\n");
	return 0;
    }

    if (globus_io_attr_set_secure_authorization_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHORIZATION_MODE_CALLBACK, 
	    &authData)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting authorization mode to callback\n");
	return 0;
    }

    if (globus_io_attr_set_secure_channel_mode(
	    &attr, GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting channel mode to GSI wrap\n");
	return 0;
    }
                                                       
    /* Increase socket buffer sizes - defaults are too small */
    errorCode=globus_io_attr_set_socket_sndbuf(&attr, 1024);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting socket send buffer size\n");
	return 0;
    }

    errorCode=globus_io_attr_set_socket_rcvbuf(&attr, 1024);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting socket receive buffer size\n");
	return 0;
    }

    errorCode=globus_io_attr_set_tcp_nodelay(&attr, GLOBUS_TRUE);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting TCP no delay mode\n");
	return 0;
    }

    /* Try to open a listener on each port in the range until we find one that works */
    listenPort_=firstPort;

    while (listenPort_<=lastPort)
    {
	if (globus_io_tcp_create_listener(&listenPort_, -1, &attr, &sock_)
	    ==GLOBUS_SUCCESS)
	{
	    /* Found a port we can use */
	    break;
	}

	/* Try the next one */
	listenPort_++;
    }

    if (listenPort_>lastPort)
    {
	fprintf(stderr, "Couldn't find a port to open listener on\n");
	return 0;
    }

    printf("Opened listener on port %d\n", listenPort_);

    if (globus_io_tcp_register_listen(&sock_, jsListenCallback, NULL)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error listening on Globus socket\n");
	return 0;
    }

    return 1;
}

/*
 * Set if this job is going to access the data grid
 */
int datagridNeeded_=0;

/*
 * Set if the job is starting in interactive mode
 */
int interactiveMode_=1;

static int startupJobController()
{
    if (datagridNeeded_)
    {
	if (!qcdgridInit(1))
	{
	    return 0;
	}
    }
    else
    {
	/*
	 * If we're not going to access the datagrid, don't do the full
	 * datagrid initialisation - it takes a while. Just activate
	 * Globus instead
	 */
	if (!activateGlobusModules())
	{
	    return 0;
	}
    }

    /*
     * Obtain user's identity - used for authorising connections from
     * the client
     */
    if (!getCredentials())
    {
	return 0;
    }

    return 1;
}

/*
 * Information on files to be fetched from/stored to the datagrid
 */
struct datagridFile
{
    char *lfn;         /* logical name of file on grid */
    char *pfn;         /* name of file as stored locally */
};

int numFilesToFetch_=0;
struct datagridFile *filesToFetch_=NULL;
int numFilesToStore_=0;
struct datagridFile *filesToStore_=NULL;

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Entry point for the job controller. Parses the job description,
*   submits job to batch system, and opens socket for communication
*   with the client
*    
*   Parameters:                                        [I/O]
*
*     argv[1]  name of the job description file         I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    FILE *jobDescription;
    FILE *portFile;
    char *lfn, *pfn;
    char *batchSystem;
    char *staged;
    int i;

    /*
     * Check arguments. There should be exactly one: the name of a
     * QCDgrid job description file
     */
    if (argc!=2)
    {
	printf("This is an internal part of the QCDgrid job submission software\n");
	printf("It should not be run directly, except for testing purposes\n");
	printf("(Requires one parameter, the name of a job description file)\n");
	return 0;
    }

    /*
     * Try to open the job description file
     */
    jobDescription=fopen(argv[1], "r");
    if (!jobDescription)
    {
	error("Can't open job description file %s\n", argv[1]);
	return 1;
    }

    /*
     * Try to parse it
     */
    if (!parseJobDescription(jobDescription))
    {
	fclose(jobDescription);
	error("Error parsing job description file %s\n", argv[1]);
	return 1;
    }
    fclose(jobDescription);

    /*
     * Get job's directory
     */
    jobDirectory_=getJobDescriptionEntry("directory");
    if (!jobDirectory_)
    {
	error("No directory specified in job description\n");
	return 1;
    }
    if (chdir(jobDirectory_)<0)
    {
	error("Error changing to job directory\n");
	return 1;
    }

    /*
     * Create log file for this controller
     */
    errorLogFile_=fopen("controller.log", "w");
    if (!errorLogFile_)
    {
	error("Error opening controller log file\n");
	return 1;
    }

    /*
     * Set batch system
     */
    batchSystem=getJobDescriptionEntry("batch_system");
    if (!batchSystem)
    {
	error("No batch system specified in job description\n");
    }
    setBatchSystem(batchSystem);

    /*
     * If executable was staged, make it executable
     */
    staged=getJobDescriptionEntry("staged");
    if (!strcmp(staged, "yes"))
    {
	if (chmod(getJobDescriptionEntry("executable"), S_IRWXU)<0)
	{
	    error("Error setting permissions on staged executable\n");
	}
    }

    /*
     * Check for files to fetch from (or store onto) datagrid here, so
     * we know if we need to initialise it
     */
    lfn=getJobDescriptionEntry("fetch");
    while (lfn)
    {
	pfn=getNextJobDescriptionEntry("as");
	if (!pfn)
	{
	    error("Malformed job description ('fetch' without corresponding 'as')\n");
	    return 1;
	}
	numFilesToFetch_++;
	filesToFetch_=realloc(filesToFetch_, numFilesToFetch_*sizeof(struct datagridFile));
	if (!filesToFetch_)
	{
	    error("Out of memory\n");
	    return 1;
	}
	filesToFetch_[numFilesToFetch_-1].lfn=strdup(lfn);
	filesToFetch_[numFilesToFetch_-1].pfn=strdup(pfn);
	
	lfn=getNextJobDescriptionEntry("fetch");
    }

    pfn=getJobDescriptionEntry("store");
    while (pfn)
    {
	lfn=getNextJobDescriptionEntry("as");
	if (!lfn)
	{
	    error("Malformed job description ('store' without corresponding 'as')\n");
	    return 1;
	}
	numFilesToStore_++;
	filesToStore_=realloc(filesToStore_, numFilesToStore_*sizeof(struct datagridFile));
	if (!filesToStore_)
	{
	    error("Out of memory\n");
	    return 1;
	}
	filesToStore_[numFilesToStore_-1].lfn=strdup(lfn);
	filesToStore_[numFilesToStore_-1].pfn=strdup(pfn);
	
	pfn=getNextJobDescriptionEntry("store");
    }

    if ((numFilesToFetch_)||(numFilesToStore_))
    {
	datagridNeeded_=1;
    }

    /*
     * Try to initialise Globus related stuff
     */
    if (!startupJobController())
    {
	error("Error initialising Globus services\n");
	return 1;
    }

    /*
     * Open a socket to listen for communication from the client
     */
    if (!jsListenForMessages())
    {
	error("Error opening Globus socket\n");
	return 1;
    }

    /*
     * Output the port number to a file where the client can get
     * it. Not a particularly neat way of doing this, but the best
     * I can think of.
     */
    portFile=fopen("port", "w");
    if (!portFile)
    {
	error("Error opening port number file\n");
	return 1;
    }
    fprintf(portFile, "%d\n", listenPort_);
    fclose(portFile);

    /*
     * Fetch specified input files from the datagrid
     */
    /*
     * FIXME: support for a whole directory at once?
     */
    fetchingInput_=1;
    for (i=0; i<numFilesToFetch_; i++)
    {
	if (asprintf(&pfn, "%s/%s", jobDirectory_, filesToFetch_[i].pfn)<0)
	{
	    error("Out of memory\n");
	    return 1;
	}
	if (!qcdgridGetFile(filesToFetch_[i].lfn, pfn))
	{
	    error("Error getting input file %s from data grid\n",
		  filesToFetch_[i].lfn);
	    return 1;
	}
    }
    fetchingInput_=0;

    /*
     * If the job is starting in interactive mode, have to wait
     * for a connection from the client before actually starting
     * the job
     */
    if (interactiveMode_)
    {
	/* FIXME: timeout here */
	while ((!clientConnected_));
    }

    /*
     * Actually send the job executable to the batch system
     */
    if (!submitJob(argv[1]))
    {
	error("Error submitting job\n");
	return 1;
    }

    sleep(1);

    error("Job submitted successfully\n");
    while(1)
    {
	/* FIXME: wait for job to terminate here */
	if (getJobStatus()==JOB_FINISHED)
	{
	    break;
	}
    }

    /*
     * Store files on the datagrid
     */
    /* FIXME: metadata?? */
    storingOutput_=1;
    for (i=0; i<numFilesToStore_; i++)
    {
	if (asprintf(&pfn, "%s/%s", jobDirectory_, filesToStore_[i].pfn)<0)
	{
	    error("Out of memory\n");
	    return 1;
	}
	if (!qcdgridPutFile(pfn, filesToStore_[i].lfn))
	{
	    error("Error putting output file %s on data grid\n",
		  filesToStore_[i].lfn);
	}
    }
    storingOutput_=0;

    /*
     * Wait for client to disconnect before exiting
     */
    /* FIXME: maybe timeout? */
    while (clientConnected_);

    return 0;
}
