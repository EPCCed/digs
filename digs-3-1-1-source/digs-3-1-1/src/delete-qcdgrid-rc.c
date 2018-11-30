/***********************************************************************
*
*   Filename:   delete-qcdgrid-rc.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Completely removes the QCDgrid replica catalogue
*
*   Contents:   Main function, routines specific to this command
*
*   Used in:    Administration of QCDgrid
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

#include "misc.h"
#include "node.h"
#include "replica.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of replica catalogue deleting executable
*    
*   Parameters:                                                  [I/O]
*
*     (optional) hostname to delete catalogue for - default all   I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    int i, nn;
    char *host;
    char c;

    /* Read config files, setup Globus, etc.*/
    if (!qcdgridInit(0)) 
    {
	return 1;
    }

    if (argc == 2)
    {
	host = argv[1];
	if (nodeIndexFromName(host) < 0)
	{
	    globus_libc_fprintf(stderr, "%s is not a grid node\n", host);
	    return 1;
	}

	deleteEntireLocation(host);
    }
    else
    {
	globus_libc_printf("This will delete the whole QCDgrid replica catalogue!\n");
	globus_libc_printf("Are you sure you want to continue? (Y/N)\n");
	
	c = fgetc(stdin);
	if ((c != 'y') && (c != 'Y'))
	{
	    return 0;
	}
	
	/* Loop over every node in the grid */
	nn = getNumNodes();
	for (i = 0; i < nn; i++)
	{
	    host = getNodeName(i);
	    globus_libc_printf("Deleting location %s...\n", host);
	    deleteEntireLocation(host);
	}
	
	/* 
	 * FIXME: this will leave any information from nodes not currently
	 * in the node list. This may not matter.
	 */
    }

    return 0;
}
