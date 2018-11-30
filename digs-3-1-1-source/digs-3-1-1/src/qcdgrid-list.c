/***********************************************************************
*
*   Filename:   qcdgrid-list.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*               Daragh Byrne           (daragh)   EPCC.
*
*   Purpose:    Command to list files on the QCDgrid
*
*   Contents:   Main function
*
*   Used in:    Day to day use of QCDgrid
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003-2007 The University of Edinburgh
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

#include "qcdgrid-client.h"
#include "replica.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of list command
*    
*   Parameters:
*
*     Optional parameter '-n' give number of copies of each file
*     Optional parameter '-l' gives locations of the file.
*     Optional filename or pattern narrows listing
*
*   Returns: 0 on success, 1 on error
*            Prints a list of files to standard output
***********************************************************************/
int main(int argc, char *argv[])
{
    int i, j;
    int showNumCopies = 0;
    int showLocations = 0;
    char* wildcard = "*";

    logicalFileInfo_t *list;
    int numFiles;

    time_t start, end;

    /* Parse command line arguments */
    for (i = 1; i < argc; i++)
    {
	if (strcmp(argv[i], "-n") == 0)
	{
	    showNumCopies = 1;
	}
        else if( strcmp( argv[i], "-l" ) == 0 )
        {
            showLocations = 1;
        }
	else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
	{
	    /* show usage */
	    globus_libc_printf("Usage: qcdgrid-list [-n | -l | -h | --help] [<wildcard>]\n");
	    globus_libc_printf("  -n: show number of copies of each file\n");
	    globus_libc_printf("  -l: show locations of copies of each file\n");
	    globus_libc_printf("  -h or --help: show this usage information\n");
	    return 0;
	}
	else
	{
	    wildcard = argv[i];
	}
    }

    if (!qcdgridInit(1))
    {
	globus_libc_fprintf(stderr, "Error loading config\n");
	return 1;
    }
    atexit(qcdgridShutdown);

    list = getFileList(wildcard, &numFiles);
    if (list == NULL)
    {
	globus_libc_printf(stderr, "Error listing files\n");
	return 1;
    }

    for (i = 0; i < numFiles; i++)
    {
	if (showNumCopies)
	{
	    globus_libc_printf("%s (%d) ", list[i].lfn, list[i].numPfns);
	}
	else
	{
	    globus_libc_printf("%s ", list[i].lfn);
	}
	if (showLocations)
	{
	    for (j = 0; j < list[i].numPfns; j++)
	    {
		globus_libc_printf("%s ", getNodeName(list[i].pfns[j]));
	    }
	}
	globus_libc_printf("\n");
    }

    freeFileList(list, numFiles);

    return 0;
}
