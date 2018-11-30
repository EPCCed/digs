/***********************************************************************
*
*   Filename:   background-delete.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Code related to deleting files on QCDgrid
*
*   Contents:   Everything delete related, except client-side
*
*   Used in:    Background process on main node of QCDgrid
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

#include "background-delete.h"
#include "background-modify.h"
#include "node.h"
#include "replica.h"
#include "misc.h"

/*
 * Delete operations which have been requested but have not yet been
 * done due to a host being unreachable are stored in an array of this
 * structure. Then each time a host comes back from the dead (list),
 * see if there were any pending deletes for that host and execute them.
 *
 * The list also needs to be saved to disk when it changes in case the
 * control thread goes down.
 */
typedef struct pendingDeletes_s
{
    char *host;
    char *file;
} pendingDeletes_t;

int numPendingDeletes_;
pendingDeletes_t *pendingDeletes_;

extern int groupModification_;

/***********************************************************************
*   void savePendingDeleteList()
*    
*   Saves the pending delete list into file pendingdels. It's copied
*   here whenever it changes in case this thread is terminated with
*   delete operations pending
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: (void)
************************************************************************/
static void savePendingDeleteList()
{
    char *filenameBuffer;
    FILE *f;
    int i;

    logMessage(1, "savePendingDeleteList()");

    if (safe_asprintf(&filenameBuffer, "%s/pendingdels", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in savePendingDeleteList");
    }

    f = fopen(filenameBuffer, "w");
    
    for (i = 0; i < numPendingDeletes_; i++)
    {
	globus_libc_fprintf(f, "%s %s\n", pendingDeletes_[i].host, 
			    pendingDeletes_[i].file);
    }
    fclose(f);
    globus_libc_free(filenameBuffer);
}

/***********************************************************************
*   int loadPendingDeleteList()
*    
*   Loads the pending delete list from the file pendingdels (if it
*   exists). Should be called as soon as the background process starts
*   up.
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: 1 on success, 0 on error
************************************************************************/
int loadPendingDeleteList()
{
    char *filenameBuffer;
    char *lineBuffer = NULL;
    char *tmp;
    int lineBufferSize=0;
    FILE *f;

    logMessage(1, "loadPendingDeleteList()");

    numPendingDeletes_ = 0;
    pendingDeletes_ = NULL;

    if (safe_asprintf(&filenameBuffer, "%s/pendingdels", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in loadPendingDeleteList");
    }

    f = fopen(filenameBuffer, "r");
    globus_libc_free(filenameBuffer);

    /* Might not have been created yet */
    if (!f)
    {
	return 1;
    }

    safe_getline(&lineBuffer, &lineBufferSize, f);
    while (!feof(f))
    {
	removeCrlf(lineBuffer);
	tmp = strchr(lineBuffer, ' ');
	if (!tmp) 
	{
	    logMessage(5, "Warning: malformed pending deletion list");
	    break;
	}
	*tmp=0;
	
	pendingDeletes_ = globus_libc_realloc(pendingDeletes_, (numPendingDeletes_+1)
					      *sizeof(pendingDeletes_t));
	if (!pendingDeletes_)
	{
	    fclose(f);
	    globus_libc_free(lineBuffer);
	    return 0;
	}

	pendingDeletes_[numPendingDeletes_].host = safe_strdup(lineBuffer);
	pendingDeletes_[numPendingDeletes_].file = safe_strdup(tmp + 1);
	numPendingDeletes_++;

	safe_getline(&lineBuffer, &lineBufferSize, f);
    }

    globus_libc_free(lineBuffer);
    fclose(f);
    return 1;
}

/***********************************************************************
*   void addPendingDelete(char *host, char *file)
*    
*   Adds a delete operation to the list of pending deletes
*    
*   Parameters:                                        [I/O]
*
*     host  The FQDN of the machine to delete from      I
*     file  The full pathname of the file to delete     I
*
*   Returns: (void)
************************************************************************/
static void addPendingDelete(char *host, char *file)
{
    logMessage(1, "addPendingDelete(%s,%s)", host, file);

    pendingDeletes_ = globus_libc_realloc(pendingDeletes_, 
					  (numPendingDeletes_+1)*sizeof(pendingDeletes_t));
    if (!pendingDeletes_)
    {
	errorExit("Out of memory in addPendingDelete");
    }

    pendingDeletes_[numPendingDeletes_].host = safe_strdup(host);
    pendingDeletes_[numPendingDeletes_].file = safe_strdup(file);

    if ((!pendingDeletes_[numPendingDeletes_].host) ||
	(!pendingDeletes_[numPendingDeletes_].file))
    {
	errorExit("Out of memory in addPendingDelete");
    }

    numPendingDeletes_++;
    savePendingDeleteList();
}

/***********************************************************************
*   void removePendingDelete(char *host, char *file)
*    
*   Removes a delete operation from the list of pending deletes
*    
*   Parameters:                                        [I/O]
*
*     host  The FQDN of the machine to delete from      I
*     file  The full pathname of the file to delete     I
*
*   Returns: (void)
************************************************************************/
static void removePendingDelete(char *host, char *file)
{
    int i;

    for (i=0; i<numPendingDeletes_; i++)
    {
	if ((!strcmp(pendingDeletes_[i].host, host))&&
	    (!strcmp(pendingDeletes_[i].file, file)))
	{
	    globus_libc_free(pendingDeletes_[i].host);
	    globus_libc_free(pendingDeletes_[i].file);
	    
	    numPendingDeletes_--;
	    for (; i < numPendingDeletes_; i++)
	    {
		pendingDeletes_[i] = pendingDeletes_[i + 1];
	    }

	    pendingDeletes_=globus_libc_realloc(pendingDeletes_, (numPendingDeletes_+1)
						*(sizeof(pendingDeletes_t)));

	    if (!pendingDeletes_)
	    {
		errorExit("Out of memory in removePendingDelete");
	    }

	    savePendingDeleteList();
	    return;
	}
    }
}

/***********************************************************************
*   void tryDeletesOnHost(char *host)
*    
*   Attempts to execute the pending delete operations on a particular
*   host. Should be called when a host is removed from the dead or
*   disabled list, for example
*    
*   Parameters:                                        [I/O]
*
*     host  The FQDN of the machine to delete from      I
*
*   Returns: (void)
************************************************************************/
void tryDeletesOnHost(char *host)
{
    int i;
    int allDone;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    struct storageElement *se;
    digs_error_code_t result;

    logMessage(1, "tryDeletesOnHost(%s)", host);

    allDone = 0;

    se = getNode(host);
    if (!se)
    {
        logMessage(ERROR, "Cannot get node structure for %s", host);
        return;
    }

    while (!allDone)
    {
	allDone = 1;

	for (i = 0; i < numPendingDeletes_; i++)
	{
	    if (!strcmp(pendingDeletes_[i].host, host))
	    {
	        result = se->digs_rm(errbuf, host, pendingDeletes_[i].file);
		if (result == DIGS_SUCCESS)
		{
		    removePendingDelete(host, pendingDeletes_[i].file);
		    allDone = 0;
		    
		    /* Need to restart iteration as pendingDeletes_ list has
		     * been changed now */
		    break;
		}
	    }
	}
    }
}

/***********************************************************************
*   void deleteLogicalFile(char *lfn)
* 
*   The top level delete function. Tries to remove all copies of the
*   specified file from the grid. If any of the nodes are down, delete
*   operations for them are added to the pending list instead of being
*   executed right away.   
*    
*   Parameters:                                        [I/O]
*
*     lfn   The logical filename of the file to delete  I
*
*   Returns: (void)
************************************************************************/
void deleteLogicalFile(char *lfn)
{
    char *location, *path;
    struct storageElement *se;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    logMessage(1, "deleteLogicalFile(%s)", lfn);

    /* Iterate over all the file's locations */
    location = getFirstFileLocationAll(lfn);

    while (location)
    {
        se = getNode(location);
	if (!se)
	{
	    logMessage(ERROR, "Error getting SE struct for %s", location);
	    globus_libc_free(location);
	    location = getNextFileLocation();
	    continue;
	}

	/* get full path to file on that node */
	path = constructFilename(location, lfn);
	if (!path)
	{
	    logMessage(3, "Error constructing path "
		       "for %s on %s", lfn, location);
	    globus_libc_free(location);
	    location = getNextFileLocation();
	    continue;
	}

	/* Remove replica catalogue entry */
	removeFileFromLocation(location, lfn);

	/* Add a pending delete for dead or disabled nodes...*/
	if ((isNodeDead(location)) || (isNodeDisabled(location)))
	{
	    addPendingDelete(location, path);
	}

	/* ..otherwise try to do it straight away, but add a pending delete if
	 * that fails. */
	else if (se->digs_rm(errbuf, location, path) != DIGS_SUCCESS)
	{
	    addPendingDelete(location, path);
	}

	globus_libc_free(path);
	globus_libc_free(location);
	location = getNextFileLocation();
    }

    /* Remove the file's collection entry in the replica catalogue */
    removeFileFromCollection(lfn);
}

/***********************************************************************
*   int deleteDirectoryCallback(char *lfn, void *param)
*
*   Called for each file in a directory being deleted
*    
*   Parameters:                                         [I/O]
*
*     lfn   The logical name of the file to delete       I
*     param Not used
*
*   Returns: 1, to continue the iteration
************************************************************************/
static int deleteDirectoryCallback(char *lfn, void *param)
{
    deleteLogicalFile(lfn);
    return 1;
}

/***********************************************************************
*   void deleteDirectory(char *lfn)
*
*   Recursively deletes an entire directory. Could be used to wipe
*   the entire grid, but various checks have already been made by the
*   time we reach this point (i.e. can only be done by an administrator,
*   required confirmation) 
*    
*   Parameters:                                         [I/O]
*
*     lfn   The logical name of the directory to delete  I
*
*   Returns: (void)
************************************************************************/
void deleteDirectory(char *lfn)
{
    logMessage(1, "deleteDirectory(%s)", lfn);

    forEachFileInDirectory(lfn, deleteDirectoryCallback, NULL);
}

/***********************************************************************
*   int canDeleteFile(char *lfn, char *user)
*
*   Check whether a certain user is allowed to delete a certain file
*    
*   Parameters:                                         [I/O]
*
*     lfn      The logical grid filename to check        I
*     user     The user's certificate subject            I
*
*   Returns: 1 if the user is allowed to delete the file, 0 if not
************************************************************************/
int canDeleteFile(char *lfn, char *user)
{
    char *submitter;
    char *lockedby;

    lockedby = fileLockedBy(lfn);
    if (lockedby != NULL)
    {
	if (strcmp(lockedby, user))
	{
	    logMessage(3, "User %s cannot delete file %s as it is locked by %s",
		       user, lfn, lockedby);
	    globus_libc_free(lockedby);
	    return 0;
	}
	globus_libc_free(lockedby);
    }

    if (groupModification_)
    {
      char *filegroup;
      char *usergroup;

      if (!getAttrValueFromRLS(lfn, "group", &filegroup)) {
	logMessage(5, "Error obtaining group from RLS for file %s", lfn);
	return 0;
      }

      if (!getUserGroup(user, &usergroup)) {
	logMessage(5, "Error getting group for user %s", user);
	globus_libc_free(filegroup);
	return 0;
      }

      if (!strcmp(usergroup, filegroup)) {
	logMessage(1, "User %s is allowed to delete file %s (group based)", user,
		   lfn);
	globus_libc_free(usergroup);
	globus_libc_free(filegroup);
	return 1;
      }
      globus_libc_free(usergroup);
      globus_libc_free(filegroup);
    }
    else
    {
      if(!getAttrValueFromRLS(lfn, "submitter", &submitter))
	{
	  logMessage(5, "Error obtaining the submitter from RLS for file %s", lfn);
	  return 0;
	}
      
      logMessage(1, "Checking if user=%s can delete file=%s submitted by=%s", user, lfn, submitter);
      
      if(strcmp(user, submitter) == 0)
	{
	  logMessage(1, "User is allowed to delete the file");
	  globus_libc_free(submitter);
	  return 1;
	}
      globus_libc_free(submitter);
    }

    logMessage(3, "User is NOT allowed to delete the file");
    return 0;
}

/***********************************************************************
*   int canDeleteDirectoryCallback(char *lfn, void *param)
*
*   Called for each file in a directory to check if a user can delete it
*    
*   Parameters:                                         [I/O]
*
*     lfn      The logical grid directory to check       I
*     param    The user's certificate subject            I
*
*   Returns: 1 if the user is allowed to delete the file, 0 if not
************************************************************************/
static int canDeleteDirectoryCallback(char *lfn, void *param)
{
    char *user = (char *)param;

    if (!canDeleteFile(lfn, user))
    {
	return 0;
    }
    return 1;
}

/***********************************************************************
*   int canDeleteDirectory(char *lfn, char *user)
*
*   Check whether a certain user is allowed to delete a certain
*   directory
*    
*   Parameters:                                         [I/O]
*
*     lfn      The logical grid directory to check       I
*     user     The user's certificate subject            I
*
*   Returns: 1 if the user is allowed to delete the directory, 0 if not
************************************************************************/
int canDeleteDirectory(char *lfn, char *user)
{
    if (!forEachFileInDirectory(lfn, canDeleteDirectoryCallback, user))
    {
	return 0;
    }
    return 1;
}
