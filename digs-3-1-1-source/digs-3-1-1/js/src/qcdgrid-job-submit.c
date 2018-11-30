/***********************************************************************
*
*   Filename:   qcdgrid-job-submit.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Job submission program for QCDgrid
*
*   Contents:   Main function
*
*   Used in:    QCDgrid client tools for job submission
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

#include "qcdgrid-js.h"
#include "qcdgrid-client.h"
#include "jobio.h"
#include "job.h"
#include "batch.h"

#include "gridftp.h"
#include "config.h"

#include <globus_gram_client.h>
#include <globus_io.h>
#include <globus_gss_assist.h>
#include <globus_gass_server_ez.h>

extern char *tmpDir_;

/*int makeDirectory(char *host, char *dirName);*/


float portInfoTimeout_=60.0;

static int makeDirectory_globus(char *host, char *dir)
{
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    result = digs_mkdir_globus(errbuf, host, dir);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error making directory %s on %s: %s (%s)",
		   dir, host, digsErrorToString(result), errbuf);
	return 0;
    }
    return 1;
}

/***********************************************************************
*   int copyToLocal(char *remoteHost, char *remoteFile, char *localFile)
*
*   Copies a file to the local file system from a remote host using
*   GridFTP
*    
*   Parameters:                                                     [I/O]
*
*     remoteHost  FQDN of host to copy to                            I
*     remoteFile  Full pathname to file on remote machine            I
*     localFile   Full pathname to file on local machine             I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
static int copyToLocal_globus(char *remoteHost, char *remoteFile, char *localFile)
{
  int handle;
  time_t startTime, endTime;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  digs_error_code_t result;
  float timeout;
  int percentComplete;
  digs_transfer_status_t status;

  result = digs_startGetTransfer_globus(errbuf, remoteHost, remoteFile,
					localFile, &handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error starting get transfer from %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  timeout = getNodeCopyTimeout(remoteHost);

  startTime = time(NULL);
  do {
    result = digs_monitorTransfer_globus(errbuf, handle, &status, &percentComplete);
    if ((result != DIGS_SUCCESS) || (status == DIGS_TRANSFER_FAILED)) {
      logMessage(ERROR, "Error in get transfer from %s: %s (%s)", remoteHost,
		 digsErrorToString(result), errbuf);
      digs_endTransfer_globus(errbuf, handle);
      return 0;
    }
    if (status == DIGS_TRANSFER_DONE) {
      break;
    }

    globus_libc_usleep(1000);

    endTime = time(NULL);
  } while (difftime(endTime, startTime) < timeout);

  result = digs_endTransfer_globus(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error in end transfer from %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  return 1;
}


/***********************************************************************
*   int copyFromLocal(char *localFile, char *remoteHost, char *remoteFile)
*
*   Copies a file from the local file system to a remote host using
*   GridFTP, synchronously
*    
*   Parameters:                                                     [I/O]
*
*     localFile   Full pathname to file on local machine             I
*     remoteHost  FQDN of host to copy to                            I
*     remoteFile  Full pathname to file on remote machine            I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
static int copyFromLocal_globus(char *localFile, char *remoteHost, char *remoteFile)
{
  int handle;
  time_t startTime, endTime;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  digs_error_code_t result;
  float timeout;
  int percentComplete;
  digs_transfer_status_t status;

  result = digs_startPutTransfer_globus(errbuf, remoteHost, localFile,
					remoteFile, &handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error starting put transfer to %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  timeout = getNodeCopyTimeout(remoteHost);

  startTime = time(NULL);
  do {
    result = digs_monitorTransfer_globus(errbuf, handle, &status, &percentComplete);
    if ((result != DIGS_SUCCESS) || (status == DIGS_TRANSFER_FAILED)) {
      logMessage(ERROR, "Error in put transfer to %s: %s (%s)", remoteHost,
		 digsErrorToString(result), errbuf);
      digs_endTransfer_globus(errbuf, handle);
      return 0;
    }
    if (status == DIGS_TRANSFER_DONE) {
      break;
    }

    globus_libc_usleep(1000);

    endTime = time(NULL);
  } while (difftime(endTime, startTime) < timeout);

  result = digs_endTransfer_globus(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error in end transfer to %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  return 1;
}

char *relativePathToFullPath(char *path)
{
    char cwd[1000];
    char *result;

    if (path[0]=='/')
    {
	return strdup(path);
    }

    getcwd(cwd, 1000);

    if (asprintf(&result, "%s/%s", cwd, path)<0)
    {
	return NULL;
    }
    return result;
}

hostInfo_t *readHostInfo(char *hostname)
{
    char *tmp, *key;
    hostInfo_t *hostInfo=NULL;

    tmp=getFirstConfigValue("jobcfg", "host");
    while (tmp)
    {
	if (!strcmp(hostname, tmp))
	{
	    hostInfo=malloc(sizeof(hostInfo_t));
	    if (!hostInfo)
	    {
		fprintf(stderr, "Out of memory\n");
		return NULL;
	    }

	    /* initialise host info to default values */
	    hostInfo->name=strdup(tmp);
	    hostInfo->tmpDir="UNKNOWN";
	    hostInfo->qcdgridDir=NULL;
	    hostInfo->batchSystem=BATCH_NONE;
	    hostInfo->pbsPath=NULL;
	    hostInfo->extraContact=NULL;
	    hostInfo->extraRsl=NULL;

	    key=getNextConfigKeyValue("jobcfg", &tmp);
	    while ((key)&&(strcmp(key, "host")))
	    {
		if (!strcmp(key, "tmpdir"))
		{
		    free(hostInfo->tmpDir);
		    hostInfo->tmpDir=strdup(tmp);
		}
		else if (!strcmp(key, "qcdgrid_path"))
		{
		    hostInfo->qcdgridDir=strdup(tmp);
		}
		else if (!strcmp(key, "batch_system"))
		{
		    if (!strcmp(tmp, "pbs"))
		    {
			hostInfo->batchSystem=BATCH_PBS;
		    }
		    else if (!strcmp(tmp, "none"))
		    {
			hostInfo->batchSystem=BATCH_NONE;
		    }
		    else
		    {
			fprintf(stderr, "Unknown batch system `%s' specified for host %s - defaulting to none\n",
				tmp, hostname);
			hostInfo->batchSystem=BATCH_NONE;
		    }
		}
		else if (!strcmp(key, "pbs_path"))
		{
		    hostInfo->pbsPath=strdup(tmp);
		}
		else if (!strcmp(key, "extra_contact"))
		{
		    hostInfo->extraContact=strdup(tmp);
		}
		else if (!strcmp(key, "extra_rsl"))
		{
		    hostInfo->extraRsl=strdup(tmp);
		}
		else
		{
		    fprintf(stderr, "Unknown property `%s' specified for host %s\n",
			    key, hostname);
		}

		key=getNextConfigKeyValue("jobcfg", &tmp);
	    }
	}
	else
	{
	    tmp=getNextConfigValue("jobcfg", "host");
	}
    }
    return hostInfo;
}

/*
 * Name of job's local directory
 */
static char *localJobDir_;

/***********************************************************************
*   char *jsGenerateRsl(char *jobName, int argc, char **argv,
*                       int envc, char **envv, char *wd)
*
*   Converts the executable path and argument list into a Globus
*   resource specification language string    
*    
*   Parameters:                                                    [I/O]
*
*     jobName   Full path to executable                             I
*     argc      Number of arguments (doesn't include exec name)     I
*     argv      Vector of arguments to pass (maybe NULL if argc=0)  I
*     envc      Number of environment vars to set                   I
*     envv      Vector of environment vars (maybe NULL if envc=0)   I
*     wd        Working directory to set (may be NULL if not reqd)  I
*     gassUrl   GASS server URL for stdout (may be NULL)            I
*    
*   Returns: Pointer to dynamically allocated RSL string,
*            or NULL on error
***********************************************************************/
static char *jsGenerateRsl(char *jobName, int argc, char **argv,
			   int envc, char **envv, char *wd, char *gassUrl,
			   char *extraRsl)
{
    char *buffer;
    char *equals;
    int length;
    int i;
    int eqOffset;
    char *stdoutBuffer=NULL;
    char *fullStdout=NULL;
    char *stderrBuffer=NULL;
    char *fullStderr=NULL;

    /* First need to know how long a string to allocate */
    length=strlen("&(executable=)(arguments=)");
    for (i=0; i<argc; i++)
    {
	length+=strlen(argv[i])+1;
    }
    length+=strlen(jobName);
    length+=10;

    if (wd)
    {
	length+=strlen(wd)+strlen("(directory=)")+1;
    }

    if (envc!=0)
    {
	length+=strlen("(environment=)");
	for (i=0; i<envc; i++)
	{
	    length+=strlen(envv[i])+2;
	}
    }

    if (gassUrl!=NULL)
    {
	if (asprintf(&stdoutBuffer, "%s/stdout", localJobDir_)<0) {
	    return NULL;
	}
	fullStdout=relativePathToFullPath(stdoutBuffer);
	free(stdoutBuffer);

	if (asprintf(&stderrBuffer, "%s/stderr", localJobDir_)<0) {
	    return NULL;
	}
	fullStderr=relativePathToFullPath(stderrBuffer);
	free(stderrBuffer);

	length+=strlen("(stdout=)")+strlen(gassUrl)+strlen(fullStdout)+1;
	length+=strlen("(stderr=)")+strlen(gassUrl)+strlen(fullStderr)+1;
    }

    if (extraRsl!=NULL)
    {
	length+=strlen(extraRsl);
    }
   
    /* Allocate storage for RSL */
    buffer=malloc(length);
    if (!buffer)
    {
	return NULL;
    }
    
    /* Do basic executable string */
    sprintf(buffer, "&(executable=%s)", jobName);

    /* Add arguments if any */
    if (argc)
    {
	strcat(buffer, "(arguments=");
	for (i=0; i<argc; i++)
	{
	    strcat(buffer, argv[i]);
	    if (i!=(argc-1)) strcat(buffer, " ");
	    else strcat(buffer, ")");
	}
    }

    /* Add working directory if specified */
    if (wd)
    {
	strcat(buffer, "(directory=");
	strcat(buffer, wd);
	strcat(buffer, ")");
    }

    /* Add stdout/stderr via GASS if specified */
    if (gassUrl)
    {
	strcat(buffer, "(stdout=");
	strcat(buffer, gassUrl);
	strcat(buffer, fullStdout);
	strcat(buffer, ")");

	strcat(buffer, "(stderr=");
	strcat(buffer, gassUrl);
	strcat(buffer, fullStderr);
	strcat(buffer, ")");

	free(fullStdout);
	free(fullStderr);
    }

    /* Add environment */
    if (envc)
    {
	strcat(buffer, "(environment=");
	
	for (i=0; i<envc; i++)
	{
	    equals=strchr(envv[i], '=');
	    if (equals==NULL)
	    {
		fprintf(stderr, "Warning, malformed environment setting `%s'\n", envv[i]);
	    }
	    else
	    {
		strcat(buffer, "(");
		eqOffset=equals-envv[i];
		envv[i][eqOffset]=' ';
		strcat(buffer, envv[i]);
		envv[i][eqOffset]='=';
		strcat(buffer, ")");
	    }
	}

	strcat(buffer, ")");
    }

    if (extraRsl!=NULL)
    {
	strcat(buffer, extraRsl);
    }

    return buffer;
}

static volatile int currentJobState_;
static volatile int currentJobError_;

static char *jobContact_;

/***********************************************************************
*   void jsGlobusJobCallback(void *userArg, char *jobContact, int state, int error)
*    
*   Callback function used by Globus to notify us of state changes
*   of the job
*    
*   Parameters:                                                    [I/O]
*
*     userArg     User defined argument. Not used                   I
*     jc          Contact string for job. Not used                  I
*     state       Job's current state                               I
*     error       Error code, if job's state is FAILED. Not used    I
*    
*   Returns: (void)
***********************************************************************/
static void jsGlobusJobCallback(void *userArg, char *jc, int state, int error)
{
    /*
     * Simply set the global variable to notify the main job execution
     * function of what is going on. All the decisions happen there.
     */
    currentJobError_=error;
/*    printf("CALLBACK: %d\n", state);*/
    currentJobState_=state;
}

/***********************************************************************
*   int jsExecuteJob(char *hostname, char *jobName, int argc, char **argv)
*    
*   Runs an executable on a remote host, using Globus
*    
*   Parameters:                                                    [I/O]
*
*     hostname  FQDN of host to run on                              I
*     jobName   Full path to executable                             I
*     argc      Number of arguments (doesn't include exec name)     I
*     argv      Vector of arguments to pass (maybe NULL if argc=0)  I
*     envc      Number of environment variables (maybe 0)           I
*     envv      Vector of environment vars (maybe NULL if envc=0)   I
*     wd        Working directory for job (NULL means don't care)   I
*     gassUrl   GASS server to receive stdout/err (maybe NULL)      I
*     extraContact   String to add to job manager name (maybe NULL) I
*
*   Returns: 1 for success, 0 on error
***********************************************************************/
static int jsExecuteJob(char *hostname, char *jobName, int argc,
			char **argv, int envc, char **envv, char *wd,
			char *gassUrl, char *extraContact,
			char *extraRsl)
{
    char *rsl;
    char *callbackContact=NULL;
    int err;
    time_t startTime;
    time_t endTime;
    char *jobManager;

    rsl=jsGenerateRsl(jobName, argc, argv, envc, envv, wd, gassUrl,
		      extraRsl);

    if (!rsl)
    {
	return 0;
    }

    /* Reset the state variable */
    currentJobState_=GLOBUS_GRAM_PROTOCOL_JOB_STATE_PENDING;

    /* Open a port to listen for job state info */
    err=globus_gram_client_callback_allow(jsGlobusJobCallback, NULL, &callbackContact);
    if (err!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Globus error: %s\n",
		globus_gram_client_error_string(err));
    }

    if (callbackContact==NULL)
    {
	free(rsl);
	return 0;
    }

    printf("RSL=%s\n", rsl);

    if (extraContact)
    {
	if (asprintf(&jobManager, "%s%s", hostname, extraContact)<0)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 0;
	}
    }
    else
    {
	jobManager=strdup(hostname);
    }

    /* Start the job running */
    err=globus_gram_client_job_request(jobManager, rsl, GLOBUS_GRAM_PROTOCOL_JOB_STATE_ALL,
				       callbackContact, &jobContact_);
    free(jobManager);
    if (err!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Globus error: %s\n",
		globus_gram_client_error_string(err));
	free(rsl);
	globus_gram_client_callback_disallow(callbackContact);
	return 0;
    }

    /* Give the job 600 seconds before we assume it's gone pear shaped */
    startTime=time(NULL);


    /* Wait for it to terminate
    while (currentJobState_!=GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE)
    {
	if (currentJobState_==GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED)
	{
	    fprintf(stderr, "Globus error: %s\n",
		    globus_gram_client_error_string(err));

	    free(rsl);
	    globus_gram_client_callback_disallow(callbackContact);
	    return 0;
	}

	endTime=time(NULL);
	if (difftime(endTime, startTime)>600.0)
	{
	    free(rsl);
	    globus_gram_client_callback_disallow(callbackContact);
	    fprintf(stderr, "Job submission timed out\n");
	    switch (currentJobState_)
	    {
	    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_PENDING:
		printf("State pending\n");
		break;
	    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_ACTIVE:
		printf("State active\n");
		break;
	    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED:
		printf("State failed\n");
		break;
	    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE:
		printf("State done\n");
		break;
	    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_SUSPENDED:
		printf("State suspended\n");
		break;
	    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_UNSUBMITTED:
		printf("State unsubmitted\n");
		break;
	    default:
		printf("Unknown job state %d\n", currentJobState_);
		break;
	    }
	    return 0;
	}

	n=read(STDIN_FILENO, stdinBuffer, 1000);
	if ((n>0)&&(isSock2Connected_))
	{
	    stdinBuffer[n]=0;
	    globus_io_write(&sock2_, stdinBuffer, n, &o);
	}

	globus_gram_client_job_status(jobContact_, &currentJobState_, &err);
    }

*/
    free(rsl);
/*    globus_gram_client_callback_disallow(callbackContact);*/

    return 1;
}

/*
 * Set if no QCDgrid software is installed on the server side.
 * In this case all access to the remote machine must use plain Globus
 */
static int clientOnlyMode_=0;

/*
 * Set if going straight into interactive mode (i.e. redirecting local
 * I/O streams to and from job's right after we submit the job)
 */
static int interactiveMode_=1;

/*
 * Name of job's directory on server
 */
static char *remoteJobDir_;

int readJobConfig()
{
    char *jobConfigFile;

    /*
     * First try user-specific config file in their home directory
     */
    if (asprintf(&jobConfigFile, "%s/.qcdgridjob", getenv("HOME"))<0)
    {
	return 0;
    }
    if (!loadConfigFile(jobConfigFile, "jobcfg"))
    {
	/*
	 * User specific file couldn't be opened. Try system-wide file
	 */
	printf("Trying to load global config file\n");
	if (!loadConfigFile("/etc/qcdgridjob.conf", "jobcfg"))
	{
	    fprintf(stderr, "Couldn't find QCDgrid job config file "
		    "in either %s or /etc/qcdgridjob.conf\n",
		    jobConfigFile);
	    return 0;
	}
	printf("Loaded global config file\n");
    }	
    free(jobConfigFile);

    /*
     * Now parse useful information out of file
     */
    

    return 1;
}

/*
 * Various information parsed out of the command line is stored here
 * in an intermediate form before being written into the job description
 * file (server mode) or processed further (client only mode)
 */
static int numJobArgs_=0;
static char **jobArgs_=NULL;

static int numJobEnvVars_=0;
static char **jobEnvVars_=NULL;

static int numInputFiles_=0;
static char **inputFiles_=NULL;

/*
 * Information on files to be fetched from/stored to the datagrid
 */
struct datagridFile
{
    char *lfn;         /* logical name of file on grid */
    char *pfn;         /* name of file as stored locally */
};

static int numFilesToFetch_=0;
static struct datagridFile *filesToFetch_=NULL;
static int numFilesToStore_=0;
static struct datagridFile *filesToStore_=NULL;

char *batchNames_[2]={ "none", "pbs" };

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Top level main function of QCDgrid job submission utility
*    
*   Parameters:                                             [I/O]
*
*     argv[1]  Machine name to submit job to                 I
*     argv[2]  Executable to run on machine                  I
*     argv[3+] Arguments for executable 
*
*   Returns: 0 on success, 1 on failure
************************************************************************/
int main(int argc, char *argv[])
{
    int port;
    char *hostname;
    int localJobId;
    int remoteJobId;
    FILE *jd;
    char *jdFileName;
    char *exeName;
    char *remoteJdFileName;
    char *remoteExeName;
    char *remoteTmpDir;
    char **controllerArgs;
    char *controllerName;
    char stdinBuffer[1000];
    char *tmp;
    int i, j;
    hostInfo_t *hostInfo;
    int stagingExecutable;
    char *tmpFileName;
    unsigned short gassPort;
    char *gassUrl=NULL;

    globus_gass_transfer_listener_t gassListener=GLOBUS_NULL;
    globus_gass_transfer_listenerattr_t * gassAttr=GLOBUS_NULL;
    globus_gass_transfer_requestattr_t * gassReqAttr=GLOBUS_NULL;
    char *gassScheme=NULL;
    
    if (!activateGlobusModules())
    {
	fprintf(stderr, "Error activating Globus modules\n");
	return 1;
    }

    if (!readJobConfig())
    {
	fprintf(stderr, "Error reading QCDgrid job submission system config\n");
	return 1;
    }

    tmp=getenv("QCDGRID_TMP");
    if (tmp)
    {
	tmpDir_=strdup(tmp);
    }
    else
    {
	tmpDir_=strdup("/tmp");
    }

    stagingExecutable=0;

    /*
     * Parse parameters here so we know about modes, arguments,
     * environment, etc.
     */
    remoteTmpDir=NULL;
    j=0;
    for (i=1; i<argc; i++)
    {
	/*
	 * Look for special switches first
	 */
	if (!strcmp(argv[i], "-fetch"))
	{
	    /*
	     * Fetch a file from the datagrid
	     */
	    numFilesToFetch_++;
	    filesToFetch_=realloc(filesToFetch_, numFilesToFetch_*sizeof(struct datagridFile));
	    if (!filesToFetch_)
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    filesToFetch_[numFilesToFetch_-1].lfn=strdup(argv[i+1]);
	    filesToFetch_[numFilesToFetch_-1].pfn=strdup(argv[i+2]);
	    if ((!filesToFetch_[numFilesToFetch_-1].lfn)||
		(!filesToFetch_[numFilesToFetch_-1].pfn))
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    i+=2;
	}
	else if (!strcmp(argv[i], "-store"))
	{
	    /*
	     * Store a file onto the datagrid
	     */
	    numFilesToStore_++;
	    filesToStore_=realloc(filesToStore_, numFilesToStore_*sizeof(struct datagridFile));
	    if (!filesToStore_)
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    filesToStore_[numFilesToStore_-1].pfn=strdup(argv[i+1]);
	    filesToStore_[numFilesToStore_-1].lfn=strdup(argv[i+2]);
	    if ((!filesToStore_[numFilesToStore_-1].lfn)||
		(!filesToStore_[numFilesToStore_-1].pfn))
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    i+=2;
	}
	else if (!strcmp(argv[i], "-input"))
	{
	    /*
	     * Specify an input file to copy to the server
	     */
	    numInputFiles_++;
	    inputFiles_=realloc(inputFiles_, numInputFiles_*sizeof(char*));
	    if (!inputFiles_)
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    inputFiles_[numInputFiles_-1]=strdup(argv[i+1]);
	    if (!inputFiles_[numInputFiles_-1])
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    i++;
	}
	else if (!strcmp(argv[i], "-env"))
	{
	    /*
	     * Specify an environment variable for the job
	     */
	    numJobEnvVars_++;
	    jobEnvVars_=realloc(jobEnvVars_, numJobEnvVars_*sizeof(char*));
	    if (!jobEnvVars_)
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    jobEnvVars_[numJobEnvVars_-1]=strdup(argv[i+1]);
	    if (!jobEnvVars_[numJobEnvVars_-1])
	    {
		fprintf(stderr, "Out of memory!\n");
		return 1;
	    }
	    i++;
	}
	else if (!strcmp(argv[i], "-batch"))
	{
	    /*
	     * Run in batch mode, not interactive
	     */
	    interactiveMode_=0;
	}
	else if (!strcmp(argv[i], "-stage"))
	{
	    stagingExecutable=1;
	}
	else if (!strcmp(argv[i], "-tmpdir"))
	{
	    remoteTmpDir=strdup(argv[i+1]);
	    i++;
	}
	else
	{
	    if (j==0)
	    {
		hostname=argv[i];
		j++;
	    }
	    else if (j==1)
	    {
		exeName=argv[i];
		j++;
	    }
	    else
	    {
		/*
		 * Argument
		 */
		numJobArgs_++;
		jobArgs_=realloc(jobArgs_, numJobArgs_*sizeof(char*));
		if (!jobArgs_)
		{
		    fprintf(stderr, "Out of memory!\n");
		    return 1;
		}
		jobArgs_[numJobArgs_-1]=strdup(argv[i]);
		if (!jobArgs_[numJobArgs_-1])
		{
		    fprintf(stderr, "Out of memory!\n");
		    return 1;
		}
		j++;
	    }
	}
    }

    hostInfo=readHostInfo(hostname);
    if (!hostInfo)
    {
	fprintf(stderr, "Host %s not listed in config file\n", hostname);
	return 1;
    }

    /*
     * Override default temp directory if specified on command line
     */
    if (remoteTmpDir)
    {
	hostInfo->tmpDir=remoteTmpDir;
    }

    if (hostInfo->qcdgridDir!=NULL)
    {
	clientOnlyMode_=0;
    }
    else
    {
	clientOnlyMode_=1;
    }

    printf("Read config\n");

    /*
     * Create a local directory for the job
     */
    /*
     * Work out a unique ID number for this job and use it to create a directory
     */
    localJobId=0;
    do
    {
	if (localJobDir_)
	{
	    free(localJobDir_);
	}
	localJobId++;
	
	if (localJobId>=1000000)
	{
	    fprintf(stderr, "Too many job results (1000000) in this directory already\n");
	    fprintf(stderr, "Please delete some of them and try again\n");
	    return 1;
	}
	
	if (asprintf(&localJobDir_, "qcdgridjob%06d", localJobId)<0)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 1;
	}
    } while (mkdir(localJobDir_, S_IRWXU)<0);
    printf("Storing results in local directory %s\n", localJobDir_);
    
    if (!clientOnlyMode_)
    {
	/*
	 * Create a directory on the server for the job
	 */
	{
	    char *getdirArgs[2]; /*={ "/tmp", NULL };*/
	    char *getdirName;

	    getdirArgs[0]=hostInfo->tmpDir;
	    getdirArgs[1]=NULL;

	    if (asprintf(&getdirName, "%s/qcdgrid-job-getdir", hostInfo->qcdgridDir)<0)
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    if (!executeJob(hostname, getdirName, 1, &getdirArgs[0]))
	    {
		fprintf(stderr, "Cannot create remote job directory\n");
		return 1;
	    }
	    remoteJobDir_=strdup(getLastJobOutput());
	    printf("Storing results in remote directory %s\n", remoteJobDir_);
	}

	/*
	 * Work out server side name of EXE
	 */
	if (!stagingExecutable)
	{
	    remoteExeName=strdup(exeName);
	}
	else
	{
	    char *exeBaseName;

	    exeBaseName=&exeName[strlen(exeName)-1];
	    while ((exeBaseName>exeName)&&((*exeBaseName)!='/'))
	    {
		exeBaseName--;
	    }
	    if ((*exeBaseName)=='/')
	    {
		exeBaseName++;
	    }

	    if (asprintf(&remoteExeName, "%s/%s", remoteJobDir_, exeBaseName)<0)
	    {
		fprintf(stderr, "out of memory\n");
		return 1;
	    }
	}

	/*
	 * Prepare job description file, and upload it
	 */
	if (asprintf(&jdFileName, "%s/jobdesc", localJobDir_)<0)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 1;
	}
	jd=fopen(jdFileName, "w");
	if (!jd)
	{
	    fprintf(stderr, "Error creating job description file\n");
	    return 1;
	}
	fprintf(jd, "executable=%s\n", remoteExeName);
	if (stagingExecutable)
	{
	    fprintf(jd, "staged=yes\n");
	}
	else
	{
	    fprintf(jd, "staged=no\n");
	}
	fprintf(jd, "directory=%s\n", remoteJobDir_);
	for (i=0; i<numJobArgs_; i++)
	{
	    fprintf(jd, "arg=%s\n", jobArgs_[i]);
	}
	for (i=0; i<numJobEnvVars_; i++)
	{
	    fprintf(jd, "env=%s\n", jobEnvVars_[i]);
	}
	for (i=0; i<numFilesToFetch_; i++)
	{
	    fprintf(jd, "fetch=%s\nas=%s\n", filesToFetch_[i].lfn,
		    filesToFetch_[i].pfn);
	}
	for (i=0; i<numFilesToStore_; i++)
	{
	    fprintf(jd, "store=%s\nas%s\n", filesToStore_[i].pfn,
		    filesToStore_[i].lfn);
	}
	for (i=0; i<numInputFiles_; i++)
	{
	    char *inputFileBase;

	    /* point to base name of input file (ignore path) */
	    inputFileBase=inputFiles_[i]+strlen(inputFiles_[i])-1;
	    while ((inputFileBase>inputFiles_[i])&&
		   (*inputFileBase!='/'))
	    {
		inputFileBase--;
	    }
	    if (*inputFileBase=='/')
	    {
		inputFileBase++;
	    }
	    fprintf(jd, "input=%s\n", inputFileBase);
	}
	fprintf(jd, "qcdgrid_path=%s\n", hostInfo->qcdgridDir);
	fprintf(jd, "batch_system=%s\n", batchNames_[hostInfo->batchSystem]);
	if (hostInfo->batchSystem==BATCH_PBS)
	{
	    fprintf(jd, "pbs_path=%s\n", hostInfo->pbsPath);
	}
	fclose(jd);

	if (asprintf(&remoteJdFileName, "%s/jobdesc", remoteJobDir_)<0)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 1;
	}

	if (!copyFromLocal_globus(jdFileName, hostname,  remoteJdFileName))
	{
	    fprintf(stderr, "Error copying job description file to server\n");
	    return 1;
	}

	if (stagingExecutable)
	{
	    if (!copyFromLocal_globus(exeName, hostname, remoteExeName))
	    {
		fprintf(stderr, "Error staging executable to server\n");
		return 1;
	    }
	}

	/*
	 * Copy any input files from the local machine over
	 */
	for (i=0; i<numInputFiles_; i++)
	{
	    char *remoteInputFile;
	    char *inputFileBase;
	    char *localInputFile;

	    /* point to base name of input file (ignore path) */
	    inputFileBase=inputFiles_[i]+strlen(inputFiles_[i])-1;
	    while ((inputFileBase>inputFiles_[i])&&
		   (*inputFileBase!='/'))
	    {
		inputFileBase--;
	    }
	    if (*inputFileBase=='/')
	    {
		inputFileBase++;
	    }

	    if (asprintf(&remoteInputFile, "%s/%s", remoteJobDir_, inputFileBase)<0)
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }
	    localInputFile=relativePathToFullPath(inputFiles_[i]);
	    if (!copyFromLocal_globus(localInputFile, hostname, remoteInputFile))
	    {
		fprintf(stderr, "Error copying over input file %s\n", inputFiles_[i]);
		return 1;
	    }
	    free(localInputFile);
	    free(remoteInputFile);
	}

	/*
	 * Actually submit job (well, controller in server side mode)
	 */
	controllerArgs=malloc(sizeof(char*));
	if (!controllerArgs)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 1;
	}
	controllerArgs[0]=remoteJdFileName;

	if (asprintf(&controllerName, "%s/qcdgrid-job-controller", hostInfo->qcdgridDir)<0)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 1;
	}
	if (!jsExecuteJob(hostname, controllerName, 1, controllerArgs, 0, NULL, NULL, NULL, NULL,
			  NULL))
	{
	    fprintf(stderr, "Can't execute job\n");
	    return 1;
	}

	if (interactiveMode_)
	{
	    char *remotePortName, *localPortName;
	    FILE *portFile;
	    time_t startTime;

	    /*
	     * If we're running in interactive mode, connect to the job
	     * controller straight away, and start redirecting I/O/err
	     * streams
	     */
	    /*
	     * First try to read the port number from the remote job
	     * directory
	     */
	    if ((asprintf(&remotePortName, "%s/port", remoteJobDir_)<0)||
		(asprintf(&localPortName, "%s/port", localJobDir_)<0))
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    startTime=time(NULL);
	    while (difftime(time(NULL), startTime) < portInfoTimeout_)
	    {
		if (copyToLocal_globus(hostname, remotePortName, localPortName))
		{
		    break;
		}
	    }
	    portFile=fopen(localPortName, "r");
	    fscanf(portFile, "%d", &port);
	    printf("Connecting to port %d...\n", port);

	    /*
	     * Now actually connect
	     */
	    if (!connectToHost(hostname, port))
	    {
		fprintf(stderr, "Can't connect to host\n");
		return 1;
	    }

	    /*
	     * Set stdin to non-blocking so we can monitor it without holding up the
	     * monitoring of the output/error streams
	     */
	    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0)|O_NONBLOCK)<0)
	    {
		fprintf(stderr, "Can't set stdin to non-blocking\n");
	    }

	    sleep(1);

	    /*
	     * FIXME: some way to exit this (or maybe just CTRL-C?)
	     * Anyway, should implement a SIGINT handler to exit
	     * cleanly
	     */
	    while (1)
	    {
		char *buffer;
		int len;
		len=getNewOutputData(&buffer, 0);
		if (len>0)
		{
		    printf("OUTPUT: ");
		    fwrite(buffer, len, 1, stdout);
		    printf("\n");
		    free(buffer);
		}
		len=getNewOutputData(&buffer, 1);
		if (len>0)
		{
		    printf("ERROR: ");
		    fwrite(buffer, len, 1, stdout);
		    printf("\n");
		    free(buffer);
		}

		/*
		 * Check for new data on stdin, if there is any send it down the
		 * Globus socket to the job wrapper
		 */
		len=read(STDIN_FILENO, stdinBuffer, 999);
		if (len>0)
		{
		    stdinBuffer[len]=0;
		    if (!sendNewInputData(stdinBuffer, len))
		    {
			fprintf(stderr, "Error sending input data\n");
		    }
		}
//		sleep(1);

		/*
		 * Check for job termination, exit loop if this happens
		 */
		if (getJobState()==JOB_FINISHED)
		{
		    char **ofns;

		    printf("Job has completed\n");
		    /*
		     * Fetch output files now
		     */
		    ofns=getOutputFileNames();
		    if (!ofns)
		    {
			fprintf(stderr, "Error getting output filenames\n");
		    }
		    else
		    {
			i=0;
			while (ofns[i])
			{
			    char *remoteOutputFile;
			    char *localOutputFile;

			    if ((asprintf(&remoteOutputFile, "%s/%s", remoteJobDir_, ofns[i])<0)||
				(asprintf(&localOutputFile, "%s/%s", localJobDir_, ofns[i])<0))
			    {
				fprintf(stderr, "Out of memory\n");
				return 1;
			    }

			    printf("Retrieving %s\n", ofns[i]);
			    if (!copyToLocal_globus(hostname, remoteOutputFile, localOutputFile))
			    {
				fprintf(stderr, "Error copying output file %s\n", ofns[i]);
			    }
			    free(remoteOutputFile);
			    free(localOutputFile);

			    free(ofns[i]);
			    i++;
			}
			free(ofns);
		    }
		    break;
		}
	    }
	}
	else
	{
	    /*
	     * Otherwise print a job ID for access to it later, and
	     * exit
	     */
	    printf("Job ID: %s\n", localJobDir_);
	}
    }
    else
    {
	/*
	 * Client only mode
	 * In this mode, there is no QCDgrid software installed
	 * on the server, only plain Globus. So some features,
	 * e.g. interactive jobs, will not be possible. Others
	 * will be moved to the client side, e.g. fetching from
	 * and storing to the data grid. Still others will be
	 * implemented differently, e.g. streaming of output and
	 * access to batch systems.
	 */

	/*
	 * Create remote directory for job
	 */
	/*
	 * FIXME: replace this whole thing with a more efficient and correct
	 * gridFTP function
	 */
	remoteJobId=0;
	do
	{
	    if (remoteJobDir_)
	    {
		free(remoteJobDir_);
	    }
	    remoteJobId++;
	    
	    if (remoteJobId>=1000000)
	    {
		fprintf(stderr, "Too many job results (1000000) in this directory already\n");
		fprintf(stderr, "Please delete some of them and try again\n");
		return 1;
	    }
	    /*
	     * FIXME: is /tmp a good place for this??
	     */
	    if (asprintf(&remoteJobDir_, "/tmp/qcdgridjob%06d", remoteJobId)<0)
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }
	    /*
	     * FIXME: don't really want to wait for it to try this 1000000 times if
	     * the error is a time out or something, not prior existence...
	     */
	} while (!makeDirectory_globus(hostname, remoteJobDir_));

	/*
	 * Work out server side name of EXE
	 */
	if (!stagingExecutable)
	{
	    remoteExeName=strdup(exeName);
	}
	else
	{
	    char *exeBaseName;

	    exeBaseName=&exeName[strlen(exeName)-1];
	    while ((exeBaseName>exeName)&&((*exeBaseName)!='/'))
	    {
		exeBaseName--;
	    }
	    if ((*exeBaseName)=='/')
	    {
		exeBaseName++;
	    }

	    if (asprintf(&remoteExeName, "%s/%s", remoteJobDir_, exeBaseName)<0)
	    {
		fprintf(stderr, "out of memory\n");
		return 1;
	    }
	}

	/*
	 * Stage executable if necessary
	 */
	if (stagingExecutable)
	{
	    if (!copyFromLocal_globus(exeName, hostname, remoteExeName))
	    {
		fprintf(stderr, "Error staging executable to server\n");
		return 1;
	    }
	}

	/*
	 * Copy any input files from the local machine over
	 */
	for (i=0; i<numInputFiles_; i++)
	{
	    char *remoteInputFile;
	    char *inputFileBase;
	    char *localInputFile;

	    /* point to base name of input file (ignore path) */
	    inputFileBase=inputFiles_[i]+strlen(inputFiles_[i])-1;
	    while ((inputFileBase>inputFiles_[i])&&
		   (*inputFileBase!='/'))
	    {
		inputFileBase--;
	    }
	    if (*inputFileBase=='/')
	    {
		inputFileBase++;
	    }

	    if (asprintf(&remoteInputFile, "%s/%s", remoteJobDir_, inputFileBase)<0)
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }
	    localInputFile=relativePathToFullPath(inputFiles_[i]);
	    if (!copyFromLocal_globus(localInputFile, hostname, remoteInputFile))
	    {
		fprintf(stderr, "Error copying over input file %s\n", inputFiles_[i]);
		return 1;
	    }
	    free(localInputFile);
	    free(remoteInputFile);
	}

	/*
	 * Now fetch input files from the grid and copy them over
	 */
	if (asprintf(&tmpFileName, "%s/tmpgridfile", localJobDir_)<0)
	{
	    fprintf(stderr, "Out of memory\n");
	    return 1;
	}
	for (i=0; i<numFilesToFetch_; i++)
	{
	    char *destName;

	    if (!qcdgridGetFile(filesToFetch_[i].lfn, tmpFileName))
	    {
		fprintf(stderr, "Error fetching input file %s from grid\n", filesToFetch_[i].lfn);
		return 1;
	    }

	    if (asprintf(&destName, "%s/%s", remoteJobDir_, filesToFetch_[i].pfn)<0)
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    if (!copyFromLocal_globus(tmpFileName, hostname, destName))
	    {
		fprintf(stderr, "Error copying input file %s\n", filesToFetch_[i].pfn);
		return 1;
	    }
	    free(destName);
	}
	unlink(tmpFileName);
	free(tmpFileName);

	/*
	 * If 'interactive mode', try to setup a GASS server to listen for output/error info
	 */
	if (interactiveMode_)
	{
	    globus_result_t res;

	    gassPort=0;

	    printf("Trying to create GASS server...\n");
/*	    res=globus_gass_server_ez_init(&gassPort, &gassUrl, 0, NULL);*/

	    res=globus_gass_server_ez_init(&gassListener,
					   gassAttr,
					   gassScheme,
					   gassReqAttr,
					   GLOBUS_GASS_SERVER_EZ_LINE_BUFFER |
					   GLOBUS_GASS_SERVER_EZ_TILDE_EXPAND |
					   GLOBUS_GASS_SERVER_EZ_TILDE_USER_EXPAND |
					   GLOBUS_GASS_SERVER_EZ_WRITE_ENABLE |
					   GLOBUS_GASS_SERVER_EZ_STDOUT_ENABLE |
					   GLOBUS_GASS_SERVER_EZ_STDERR_ENABLE |
					   GLOBUS_GASS_SERVER_EZ_READ_ENABLE,
					   NULL);	    

	    if (res!=GLOBUS_SUCCESS)
	    {
		fprintf(stderr, "Error creating GASS server to listen for job output\n");
		return 1;
	    }
	    printf("Created GASS server. Trying to get URL\n");

	    gassUrl=globus_gass_transfer_listener_get_base_url(gassListener);
	}

	/*
	 * Generate Globus RSL for job, and
	 * submit job via GRAM
	 */
	if (!jsExecuteJob(hostname, remoteExeName, numJobArgs_, jobArgs_,
			  numJobEnvVars_, jobEnvVars_, remoteJobDir_,
			  gassUrl, hostInfo->extraContact, hostInfo->extraRsl))
	{
	    fprintf(stderr, "Error sending job to remote host\n");
	    return 1;
	}

	/*
	 * If 'interactive mode', try to connect and stream output
	 */
	if (interactiveMode_)
	{
	    int err;
	    
	    int stdoutLen;
	    int prevStdoutLen=0;
	    int stderrLen;
	    int prevStderrLen=0;
	    FILE *f;
	    char *stdoutName;
	    char *stderrName;

	    printf("Warning, interactive mode only works for stdout/stderr on non-QCDgrid servers\n");
	    printf("You cannot send input to the job as it runs\n");

	    if (asprintf(&stderrName, "%s/stderr", localJobDir_)<0) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }
	    if (asprintf(&stdoutName, "%s/stdout", localJobDir_)<0) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    /*
	     * Wait for job to exit
	     */
	    do {

		sleep(1);
		/*
		 * Redirect any new output to the console
		 */
		f=fopen(stdoutName, "rb");
		fseek(f, 0, SEEK_END);
		stdoutLen=ftell(f);
		if (stdoutLen>prevStdoutLen) {
		    printf("OUTPUT: ");
		    fseek(f, prevStdoutLen, SEEK_SET);

		    while (prevStdoutLen<stdoutLen) {
			fputc(fgetc(f), stdout);
			prevStdoutLen++;
		    }
		    printf("\n");
		}
		fclose(f);
		
		f=fopen(stderrName, "rb");
		fseek(f, 0, SEEK_END);
		stderrLen=ftell(f);
		if (stderrLen>prevStderrLen) {
		    printf("ERROR: ");
		    fseek(f, prevStderrLen, SEEK_SET);

		    while (prevStderrLen<stderrLen)
		    {
			fputc(fgetc(f), stderr);
			prevStderrLen++;
		    }
		    printf("\n");
		}
		fclose(f);

	    }while ((currentJobState_!=GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED)&&
		    (currentJobState_!=GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE));

	    free(stdoutName);
	    free(stderrName);

	    /*
	     * When job finishes:
	     */
	    /*
	     * Copy output files back to local machine
	     */
	    {
		char *scanArgs[2];
		char *outputFiles;
		char *nextOutputFile;

		char *localOutputFile;
		char *localOutputFile2;
		char *remoteOutputFile;

		scanArgs[0]=remoteJobDir_;
		scanArgs[1]=NULL;

		if (!executeJob(hostname, "/bin/ls", 1, scanArgs))
		{
		    fprintf(stderr, "Error listing output files\n");
		    /* FIXME: maybe retry here? */
		    return 1;
		}

		outputFiles=getLastJobOutput();
		
		nextOutputFile=outputFiles;

		do
		{
		    while ((nextOutputFile[0]!=0)&&(nextOutputFile[0]!='\n'))
		    {
			nextOutputFile++;
		    }
		    if (nextOutputFile[0]==0)
		    {
			break;
		    }
		    nextOutputFile[0]=0;

		    /* Get this file */
		    printf("Retrieving %s...\n", outputFiles);
		    if (asprintf(&localOutputFile, "%s/%s", localJobDir_, outputFiles)<0)
		    {
			fprintf(stderr, "Out of memory\n");
			return 1;
		    }
		    localOutputFile2=relativePathToFullPath(localOutputFile);
		    free(localOutputFile);

		    if (asprintf(&remoteOutputFile, "%s/%s", remoteJobDir_, outputFiles)<0)
		    {
			fprintf(stderr, "Out of memory\n");
			return 1;
		    }

		    if (!copyToLocal_globus(hostname, remoteOutputFile, localOutputFile2))
		    {
			fprintf(stderr, "Error copying back output file %s\n", outputFiles);
		    }
		    free(remoteOutputFile);
		    free(localOutputFile2);

		    nextOutputFile++;
		    outputFiles=nextOutputFile;
		} while (1);		
	    }	    

	    /*
	     * Store any output files to grid if required
	     */
	    for (i=0; i<numFilesToStore_; i++)
	    {
		char *sourceName;
		char *sourceName2;

		printf("Storing output file %s on grid...\n", filesToStore_[i].pfn);

		if (asprintf(&sourceName, "%s/%s", localJobDir_, filesToStore_[i].pfn)<0)
		{
		    fprintf(stderr, "Out of memory\n");
		    return 1;
		}

		sourceName2=relativePathToFullPath(sourceName);
		free(sourceName);

		if (!qcdgridPutFile(sourceName2, filesToStore_[i].lfn, "private"))
		{
		    fprintf(stderr, "Error storing file %s on grid\n", filesToStore_[i].pfn);
		}

		free(sourceName2);
	    }
	}
	else
	{
	    char *jcFilename;
	    FILE *jcFile;

	    /*
	     * Write the job contact string to a file in the job directory so that future
	     * JSS commands can use it
	     */
	    if (asprintf(&jcFilename, "%s/jobcontact", localJobDir_)<0)
	    {
		fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    jcFile=fopen(jcFilename, "w");
	    fprintf(jcFile, "%s\n", jobContact_);
	    fclose(jcFile);
	    free(jcFilename);

	    /*
	     * Print job ID and exit
	     */
	    printf("Job ID: %s\n", localJobDir_);
	}
    }

    return 0;
}
