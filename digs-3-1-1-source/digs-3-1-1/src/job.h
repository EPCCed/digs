/***********************************************************************
*
*   Filename:   job.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Provides a simple interface to Globus job execution
*               to other modules
*
*   Contents:   Function prototypes for this module
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
/*
 * This module deals with remotely executing jobs on machines. There are a
 * number of operations in the running of the grid which require this, for
 * example querying free disk space on storage elements, or updating and
 * retrieving file access times
 */

#ifndef JOB_H
#define JOB_H

/*
 * Runs the executable specified on the remote host. The job name is a full
 * path to the executable. Arguments are passed in in argc and argv (which may
 * be 0 and NULL respectively if there are no arguments) 
 */
int executeJob(char *hostname, char *jobName, int argc, char **argv);

/*
 * Retrieves the output of the last job executed. Note that the storage
 * referenced by the pointer will be overwritten if another job is executed, 
 * so if it is required after this, it should be copied elsewhere. Returns 
 * NULL if no jobs have been executed yet.
 */
char *getLastJobOutput();

#endif
