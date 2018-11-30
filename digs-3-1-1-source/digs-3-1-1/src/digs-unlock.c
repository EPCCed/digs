/***********************************************************************
*
*   Filename:   digs-unlock.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to unlock files
*               on a DiGS system
*
*   Contents:   Main function
*
*   Used in:    Day to day usage of DiGS
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2007-2008 The University of Edinburgh
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
#include "replica.h"
#include "hashtable.h"
#include "config.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of digs-unlock executable. Sets everything
*   up, then sends a message to the main node
*    
*   Parameters:                                            [I/O]
*
*     (optional) "-R" or "--recursive" to unlock a whole    I
*                directory tree recursively
*     Logical filename of file to be unlocked               I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char *usage = "Usage: digs-unlock [-R|--recursive] <filename>\n";
    int recursive = 0;
    char *lfn;
    int i;
    char *msgBuffer;

    /* check arguments */
    if (argc < 2)
    {
	globus_libc_printf(usage);
	return 1;
    }

    for (i = 1; i < (argc-1); i++)
    {
	if ((!strcmp(argv[i], "-R")) || (!strcmp(argv[i], "--recursive")))
	{
	    recursive = 1;
	}
	else
	{
	    globus_libc_printf("Unrecognised argument %s\n", argv[i]);
	    globus_libc_printf(usage);
	    return 1;
	}
    }
    lfn = argv[argc - 1];

    /*
     * Initialise grid
     */
    if (!qcdgridInit(0)) 
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	return 1;
    }
    atexit(qcdgridShutdown);

    if (recursive)
    {
	/* let the control thread handle all the checks if recursive, just send message */
	if (safe_asprintf(&msgBuffer, "unlockdir %s", lfn) < 0)
	{
	    globus_libc_fprintf(stderr, "Out of memory\n");
	    return 1;
	}
	if (!sendMessageToMainNode(msgBuffer))
	{
	    globus_libc_free(msgBuffer);
	    globus_libc_fprintf(stderr, "Error sending message to main node\n");
	    return 1;
	}
	globus_libc_free(msgBuffer);
    }
    else
    {
	char *lockedby;
	char *identity;
	char *admin;
	char *loc;
	int isAdmin = 0;

	/* check if file exists */
	loc = getFirstFileLocation(lfn);
	if (loc == NULL)
	{
	    globus_libc_fprintf(stderr, "File %s does not exist on grid\n", lfn);
	    return 1;
	}
	globus_libc_free(loc);

	/* 
	 * Check if user allowed to unlock this file. As in digs-lock, these
	 * checks don't enforce anything. They're just to give more meaningful
	 * error messages if something goes wrong. All enforcement happens on
	 * control thread side
	 */
	if (!getUserIdentity(&identity))
	{
	    globus_libc_fprintf(stderr, "Error getting user identity\n");
	    return 1;
	}
	admin = getFirstConfigValue("miscconf", "administrator");
	while (admin)
	{
	    if (!strcmp(admin, identity))
	    {
		isAdmin = 1;
		break;
	    }
	    admin = getNextConfigValue("miscconf", "administrator");
	}

	lockedby = getRLSAttribute(lfn, "lockedby");
	if ((lockedby == NULL) || (!strcmp(lockedby, "(null)")))
	{
	    globus_libc_fprintf(stderr, "File %s is not locked\n", lfn);
	    globus_libc_free(identity);
	    return 1;
	}

	if ((!isAdmin) && (strcmp(identity, lockedby)))
	{
	    globus_libc_fprintf(stderr, "File %s is locked by %s\n", lfn, lockedby);
	    globus_libc_free(identity);
	    globus_libc_free(lockedby);
	    return 1;
	}
	globus_libc_free(lockedby);
	globus_libc_free(identity);

	/* actually unlock the file */
	if (safe_asprintf(&msgBuffer, "unlock %s", lfn) < 0)
	{
	    globus_libc_fprintf(stderr, "Out of memory\n");
	    return 1;
	}
	if (!sendMessageToMainNode(msgBuffer))
	{
	    globus_libc_free(msgBuffer);
	    globus_libc_fprintf(stderr, "Error sending message to main node\n");
	    return 1;
	}
	globus_libc_free(msgBuffer);
    }
    
    return 0;
}
