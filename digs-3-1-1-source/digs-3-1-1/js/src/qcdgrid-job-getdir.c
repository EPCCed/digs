/***********************************************************************
*
*   Filename:   qcdgrid-job-getdir.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Creates a new directory for storing files related to
*               a job on a remote machine
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

#include <sys/stat.h>

int main(int argc, char *argv[])
{
    char *dirName;
    char *parentDir;
    unsigned int num;

    if (argc!=2)
    {
	printf("This is an internal part of the QCDgrid job submission software\n");
	printf("It should not be run directly except for testing purposes\n");
	printf("(Usage: qcdgrid-job-getdir <parent directory>)\n");
	return 0;
    }

    /*
     * UNKNOWN means no special temp directory specified, so we should try
     * to determine one here
     */
    if (!strcmp(argv[1], "UNKNOWN"))
    {
	/*
	 * Try the user's home directory
	 */
	parentDir=getenv("HOME");

	/*
	 * Try to ensure it's valid
	 */
	if ((parentDir==NULL)||(strlen(parentDir)==0)||(parentDir[0]!='/'))
	{
	    /* Fall back to /tmp if all else fails */
	    parentDir="/tmp";
	}
    }
    else
    {
	parentDir=argv[1];
    }

    dirName=malloc(strlen(argv[1])+strlen("/qcdgridjob000000")+2);
    if (!dirName)
    {
	fprintf(stderr, "Out of memory\n");
	return 1;
    }

    num=0;
    do
    {
	num++;
	sprintf(dirName, "%s/qcdgridjob%06d", argv[1], num);
	if (num==1000000)
	{
	    fprintf(stderr, "Too many job results already\n");
	    return 1;
	}
    }
    while (mkdir(dirName, S_IRWXU)<0);

    printf(dirName);
    free(dirName);
    return 0;
}
