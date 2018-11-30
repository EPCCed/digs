/***********************************************************************
*
*   Filename:   batch-pbs.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Interfaces QCDgrid job submission to PBS queues
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

#include "qcdgrid-js.h"
#include "batch.h"

extern char *jobDirectory_;

void error(char *fmt, ...);

int pbs_SubmitJob(char *jdName)
{
    FILE *pbsScript;
    char *pbsCmd;

    pbsScript=fopen("pbs.sh", "w");
    if (!pbsScript)
    {
	error("Unable to create pbs.sh script\n");
	return 0;
    }

    fprintf(pbsScript, "#!/bin/sh\n\n%s/qcdgrid-job-wrapper %s\ntouch %s/pbsdone\n",
	    getJobDescriptionEntry("qcdgrid_path"), jdName, jobDirectory_);
    fclose(pbsScript);

    if (asprintf(&pbsCmd, "%s/qsub pbs.sh > pbsid", 
		 getJobDescriptionEntry("pbs_path"))<0)
    {
	error("Out of memory\n");
	return 0;
    }

    system(pbsCmd);
    free(pbsCmd);

    return 1;
}

int pbs_GetJobStatus()
{
    FILE *f;
    char *lineBuffer=NULL;
    int lineBufferLength=0;
    char *pbsid;
    int st;
    int i;

    f=fopen("pbsdone", "r");
    if (f)
    {
	st=JOB_FINISHED;
	fclose(f);
    }
    else
    {
	st=JOB_QUEUED;
    }

    /* Read the PBS ID from its file */
    f=fopen("pbsid", "r");
    if (!f)
    {
	error("Unable to open pbsid file\n");
	return -1;
    }
    getline(&lineBuffer, &lineBufferLength, f);
    if (!lineBuffer)
    {
	error("Error reading pbsid file\n");
	return -1;
    }
    fclose(f);

    /* Strip off any carriage return or line feed */
    i=0;
    while ((lineBuffer[i])&&(lineBuffer[i]!=0xa)&&(lineBuffer[i]!=0xd))
    {
	i++;
    }
    lineBuffer[i]=0;
    pbsid=strdup(lineBuffer);

    /* Get the PBS queue status */
    /* FIXME: qstat path */
    system("/usr/pbs/bin/qstat > pbsstatus");
    
    f=fopen("pbsstatus", "r");
    if (!f)
    {
	error("Unable to open pbsstatus file\n");
	return -1;
    }

    while (!feof(f))
    {
	if (getline(&lineBuffer, &lineBufferLength, f)<0)
	{
	    break;
	}
	if (!strncmp(lineBuffer, pbsid, strlen(pbsid)))
	{
	    /* Found the line containing this job */
	    /* FIXME: parse the line properly, distinguish other states */
	    st=JOB_RUNNING;
	}
    }
    fclose(f);
    if (lineBuffer)
    {
	free(lineBuffer);
    }
    free(pbsid);

    return st;
}
