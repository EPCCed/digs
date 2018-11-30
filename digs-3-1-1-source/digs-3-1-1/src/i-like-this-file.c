/***********************************************************************
*
*   Filename:   i-like-this-file.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to register
*               interest in having a particular file on the QCDgrid
*               stored locally
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

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of i-like-this-file executable. Sets everything
*   up, then sends the 'touch' message to the main node.
*    
*   Parameters:                                  [I/O]
*
*    (optional)  "-R" or "--recursive" to touch a
*                whole directory recursively      I
*    Logical grid filename to touch               I
*    (Optional) hostname to bring file to         I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char *destination;
    int i;
    char usage[] = "Usage: digs-i-like-this-file [--recursive] [-R] "
	"<logical filename> [<optional hostname>]\n";
    int recursive; /* set if recursing through directories */

    /* Parse arguments */
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

    if ((argc < 2) || (argc > 3))
    {
	globus_libc_printf(usage);
	return 0;
    }

    argv[1] = argv[i];
    if (argc == 3)
    {
	argv[2] = argv[i + 1];
    }

    /* Load grid configuration */
    if (!qcdgridInit(0)) 
    {
	return 1;
    }
    atexit(qcdgridShutdown);

    /* Check the arguments. 2 arguments means the user specified where the file
     * should go. 1 means just use the closest storage element */
    if (argc == 3)
    {
	destination = argv[2];

    }
    else
    {
	destination = NULL;
    }

    /*
     * Implementation has been changed: previously this code used to contain
     * the actual logic of the operation, now that is delegated to the main
     * node. This is so that it could potentially be run by the web interface.
     */
    if (!recursive)
    {
	if (!qcdgridTouchFile(argv[1], destination))
	{
	    return 1;
	}
    }
    else
    {
	if (!qcdgridTouchDirectory(argv[1], destination))
	{
	    return 1;
	}
    }
    return 0;
}
