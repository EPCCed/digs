/***********************************************************************
*
*   Filename:   verify-qcdgrid-rc.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Makes sure that the QCDgrid replica catalogue matches
*               what's actually in the nodes' storage directories
*
*   Contents:   Main function, support functions specific to this
*               command
*
*   Used in:    QCDgrid administration
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

#include "verify.h"
#include "node.h"
#include "misc.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of replica catalogue verifying executable
*    
*   Parameters:                                               [I/O]
*
*     (optional) "-n" or "--non-interactive" to run in non-    I
*                interactive mode (just print report)
*     (optional) hostname to run check on (default all nodes)  I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    int interactive = 1;
    char *node = NULL;
    char *usage = "Usage: digs-verify-rc [-n | --non-interactive]  [<host>]\n"
                  "       digs-verify-rc [-f | --force-default-values] [<host>]\n\n"
	"This command verifies that the replica catalogue matches the\n"
	"actual file structure on the nodes and allows the user to\n"
	"repair inconsistencies or forces the default values\n\n";

    if (argc > 3)
    {
	globus_libc_printf(usage);
	return 0;
    }

    if (argc > 1)
    {
	if ((!strcmp(argv[1], "-n")) || (!strcmp(argv[1], "--non-interactive")))
	{
	    interactive = 0;
	    if (argc > 2)
	    {
		node = argv[2];
	    }
	}
	else if ((!strcmp(argv[1], "-f")) || (!strcmp(argv[1], "--force-default-values")))
	{
	    interactive = -1;
	    if (argc > 2)
	    {
		node = argv[2];
	    }
	}
	else
	{
	    node = argv[1];
	    if (argc > 2)
	    {
		globus_libc_printf(usage);
		return 0;
	    }
	}
    }

    /* Read config files, setup Globus, etc.*/
    if (!qcdgridInit(0)) 
    {
	return 1;
    }

    /* validate node name given */
    if (node)
    {
	if (nodeIndexFromName(node) < 0)
	{
	    globus_libc_fprintf(stderr, "%s is not a grid node\n", node);
	    return 1;
	}
    }

    /* Run the verify in interactive mode */
    runGridVerification(node, interactive);
    
    /* Print out results */
    globus_libc_printf("\n\nInconsistencies found: %d\n", inconsistencies_);
    globus_libc_printf("Inconsistencies remaining: %d\n\n", unrepairedInconsistencies_);

    return 0;
}
