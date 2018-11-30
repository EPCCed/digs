/***********************************************************************
*
*   Filename:   qcdgrid-job-wrapper.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Wraps the actual job executable, streaming its I/O
*               to and from files
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
/*
 * This program is run on the actual backend system, i.e. where the
 * job itself runs. So we should depend on as few external functions
 * as possible - ideally just standard C library and UNIX calls.
 * It spends most of its time in a 'select' call, waiting for activity
 * on the job's stdout and stderr streams. So it should have pretty
 * lightweight CPU requirements.
 *
 * Roughly speaking, this module is responsible for
 *
 * (1) Providing the job with the right environment (environment
 *     variables, arguments, working directory) in which to run
 * (2) Streaming the job's input, output and error streams to and
 *     from files (where they can be accessed in real time by the
 *     job controller)
 * (3) Saving the job's exit code in a file after it terminates
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "jobdesc.h"

/*
 * Implement asprintf and getline if we don't have the GNU C library
 */
#ifndef __GNU_LIBRARY__

int asprintf (char **ptr, const char *template, ...)
{
    va_list ap;
    char *buffer;
    int size;
    int sizeneeded;

    va_start(ap, template);

    size=1024;
    buffer=malloc(size);
    if (!buffer) return -1;

    sizeneeded=vsnprintf(buffer, size, template, ap);
    while (sizeneeded>size)
    {
	free(buffer);
	size=sizeneeded+1024;
	buffer=malloc(size);
	if (!buffer) return -1;
	
	sizeneeded=vsnprintf(buffer, size, template, ap);
    }


    *ptr=buffer;
    va_end(ap);
    return 0;
}

int getline (char **lineptr, int *n, FILE *stream)
{
    /*
     * This isn't as robust as the GNU version (it truncate lines
     * longer than 1024 characters) but should be good enough for
     * our requirements
     */
    if ((*n==0)||(*lineptr==NULL))
    {
	*n=1024;
	*lineptr=malloc(1024);
    }
    if (!fgets(*lineptr, *n, stream))
    {
	return -1;
    }
    else
    {
	return 0;
    }
}

#endif

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
 * Set by the SIGCHLD handler when the job completes
 */
static volatile int jobHasTerminated_;

/*
 * The time at which the job completed. We keep running for a bit after
 * this to make sure we get all the output from the pipes
 */
static time_t jobFinishedTime_;

/***********************************************************************
*   void sigChildHandler(int num)
*    
*   Handles the SIGCHLD signal, in order to be notified when the job
*   executable completes
*    
*   Parameters:                                        [I/O]
*
*     num  Signal number received (always SIGCHLD)      I
*    
*   Returns: (void)
***********************************************************************/
static void sigChildHandler(int num)
{
    /*
     * This process only has one child, the actual job executable, so
     * when we receive SIGCHLD, it must mean the job has finished
     */
    jobHasTerminated_=1;
    jobFinishedTime_=time(NULL);
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Entry point for the job wrapper. Parses the job description, forks
*   off a process to run the actual job in the correct environment,
*   then loops, streaming input and output between the job and disk
*   files until it completes
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

    FILE *jobStdout;
    FILE *jobStderr;
    FILE *jobStdin;
    int stdinLastLength, stdinLength;
    int numRead;
    static char outputBuffer[1000];

    char *exeName;
    char *directory;

    int pipeForStdout[2];
    int pipeForStderr[2];
    int pipeForStdin[2];
    int maxFd;
    fd_set fds;
    struct timeval timeOut;

    int pid;
    int jobExitStatus;
    FILE *jobExitCode;

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
    directory=getJobDescriptionEntry("directory");
    if (!directory)
    {
	error("No directory specified in job description\n");
	return 1;
    }

    if (chdir(directory)<0)
    {
	error("Error changing to directory %s\n", directory);
	return 1;
    }

    /*
     * Try to create wrapper log file
     */
    errorLogFile_=fopen("wrapper.log", "w");
    if (!errorLogFile_)
    {
	error("Error creating wrapper log file\n");
	return 1;
    }

    /*
     * Try to create stdout and stderr files
     */
    jobStdout=fopen("stdout", "wb");
    jobStderr=fopen("stderr", "wb");
    if ((!jobStdout)||(!jobStderr))
    {
	error("Error creating job stdout and stderr files\n");
	return 1;
    }

    stdinLastLength=0;

    /*
     * Work out what we're trying to run
     */
    exeName=getJobDescriptionEntry("executable");
    if (!exeName)
    {
	error("No executable specified in job description\n");
	return 1;
    }

    /*
     * Create a pipe to connect the standard output of the job
     * to this wrapper
     */
    if (pipe(pipeForStdout)<0)
    {
	error("Error creating pipe for standard output\n");
	return 1;
    }
    
    /*
     * Create a pipe to connect the standard error of the job to
     * this wrapper
     */
    if (pipe(pipeForStderr)<0)
    {
	error("Error creating pipe for standard error\n");
	return 1;
    }
    
    /*
     * Create a pipe to connect the standard input of the job to
     * this wrapper
     */
    if (pipe(pipeForStdin)<0)
    {
	error("Error creating pipe for standard input\n");
	return 1;
    }

    /*
     * Setup SIGCHLD handler so we know when the job has
     * finished
     */
    jobHasTerminated_=0;
    signal(SIGCHLD, sigChildHandler);

    /*
     * Fork: child process runs actual job, parent process monitors
     * its output etc. and sends it back to the submitter
     */
    pid=fork();
    if (pid<0)
    {
	error("Error forking\n");
	return 1;
    }
    if (pid==0)
    {
	/*
	 * In child process
	 */
	char **args;
	int jobargc;
	char *a;

	/*
	 * Redirect stdout into the pipe
	 */
	close(pipeForStdout[0]);
	dup2(pipeForStdout[1], STDOUT_FILENO);
	
	/*
	 * Redirect stderr into the pipe
	 */
	close(pipeForStderr[0]);
	dup2(pipeForStderr[1], STDERR_FILENO);
	
	/*
	 * Redirect stdin from pipe
	 */
	close(pipeForStdin[1]);
	dup2(pipeForStdin[0], STDIN_FILENO);

	/*
	 * Not sure if this actually makes any difference, it may solve
	 * the 'stdout needs to be flushed' problem. Shouldn't hurt
	 * anyway.
	 */	
	if (fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL, 0)|O_SYNC)<0)
	{
	    error("Can't set stdout to synchronous\n");
	}

	/*
	 * Build argument list for exec
	 */
	jobargc=1;
	a=getJobDescriptionEntry("arg");
	while (a)
	{
	    /* count args to know what size of array to alloc */
	    jobargc++;
	    a=getNextJobDescriptionEntry("arg");
	}
	args=malloc((jobargc+1)*sizeof(char*));
	if (!args)
	{
	    error("Out of memory\n");
	    return 1;
	}
	args[0]=strdup(exeName);
	a=getJobDescriptionEntry("arg");
	i=1;
	while (i<jobargc)
	{
	    args[i]=strdup(a);
	    a=getNextJobDescriptionEntry("arg");
	    i++;
	}
	args[i]=NULL;

	/*
	 * Setup environment for job
	 */
	a=getJobDescriptionEntry("env");
	while (a)
	{
	    char *val;

	    /*
	     * parse each 'env' entry into a variable name
	     * and a value, separated by an equals sign
	     */
	    a=strdup(a);
	    val=strchr(a, '=');
	    if (val)
	    {
		*val=0;
		val++;
		setenv(a, val, 1);
	    }
	    else
	    {
		/*
		 * in the case of no '=', clear the variable
		 */
		setenv(a, "", 1);
	    }

	    free(a);
	    a=getNextJobDescriptionEntry("env");
	}

	/*
	 * Actually start the job executable
	 */
	if (execv(exeName, args)<0)
	{
	    error("Error execing\n");
	    return 1;
	}
	error("How did I end up here??\n");
	return 0;
    }

    /*
     * Close unnecessary ends of the pipes
     */
    close(pipeForStdout[1]);
    close(pipeForStderr[1]);
    close(pipeForStdin[0]);

    /*
     * Work out max file descriptor number to wait on, for select call
     */
    maxFd=pipeForStdout[0];
    if (pipeForStderr[0]>maxFd)
    {
	maxFd=pipeForStderr[0];
    }
    
    /*
     * Keep looping, reading job's stdout and stderr streams, until it
     * terminates
     */
    /*
     * We wait an extra 1.5 seconds after the job terminates to make sure we get
     * all of its output before closing everything down. This is a bit of an
     * arbitrary time limit - would be better if there was some way to be sure
     * we had all the output.
     */
    while ((!jobHasTerminated_)||(difftime(time(NULL), jobFinishedTime_)<1.5))
    {
	FD_ZERO(&fds);
	FD_SET(pipeForStdout[0], &fds);
	FD_SET(pipeForStderr[0], &fds);
	
	/*
	 * Half a second time out on select. This allows us to periodically
	 * check for job termination or input even if no output is coming
	 */
	timeOut.tv_sec=0;
	timeOut.tv_usec=500000;

	/*
	 * Check both stdout and stderr pipes for activity
	 */
	if (select(maxFd+1, &fds, NULL, NULL, &timeOut)<0)
	{
	    error("Error in select call\n");
	    return 1;
	}

	/*
	 * Check for activity on stdout
	 */
	if (FD_ISSET(pipeForStdout[0], &fds))
	{
	    do
	    {
		numRead=read(pipeForStdout[0], outputBuffer, 999);
		if (numRead>0)
		{
		    fwrite(outputBuffer, numRead, 1, jobStdout);
		    fflush(jobStdout);
		}
	    } while (numRead>=999);
	}

	/*
	 * Check for activity on stderr
	 */
	if (FD_ISSET(pipeForStderr[0], &fds))
	{
	    do
	    {
		numRead=read(pipeForStderr[0], outputBuffer, 999);
		if (numRead>0)
		{
		    fwrite(outputBuffer, numRead, 1, jobStderr);
		    fflush(jobStderr);
		}
	    } while (numRead>=999);
	}

	/*
	 * Check for updated input file
	 */
	/* FIXME: file locking...?*/
	jobStdin=fopen("stdin", "rb");
	if (jobStdin)
	{
	    fseek(jobStdin, 0, SEEK_END);
	    stdinLength=ftell(jobStdin);
	    if (stdinLength!=stdinLastLength)
	    {
		fseek(jobStdin, stdinLastLength, SEEK_SET);
		while (stdinLastLength<stdinLength)
		{
		    int l;
		    l=stdinLength-stdinLastLength;
		    if (l>999)
		    {
			l=999;
		    }
		    fread(outputBuffer, l, 1, jobStdin);
		    write(pipeForStdin[1], outputBuffer, l);
		    stdinLastLength+=l;
		}
	    }
	    fclose(jobStdin);
	}
    }

    /*
     * Get the job's exit status
     */
    waitpid(pid, &jobExitStatus, 0);

    /*
     * and save it for the submission software to send to the user
     */
    jobExitCode=fopen("exitcode", "w");
    if (!jobExitCode)
    {
	error("Can't open exitcode file\n");
    }
    else
    {
	fprintf(jobExitCode, "%d\n", jobExitStatus);
	fclose(jobExitCode);
    }

    fclose(errorLogFile_);
    fclose(jobStdout);
    fclose(jobStderr);

    return 0;
}
