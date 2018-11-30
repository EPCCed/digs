/***********************************************************************
*
*   Filename:   digs-lock.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to lock files on
*               a DiGS system
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
*   Main entry point of digs-lock executable. Sets everything
*   up, then sends a message to the main node
*    
*   Parameters:                                            [I/O]
*
*     (optional) "-R" or "--recursive" to lock a whole      I
*                directory tree recursively
*     (optional) "-c" to check whether file is locked and   I
*                by whom, rather than locking it
*     Logical filename of file to be locked                 I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char *usage = "Usage: digs-lock [-c] [-R|--recursive] <filename>\n";
    int i;
    int recursive = 0;
    int check = 0;
    char *lfn;
    char *lockedby;
    char *loc;
    char *identity;
    char *msgBuffer;
    int isAdmin = 0;
    char *admin;

    /*
     * First check arguments
     */
    if (argc < 2)
    {
	globus_libc_printf(usage);
	return 1;
    }
    for (i = 1; i < (argc-1); i++)
    {
	if (!strcmp(argv[i], "-c"))
	{
	    check = 1;
	}
	else if ((!strcmp(argv[i], "-R")) || (!strcmp(argv[i], "--recursive")))
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

    /* for single file, check it does exist */
    if (!recursive)
    {
	loc = getFirstFileLocation(lfn);
	if (loc == NULL)
	{
	    globus_libc_fprintf(stderr, "File %s not found on grid\n", lfn);
	    return 1;
	}
	globus_libc_free(loc);
    }

    /*
     * Get user identity and check whether they're an admin
     *
     * Note: the permission checks in this module don't actually enforce
     * anything. That's done on the server side. But by checking in detail
     * here we can give more meaningful error messages if something goes
     * wrong.
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

    if (recursive)
    {
	/*
	 * Recursive lock/check
	 */
	char *prefix;
	char *file;
	qcdgrid_hash_table_t *attrs;
	int allUnlocked = 1;
	char *allLockedBy = NULL;
	int firstTime = 1;

	/*
	 * Iterate over all files in directory.
	 *
	 * If checking, the purpose of this first loop is to see whether
	 * all the files in the directory have the same lock status (all
	 * locked by the same person, or all unlocked), so we know whether
	 * we can print a simple lock status message rather than iterating
	 * each file individually.
	 *
	 * If locking, the first loop checks whether there are any files
	 * in the directory locked by someone else, that would prevent the
	 * lock operation from succeeding.
	 */
	if (safe_asprintf(&prefix, "%s/", lfn) < 0)
	{
	    globus_libc_fprintf(stderr, "Out of memory\n");
	    return 1;
	}

	/* allow prefix ending in slash already */
	if (prefix[strlen(prefix) - 2] == '/')
	{
	    prefix[strlen(prefix) - 1] = 0;
	}

	attrs = getAllAttributesValues("lockedby");

	file = getFirstFile();
	while (file)
	{
	    if (!strncmp(file, prefix, strlen(prefix)))
	    {
		/* found one in our directory */
		if (check)
		{
		    /* want to know whether it's consistent or not */
		    lockedby = lookupValueInHashTable(attrs, file);
		    if (lockedby)
		    {
		        if (strcmp(lockedby, "(null)"))
			{
			    if (firstTime)
			    {
				allLockedBy = safe_strdup(lockedby);
				allUnlocked = 0;
			    }
			    else
			    {
				if ((!allLockedBy) || (strcmp(allLockedBy, lockedby)))
				{
				    /* have to do it the long way */
				    if (allLockedBy) globus_libc_free(allLockedBy);
				    allLockedBy = NULL;
				    allUnlocked = 0;
				    globus_libc_free(lockedby);
				    break;
				}
			    }
			}
			else
			{
			    if (!allUnlocked)
			    {
				/* have to do it the long way */
				if (allLockedBy) globus_libc_free(allLockedBy);
				allLockedBy = NULL;
				break;
			    }
			}

			globus_libc_free(lockedby);
		    }
		    else
		    {
			if (!allUnlocked)
			{
			    /* have to do it the long way */
			    if (allLockedBy) globus_libc_free(allLockedBy);
			    allLockedBy = NULL;
			    break;
			}
		    }
		    firstTime = 0;
		}
		else
		{
		    /* want to know if this prevents us locking or not */
		    lockedby = lookupValueInHashTable(attrs, file);
		    if (lockedby)
		    {
		        if (strcmp(lockedby, "(null)"))
			{
			    if ((!isAdmin) && (strcmp(lockedby, identity)))
			    {
				globus_libc_fprintf(stderr,
						    "Cannot lock directory, %s already locked by %s\n",
						    file, lockedby);
				globus_libc_free(lockedby);
				destroyKeyAndValueHashTable(attrs);
				globus_libc_free(prefix);
				globus_libc_free(identity);
				return 1;
			    }
			}
			globus_libc_free(lockedby);
		    }
		}
	    }

	    file = getNextFile();
	}

	if (check)
	{
	    /* check the directory */
	    if (allUnlocked)
	    {
		globus_libc_printf("Directory %s is unlocked\n", lfn);
	    }
	    else if (allLockedBy)
	    {
		globus_libc_printf("Directory %s is locked by %s\n", lfn, allLockedBy);
		globus_libc_free(allLockedBy);
	    }
	    else
	    {
		/* do complicated check here */
		file = getFirstFile();
		while (file)
		{
		    if (!strncmp(file, prefix, strlen(prefix)))
		    {
			lockedby = lookupValueInHashTable(attrs, file);
			if (lockedby)
			{
			    if (strcmp(lockedby, "(null)"))
			    {
				globus_libc_printf("File %s is locked by %s\n", file, lockedby);
			    }
			    globus_libc_free(lockedby);
			}
		    }
		    file = getNextFile();
		}
	    }
	}
	else
	{
	    /* lock directory */
	    if (safe_asprintf(&msgBuffer, "lockdir %s", lfn) < 0)
	    {
		globus_libc_fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    if (!sendMessageToMainNode(msgBuffer))
	    {
		globus_libc_free(msgBuffer);
		globus_libc_fprintf(stderr, "Error sending message to main node");
		destroyKeyAndValueHashTable(attrs);
		globus_libc_free(prefix);
		globus_libc_free(identity);
		return 1;
	    }
	    globus_libc_free(msgBuffer);
	}

	destroyKeyAndValueHashTable(attrs);
	globus_libc_free(prefix);
    }
    else
    {
	/*
	 * Non-recursive, single file
	 */
	lockedby = getRLSAttribute(lfn, "lockedby");
	if (check)
	{
	    /* check lock status of the file */
	    if ((lockedby != NULL) && (strcmp(lockedby, "(null)")))
	    {
		globus_libc_printf("File %s is locked by %s\n", lfn, lockedby);
	    }
	    else
	    {
		globus_libc_printf("File %s is not locked\n", lfn);
	    }
	}
	else
	{
	    /* actually lock the file if possible */
	    if ((lockedby) && (strcmp(lockedby, "(null)")) && (!isAdmin) && (strcmp(identity, lockedby)))
	    {
		globus_libc_fprintf(stderr, "File %s is already locked by %s\n", lfn, lockedby);
		globus_libc_free(lockedby);
		globus_libc_free(identity);
		return 1;
	    }

	    if (safe_asprintf(&msgBuffer, "lock %s", lfn) < 0)
	    {
		globus_libc_fprintf(stderr, "Out of memory\n");
		return 1;
	    }

	    if (!sendMessageToMainNode(msgBuffer))
	    {
		globus_libc_free(msgBuffer);
		globus_libc_fprintf(stderr, "Error sending message to main node");
		globus_libc_free(identity);
		if (lockedby) globus_libc_free(lockedby);
		return 1;
	    }
	    globus_libc_free(msgBuffer);
	}
	if (lockedby)
	{
	    globus_libc_free(lockedby);
	}
    }

    globus_libc_free(identity);

    return 0;
}
