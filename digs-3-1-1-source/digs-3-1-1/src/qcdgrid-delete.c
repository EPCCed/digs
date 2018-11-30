/***********************************************************************
*
*   Filename:   qcdgrid-delete.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to delete a file
*               from the QCD grid
*
*   Contents:   Main function
*
*   Used in:    Day to day usage of QCDgrid
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

#include <globus_common.h>

#include "qcdgrid-client.h"
#include "misc.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of qcdgrid-delete executable. Sets everything
*   up, then sends a message to the main node
*    
*   Parameters:                                            [I/O]
*
*     (optional) "-R" or "--recursive" to delete a whole    I
*                directory tree recursively
*     Logical filename of file to delete                    I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char usage[] = "Usage: digs-delete [--recursive] [-R] filename>\n";
    int i;
    char response;
    int recursive;

    char *ssLFN;

    /* Handle command line switches */
    recursive = 0;
    i = 1;
    while ((i < argc) && (argv[i][0] == '-'))
    {
	if ((!strcmp(argv[i], "--recursive")) || (!strcmp(argv[i], "-R")))
	{
	    recursive = 1;
	}
	else
	{
	    globus_libc_printf("Unrecognised argument %s\n", argv[i]);
	    globus_libc_printf(usage);
	    return 0;
	}

	i++;
	argc--;
    }

    if (argc != 2) 
    {
	globus_libc_printf(usage);
	return 0;
    }

//RADEK i think here should substitute slashes

    ssLFN = substituteChars(argv[i], "//", "/");
    //argv[1] = argv[i];


    if (!qcdgridInit(0)) 
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	globus_libc_free(ssLFN);
	return 1;
    }
    atexit(qcdgridShutdown);

    if (!recursive)
    {
	if (!qcdgridDeleteFile(ssLFN))
	{
	    globus_libc_free(ssLFN);
	    return 1;
	}
    }
    else
    {
	/*
	 * Better ask for confirmation: recursive delete could be used to wipe
	 * the entire grid in one single command
	 */
	globus_libc_printf("This will delete all logical files under %s\n", ssLFN);
	globus_libc_printf("Are you sure you want to continue?\n");
	response = fgetc(stdin);

	if ((response != 'y') && (response != 'Y'))
	{
	    globus_libc_free(ssLFN);
	    return 0;
	}

	if (!qcdgridDeleteDirectory(ssLFN))
	{
	    globus_libc_free(ssLFN);
	    return 1;
	}
    }

    return 0;
}
