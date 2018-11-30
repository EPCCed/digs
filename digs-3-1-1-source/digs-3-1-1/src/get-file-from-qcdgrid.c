/***********************************************************************
*
*   Filename:   get-file-from-qcdgrid.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to retrieve a
*               previously stored file from QCDgrid
*
*   Contents:   Main function
*
*   Used in:    Day to day use of the QCDgrid
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

#include <globus_common.h>

#include "qcdgrid-client.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of get-file-from-qcdgrid executable
*    
*   Parameters:                                [I/O]
*
*     argv[1]  logical grid filename            I
*     argv[2]  physical local filename          I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char *localFilename; /* full path name of local file */
    char currentDir[1000];    /* current directory path */
    int result;
    char usage[] = "Usage: digs-get [--recursive] [-R] "
	"<logical grid filename> <file>\n";
    int i;
    int recursive;        /* set if recursive through directories */

    /* Parse switches */
    i = 1;
    recursive = 0;
    while ((i < argc) && (argv[i][0] == '-'))
    {
	if ((!strcmp(argv[i], "--recursive")) || (!strcmp(argv[i], "-R")))
	{
	    argc--;
	    recursive = 1;
	}
	else
	{
	    globus_libc_printf("Unrecognised argument %s\n", argv[i]);
	    globus_libc_printf(usage);
	    return 0;
	}
	i++;
    }

    if (argc < 3) 
    {
	globus_libc_printf(usage);
	return 0;
    }

    /* Remove the switches, leaving the filenames in the expected place */
    argv[1] = argv[i];
    argv[2] = argv[i + 1];

    /* If we haven't got a full local pathname, turn it into one */
    if (argv[2][0] != '/') 
    {
	getcwd(currentDir, 1000);

	localFilename = globus_libc_malloc(strlen(currentDir) +
					   strlen(argv[2]) + 2);
	if (!localFilename) 
	{
	    globus_libc_fprintf(stderr, "Out of memory.\n");
	    return 1;
	}
	sprintf(localFilename, "%s/%s", currentDir, argv[2]);
	argv[2] = localFilename;
    } 
    else 
    {
	localFilename = NULL;
    }

    /* Load grid configuration */
    if (!qcdgridInit(1)) 
    {
	if (localFilename)
	    globus_libc_free(localFilename);
	return 1;
    }
    atexit(qcdgridShutdown);

    if (!recursive)
    {
	result = qcdgridGetFile(argv[1], argv[2]);
    }
    else
    {
	result = qcdgridGetDirectory(argv[1], argv[2]);
    }

    if (localFilename)
	globus_libc_free(localFilename);

    if (result)
    {
	return 0;
    }
    else
    {
	return 1;
    }
}
