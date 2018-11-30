/***********************************************************************
*
*   Filename:   job.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Provides a simple interface to Globus job execution
*               to other modules
*
*   Contents:   Functions to execute a job remotely and retrieve its
*               output
*
*   Used in:    Called by other modules for executing programs on remote
*               machines
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
#include <time.h>

#include <unistd.h>

#include <globus_gram_client.h>
#include <globus_gass_server_ez.h>

#include "job.h"
#include "node.h"
#include "misc.h"

/* maximum number of concurrent jobs on the same host */
#define MAX_HOST_JOBS 5

#if 0
/***********************************************************************
*   const char *getJobStateString(int state)
*
*   Returns a string representation of a Globus GRAM state code (for use
*   in error messages etc.)
*    
*   Parameters:                                                    [I/O]
*
*     state   Globus GRAM state code                                I
*    
*   Returns: string representing the state. Do not modify or free!
***********************************************************************/
static const char *getJobStateString(int state)
{
    char *result;
    switch (state)
    {
    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_PENDING:
	result = "State pending";
	break;
    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_ACTIVE:
	result = "State active";
	break;
    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED:
	result = "State failed";
	break;
    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE:
	result = "State done";
	break;
    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_SUSPENDED:
	result = "State suspended";
	break;
    case GLOBUS_GRAM_PROTOCOL_JOB_STATE_UNSUBMITTED:
	result = "State unsubmitted";
	break;
    default:
	result = "Unknown job state";
	break;
    }
    return result;
}
#endif

/*
 * Structure to represent a running GRAM job
 */
typedef struct qcdgridGramJob_s
{
    /* unique ID number for this job */
    int id;

    /* FQDN of host which this job is running on */
    char *host;

    /* RSL for the job */
    char *rsl;

    /* current state of job */
    int state;

    /* error code of job (if state FAILED) */
    int error;

    /* set if abandoned due to timeout */
    int abandoned;

    /* NULL if not set yet */
    char *callbackContact;

    /* local output filename */
    char *outputName;

    /* GASS server for output */
    globus_gass_transfer_listener_t gassListener;

    /* pointer to next job in linked list */
    struct qcdgridGramJob_s *next;
} qcdgridGramJob_t;

/*
 * set after the one-time initialisation for the module is completed
 */
static int qcdgridGramInitialised_ = 0;

/*
 * start of the linked list of jobs in progress
 */
static qcdgridGramJob_t *jobListHead_;

/*
 * mutex to protect job list from concurrent accesses
 */
static globus_mutex_t jobListLock_;

/*
 * next job ID value to use
 */
static int jobId_;

/***********************************************************************
*   void deleteGramJob(qcdgridGramJob_t *job)
*
*   Removes a job structure from the list and frees it
*    
*   Parameters:                                                    [I/O]
*
*     job   pointer to job structure                                I
*    
*   Returns: (void)
***********************************************************************/
static void deleteGramJob(qcdgridGramJob_t *job)
{
    qcdgridGramJob_t *p;

    logMessage(1, "deleteGramJob(%d)", job->id);

    globus_mutex_lock(&jobListLock_);
    if (job == jobListHead_)
    {
	jobListHead_ = job->next;
    }
    else
    {
	p = jobListHead_;
	while ((p != NULL) && (p->next != job))
	{
	    p = p->next;
	}
	if (p->next == job)
	{
	    p->next = job->next;
	}
    }
    globus_mutex_unlock(&jobListLock_);

    globus_libc_free(job->host);
    globus_libc_free(job->rsl);

    if (job->outputName)
    {
	globus_libc_free(job->outputName);
    }

    globus_libc_free(job);
}

/***********************************************************************
*   qcdgridGramJob_t *newGramJob(char *host, char *rsl)
*
*   Create a new GRAM job structure and link it to the list. Don't
*   actually start the job yet though
*    
*   Parameters:                                                    [I/O]
*
*     host    hostname to run job on                                I
*     rsl     resource specification language for job               I
*    
*   Returns: pointer to structure describing job
***********************************************************************/
static qcdgridGramJob_t *newGramJob(char *host, char *rsl)
{
    qcdgridGramJob_t *job;

    logMessage(1, "newGramJob(%s,%s)", host, rsl);

    /* allocate structure */
    job = globus_libc_malloc(sizeof(qcdgridGramJob_t));
    if (job == NULL)
    {
	errorExit("Out of memory in newGramJob");
    }

    job->host = safe_strdup(host);
    job->rsl = safe_strdup(rsl);
    if ((job->host == NULL) || (job->rsl == NULL))
    {
	errorExit("Out of memory in newGramJob");
    }

    /* assume pending and no error */
    job->error = -1;
    job->state = GLOBUS_GRAM_PROTOCOL_JOB_STATE_PENDING;
    job->abandoned = 0;

    /* set job ID to a unique integer */
    job->id = jobId_;
    jobId_++;

    /* no contact or output yet */
    job->outputName = NULL;
    job->callbackContact = NULL;
    job->gassListener = GLOBUS_NULL;

    /* link onto list */
    globus_mutex_lock(&jobListLock_);
    job->next = jobListHead_;
    jobListHead_ = job;
    globus_mutex_unlock(&jobListLock_);

    return job;
}

/***********************************************************************
*   int numHostJobs(char *host)
*
*   Returns the number of jobs currently running on the specified host
*    
*   Parameters:                                                    [I/O]
*
*     host    hostname to check                                     I
*    
*   Returns: number of jobs
***********************************************************************/
static int numHostJobs(char *host)
{
    qcdgridGramJob_t *job;
    int count = 0;

    globus_mutex_lock(&jobListLock_);
    job = jobListHead_;

    while (job != NULL)
    {
	if (!strcmp(host, job->host))
	{
	    count++;
	}
	job = job->next;
    }

    globus_mutex_unlock(&jobListLock_);
    return count;
}

/***********************************************************************
* void callback2(void *				user_callback_arg,
*	         globus_gram_protocol_error_t	operation_failure_code,
*	         const char *			job_contact,
*	         globus_gram_protocol_job_state_t	job_state,
*	         globus_gram_protocol_error_t	job_failure_code)
*
* Globus callback for register job request function. Not currently
* needed in QCDgrid
*    
* Parameters:                                                    [I/O]
*
*   (none used)
*    
* Returns: (void)
***********************************************************************/
static void callback2(void *			   user_callback_arg,
		      globus_gram_protocol_error_t operation_failure_code,
		      const char *		   job_contact,
		      globus_gram_protocol_job_state_t	job_state,
		      globus_gram_protocol_error_t job_failure_code)
{

}

/***********************************************************************
*   int initialiseQcdgridGram()
*
*   Performs one-time initialisation of the GRAM module
*    
*   Parameters:                                                    [I/O]
*
*     (none)
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int initialiseQcdgridGram()
{
    logMessage(1, "initialiseQcdgridGram()");

    jobListHead_ = NULL;

    jobId_ = 1;

    if (globus_mutex_init(&jobListLock_, NULL))
    {
	logMessage(5, "Error initialising job list mutex");
	return 0;
    }

    qcdgridGramInitialised_ = 1;

    return 1;
}

/* Points to the output from the last job that was run (always saved until
   another job is executed */
static char *lastJobOutput_=NULL;

/***********************************************************************
*   char *generateRsl(char *jobName, int argc, char **argv,
*                     char *stdoutput, char *extra)
*
*   Converts the executable path and argument list into a Globus
*   resource specification language string    
*    
*   Parameters:                                                    [I/O]
*
*     jobName   Full path to executable                             I
*     argc      Number of arguments (doesn't include exec name)     I
*     argv      Vector of arguments to pass (maybe NULL if argc=0)  I
*     stdoutput Filename to store job output in (maybe NULL). May
*               also be a URL on a GASS server                      I
*     extra     Extra RSL string to add. May be NULL                I
*    
*   Returns: Pointer to dynamically allocated RSL string,
*            or NULL on error
***********************************************************************/
static char *generateRsl(char *jobName, int argc, char **argv,
			 char *stdoutput, char *extra)
{
    char *buffer;
    int length;
    int i;

    logMessage(1, "generateRsl(%s,%d,%s,%s)", jobName, argc, stdoutput,
	       extra);

    /* First need to know how long a string to allocate */
    length = strlen("&(executable=)(arguments=)(environment=(LD_LIBRARY_PATH "
		    "/opt/globus/lib:/Home/jamesp/qcdgrid))");
    if (extra)
    {
	length += strlen(extra);
    }
    for (i = 0; i < argc; i++)
    {
	length += strlen(argv[i]) + 4;
    }
    length += strlen(jobName);
    if (stdoutput)
    {
	length += strlen(stdoutput);
	length += strlen("(stdout=)");
    }
    length += 10;
   
    /* Allocate storage for RSL */
    buffer = globus_libc_malloc(length);
    if (!buffer)
    {
	errorExit("Out of memory in generateRsl");
    }
    
    /* Do basic executable string */
    sprintf(buffer, "&(executable=%s)", jobName);

    /* Add arguments if any */
    if (argc)
    {
	strcat(buffer, "(arguments=\"");
	for (i = 0; i < argc; i++)
	{
	    strcat(buffer, argv[i]);
	    if (i != (argc-1)) strcat(buffer, "\" \"");
	    else strcat(buffer, "\")");
	}
    }

    if (extra)
    {
	strcat(buffer, extra);
    }

    /* Add stdout */
    if (stdoutput)
    {
	strcat(buffer, "(stdout=");
	strcat(buffer, stdoutput); 
	strcat(buffer, ")");
    }

    return buffer;
}

/***********************************************************************
*   void globusJobCallback(void *userArg, char *jobContact, int state,
*                          int error)
*    
*   Callback function used by Globus to notify us of state changes
*   of the job
*    
*   Parameters:                                                    [I/O]
*
*     userArg     User defined argument. Not used                   I
*     jobContact  Contact string for job. Not used                  I
*     state       Job's current state                               I
*     error       Error code, if job's state is FAILED. Not used    I
*    
*   Returns: (void)
***********************************************************************/
static void globusJobCallback(void *userArg, char *jobContact,
			      int state, int error)
{
    qcdgridGramJob_t *job, *p;
    int found;

    logMessage(1, "globusJobCallback %d,%d", state, error);

    job = (qcdgridGramJob_t *)userArg;

    globus_mutex_lock(&jobListLock_);

    /* quick safety/sanity check that job is in the list */
    found = 0;
    p = jobListHead_;
    while (p)
    {
	if (job == p)
	{
	    found = 1;
	}
	p = p->next;
    }
    if (found == 0)
    {
	logMessage(3, "Extraneous globusJobCallback detected!");
	globus_mutex_unlock(&jobListLock_);
	return;
    }

    job->state = state;
    job->error = error;

    /*
     * If the monitor thread's abandoned this job, we have to clean
     * up after it
     */
    if ((job->abandoned) &&
	((state == GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED) ||
	 (state == GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE)))
    {
	logMessage(3, "Finally cleaning up timed-out job %d", job->id);
	
	unlink(job->outputName);
	globus_gass_server_ez_shutdown(job->gassListener);

	globus_mutex_unlock(&jobListLock_);
	globus_gram_client_callback_disallow(job->callbackContact);
	deleteGramJob(job);
	return;
    }

    globus_mutex_unlock(&jobListLock_);
}

/***********************************************************************
*   int executeJob(char *hostname, char *jobName, int argc, char **argv)
*    
*   Runs an executable on a remote host, using Globus
*    
*   Parameters:                                                    [I/O]
*
*     hostname  FQDN of host to run on                              I
*     jobName   Full path to executable                             I
*     argc      Number of arguments (doesn't include exec name)     I
*     argv      Vector of arguments to pass (maybe NULL if argc=0)  I
*    
*   Returns: 1 for success, 0 on error
***********************************************************************/
int executeJob(char *hostname, char *jobName, int argc, char **argv)
{
    char *rsl;
    int fd;
    FILE *f;
    int len;
    int err;
    time_t startTime;
    time_t endTime;
    char *contactString;
    char *extraContact;
    float timeOut;
    qcdgridGramJob_t *job;

    char *gassUrl = NULL;
    char *remoteJobOutputName;
    char *localJobOutputFile;

    globus_gass_transfer_listener_t gassListener;
    globus_gass_transfer_listenerattr_t * gassAttr = GLOBUS_NULL;
    globus_gass_transfer_requestattr_t * gassReqAttr = GLOBUS_NULL;
    char *gassScheme = NULL;

    logMessage(1, "executeJob(%s,%s)", hostname, jobName);

    /* initialise the GRAM system first time we're called */
    if (!qcdgridGramInitialised_)
    {
	if (!initialiseQcdgridGram())
	{
	    return 0;
	}
    }

    /* don't allow too many concurrent jobs on same system */
    if (numHostJobs(hostname) >= MAX_HOST_JOBS)
    {
	logMessage(3, "Host %s has too many jobs running already", hostname);
	return 0;
    }

    /* Create a file for the output */
    if (safe_asprintf(&localJobOutputFile, "%s/qcdgridjoboutXXXXXX", tmpDir_) < 0)
    {
	errorExit("Out of memory in executeJob");
    }
    fd = mkstemp(localJobOutputFile);
    if (fd < 0) 
    {
	logMessage(3, "mkstemp failed in executeJob");
	globus_libc_free(localJobOutputFile);
	return 0;
    }
    close(fd);
    unlink(localJobOutputFile);

    /* setup GASS server for output */
    err = globus_gass_server_ez_init(&gassListener,
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
    if (err != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error creating GASS server");
	globus_libc_free(localJobOutputFile);
	return 0;
    }
    gassUrl = globus_gass_transfer_listener_get_base_url(gassListener);

    if (safe_asprintf(&remoteJobOutputName, "%s%s", gassUrl, localJobOutputFile) < 0)
    {
	errorExit("Out of memory in executeJob");
    }

    /* generate RSL for job */
    rsl = generateRsl(jobName, argc, argv, remoteJobOutputName,
		      getNodeExtraRsl(hostname));
    if (!rsl)
    {
	globus_libc_free(localJobOutputFile);
	globus_libc_free(remoteJobOutputName);
	globus_gass_server_ez_shutdown(gassListener);
	return 0;
    }
    globus_libc_free(remoteJobOutputName);

    /* create job structure and link onto the list */
    job = newGramJob(hostname, rsl);
    if (!job)
    {
	globus_libc_free(localJobOutputFile);
	globus_gass_server_ez_shutdown(gassListener);
	globus_libc_free(rsl);
	return 0;
    }
    globus_libc_free(rsl);

    /* lock job list mutex until we enter loop now */
    globus_mutex_lock(&jobListLock_);

    job->outputName = localJobOutputFile;
    job->gassListener = gassListener;

    /* Open a port to listen for job state info */
    err = globus_gram_client_callback_allow(globusJobCallback, (void *)job, &job->callbackContact);
    if (err != GLOBUS_SUCCESS)
    {
	logMessage(3, "(job.c) Globus error (1): %s",
		   globus_gram_client_error_string(err));
    }

    if (job->callbackContact == NULL)
    {
	unlink(job->outputName);
	globus_gass_server_ez_shutdown(gassListener);
	globus_mutex_unlock(&jobListLock_);
	deleteGramJob(job);
	return 0;
    }

    /* handle extra JSS contact info */
    extraContact = getNodeExtraJssContact(hostname);
    if (extraContact != NULL)
    {
	contactString = globus_libc_malloc(strlen(hostname) + strlen(extraContact) + 1);
	if (!contactString) errorExit("Out of memory in executeJob");
	sprintf(contactString, "%s%s", hostname, extraContact);
    }
    else
    {
	contactString = safe_strdup(hostname);
	if (!contactString) errorExit("Out of memory in executeJob");
    }

    /* Start the job running */
    err = globus_gram_client_register_job_request(contactString, job->rsl, GLOBUS_GRAM_PROTOCOL_JOB_STATE_ALL,
						  job->callbackContact, GLOBUS_GRAM_CLIENT_NO_ATTR, callback2,
						  (void *)job);
    if (err != GLOBUS_SUCCESS)
    {
	logMessage(3, "(job.c) Globus error (2): %s", globus_gram_client_error_string(err));
	globus_libc_free(contactString);
	unlink(job->outputName);
	globus_gass_server_ez_shutdown(gassListener);

	globus_mutex_unlock(&jobListLock_);
	globus_gram_client_callback_disallow(job->callbackContact);
	deleteGramJob(job);

	return 0;
    }
    globus_libc_free(contactString);

    /* Give the job a configurable amount of time before we assume it's gone pear shaped */
    startTime = time(NULL);

    timeOut = getNodeJobTimeout(hostname);
    if (timeOut < 0.0) {
	timeOut = 30.0;
    }

    /* Wait for it to terminate */
    while (job->state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE)
    {
	globus_mutex_unlock(&jobListLock_);
	globus_thread_yield();
	globus_mutex_lock(&jobListLock_);

	if (job->state == GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED)
	{
	    logMessage(3, "Globus error: %s", globus_gram_client_error_string(job->error));

	    globus_gass_server_ez_shutdown(gassListener);
	    unlink(job->outputName);

	    globus_mutex_unlock(&jobListLock_);
	    globus_gram_client_callback_disallow(job->callbackContact);
	    deleteGramJob(job);
	    return 0;
	}

	endTime = time(NULL);
	if (difftime(endTime, startTime) > timeOut)
	{
	    /* callback will deal with it when it eventually completes */
	    job->abandoned = 1;
	    globus_mutex_unlock(&jobListLock_);

	    logMessage(3, "Job submission timed out");
	    return 0;
	}
    }

    globus_mutex_unlock(&jobListLock_);
    globus_gram_client_callback_disallow(job->callbackContact);
    globus_gass_server_ez_shutdown(gassListener);
    globus_mutex_lock(&jobListLock_);

    /* Read in the output and keep it for later */
    if (lastJobOutput_) globus_libc_free(lastJobOutput_);
    lastJobOutput_ = NULL;
    f = fopen(job->outputName, "rb");
    if (!f)
    {
	logMessage(3, "Job output file %s does not exist!", job->outputName);
	globus_mutex_unlock(&jobListLock_);
	deleteGramJob(job);
	return 0;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    lastJobOutput_ = globus_libc_malloc(len+1);
    if (!lastJobOutput_) 
    {
	errorExit("Out of memory in executeJob");
    }
    fread(lastJobOutput_, len, 1, f);
    fclose(f);
    lastJobOutput_[len]=0;

    unlink(job->outputName);
    globus_mutex_unlock(&jobListLock_);
    deleteGramJob(job);

    return 1;
}

/***********************************************************************
*   char *getLastJobOutput()
*    
*   Gets the output from the last job that executed. Should not be
*   altered or freed - duplicate the string returned if you need to do
*   that
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: pointer to output of last job, or NULL if no jobs have
*            successfully executed yet
***********************************************************************/
char *getLastJobOutput()
{
    return lastJobOutput_;
}
