/***********************************************************************
*
*   Filename:   batch.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Handles submission of QCDgrid jobs to various batch
*               systems
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

void error(char *fmt, ...);

int none_SubmitJob(char *jdName);
int pbs_SubmitJob(char *jdName);

int none_GetJobStatus();
int pbs_GetJobStatus();

/*
 * The type of system installed on this machine
 */
/*
 * FIXME: make this configurable
 */
int batchSystemType_=BATCH_NONE;


/*
 * Main submission function
 */
int submitJob(char *jdName)
{
    switch (batchSystemType_)
    {
    case BATCH_NONE:
	return none_SubmitJob(jdName);
	break;
    case BATCH_PBS:
	return pbs_SubmitJob(jdName);
	break;
    default:
	error("Unknown batch system %d\n", batchSystemType_);
	break;
    } 
    return 0;
}

/*
 * Main status function
 */
int getJobStatus()
{
    switch (batchSystemType_)
    {
    case BATCH_NONE:
	return none_GetJobStatus();
	break;
    case BATCH_PBS:
	return pbs_GetJobStatus();
	break;
    default:
	error("Unknown batch system %d\n", batchSystemType_);
	break;
    }
    return -1;
}

void setBatchSystem(char *name)
{
    if (!strcmp(name, "none"))
    {
	batchSystemType_=BATCH_NONE;
    }
    else if (!strcmp(name, "pbs"))
    {
	batchSystemType_=BATCH_PBS;
    }
    else
    {
	fprintf(stderr, "Warning: unknown batch system `%s' specified, defaulting to none\n", name);
	batchSystemType_=BATCH_NONE;
    }
}
