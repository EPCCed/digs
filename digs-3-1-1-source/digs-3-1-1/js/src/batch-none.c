/***********************************************************************
*
*   Filename:   batch-none.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Handles job submission and status monitoring on systems
*               with no batch system
*
*   Contents:   Submission and status monitoring functions
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

#include <unistd.h>
#include <signal.h>

#include "qcdgrid-js.h"
#include "batch.h"

void error(char *fmt, ...);

int jobPid_;
int jobHasTerminated_;

void sigChildHandler(int num)
{
    jobHasTerminated_=1;
}

int none_SubmitJob(char *jdName)
{
    char *wrapperName;

    /*
     * Setup SIGCHLD handler so we know when the job has
     * finished
     */
    jobHasTerminated_=0;
    signal(SIGCHLD, sigChildHandler);

/*
    printf("Calling globus_libc_fork\n");
    fflush(stdout);
*/
    jobPid_=globus_libc_fork();
    if (jobPid_<0)
    {
/*	printf("Fork error\n");*/
	error("Error forking job\n");
	return 0;
    }
    if (jobPid_==0)
    {
	char **args;
	/*
	 * In child. Run job
	 */
/*
	printf("In child\n");

	error("In child, execing job...\n");
*/	

	args=malloc(3*sizeof(char*));
	if (!args)
	{
	    error("Out of memory\n");
	    return 0;
	}

	if (asprintf(&wrapperName, "%s/qcdgrid-job-wrapper",
		     getJobDescriptionEntry("qcdgrid_path"))<0)
	{
	    error("Out of memory\n");
	    return 0;
	}

	args[0]=wrapperName;
	args[1]=jdName;
	args[2]=NULL;

	if (execv(wrapperName, args)<0)
	{
	    error("Error execing job\n");
	    return 0;
	}
    }

    /*
     * In parent.
     */
/*    printf("In parent\n");*/
    return 1;
}

int none_GetJobStatus()
{
    if (jobHasTerminated_)
    {
	return JOB_FINISHED;
    }
    return JOB_RUNNING;
}
