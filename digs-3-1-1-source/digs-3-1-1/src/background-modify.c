/***********************************************************************
*
*   Filename:   background-modify.c
*
*   Authors:    James Perry              (jamesp)   EPCC.
*
*   Purpose:    Code for modifying files in DiGS
*
*   Contents:   Functions related to modifying and locking files on
*               grid
*
*   Used in:    Background process on main node of DiGS
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

#include "background-modify.h"
#include "background-delete.h"
#include "misc.h"
#include "replica.h"
#include "node.h"

typedef struct newModification_s {
    char *lfn;
    char *host;
    char *md5sum;
    long long size;
    char *time;
} newModification_t;

static int numNewModifications_ = 0;
static newModification_t *newModifications_ = NULL;

typedef struct pendingModification_s {
    char *lfn;
    char *host;
    char *source;   /* host where the new version can be found */
} pendingModification_t;

static int numPendingModifications_ = 0;
static pendingModification_t *pendingModifications_ = NULL;

/***********************************************************************
*   void savePendingModList()
*    
*   Saves the pending and new modification lists to disk in case the
*   control thread is interrupted before they are processed.
*    
*   Parameters:                                           [I/O]
*
*     (none)
*
*   Returns: (void)
************************************************************************/
void savePendingModList()
{
    char *filenameBuffer;
    FILE *f;
    int i;

    logMessage(1, "savePendingModList()");

    if (safe_asprintf(&filenameBuffer, "%s/newmods", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in savePendingModList");
    }

    f = fopen(filenameBuffer, "w");
    if (!f)
    {
	logMessage(FATAL, "Couldn't save new mod list to %s", filenameBuffer);
	return;
    }
    globus_libc_free(filenameBuffer);
    for (i = 0; i < numNewModifications_; i++)
    {
	globus_libc_fprintf(f, "%s %s %s %qd %s",
			    newModifications_[i].lfn,
			    newModifications_[i].host,
			    newModifications_[i].md5sum,
			    newModifications_[i].size,
			    newModifications_[i].time);
    }
    fclose(f);

    /* Save both new and pending lists here */
    if (safe_asprintf(&filenameBuffer, "%s/pendingmods", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in savePendingModList");
    }

    f = fopen(filenameBuffer, "w");
    if (!f)
    {
	logMessage(FATAL, "Couldn't save pending mod list to %s", filenameBuffer);
	return;
    }
    globus_libc_free(filenameBuffer);
    for (i = 0; i < numPendingModifications_; i++)
    {
	globus_libc_fprintf(f, "%s %s %s", pendingModifications_[i].lfn,
			    pendingModifications_[i].host,
			    pendingModifications_[i].source);
    }
    fclose(f);
}

/***********************************************************************
*   void loadPendingModList()
*    
*   Loads the pending and new modification lists back from disk
*    
*   Parameters:                                           [I/O]
*
*     (none)
*
*   Returns: (void)
************************************************************************/
void loadPendingModList()
{
    char *filenameBuffer;
    FILE *f;
    char *lineBuffer = NULL;
    int lineBufferSize = 0;
    char *spc1, *spc2, *spc3, *spc4;
    newModification_t *nm;
    pendingModification_t *pm;

    logMessage(1, "loadPendingModList()");

    numNewModifications_ = 0;
    newModifications_ = NULL;
    numPendingModifications_ = 0;
    pendingModifications_ = NULL;

    if (safe_asprintf(&filenameBuffer, "%s/newmods", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in loadPendingModList");
    }

    f = fopen(filenameBuffer, "r");
    globus_libc_free(filenameBuffer);
    if (f)
    {
	/* might not always exist */
   
	safe_getline(&lineBuffer, &lineBufferSize, f);
	while (!feof(f))
	{
	    removeCrlf(lineBuffer);

	    /* parse this line */
	    spc1 = strchr(lineBuffer, ' ');
	    if (!spc1)
	    {
		logMessage(5, "Invalid line %s in newmods file", lineBuffer);
		safe_getline(&lineBuffer, &lineBufferSize, f);
		continue;
	    }
	    spc2 = strchr(spc1+1, ' ');
	    if (!spc2)
	    {
		logMessage(5, "Invalid line %s in newmods file", lineBuffer);
		safe_getline(&lineBuffer, &lineBufferSize, f);
		continue;
	    }
	    spc3 = strchr(spc2+1, ' ');
	    if (!spc3)
	    {
		logMessage(5, "Invalid line %s in newmods file", lineBuffer);
		safe_getline(&lineBuffer, &lineBufferSize, f);
		continue;
	    }
	    spc4 = strchr(spc3+1, ' ');
	    if (!spc4)
	    {
		logMessage(5, "Invalid line %s in newmods file", lineBuffer);
		safe_getline(&lineBuffer, &lineBufferSize, f);
		continue;
	    }

	    *spc1 = 0;
	    *spc2 = 0;
	    *spc3 = 0;
	    *spc4 = 0;

	    numNewModifications_++;
	    newModifications_ = globus_libc_realloc(newModifications_,
						    numNewModifications_*sizeof(newModification_t));
	    if (!newModifications_)
	    {
		errorExit("Out of memory in loadPendingModList");
	    }
	    nm = &newModifications_[numNewModifications_ - 1];
	    nm->lfn = safe_strdup(lineBuffer);
	    nm->host = safe_strdup(spc1+1);
	    nm->md5sum = safe_strdup(spc2+1);
	    nm->size = strtoll(spc3+1, NULL, 10);
	    nm->time = safe_strdup(spc4+1);

	    if ((!nm->lfn) || (!nm->host) || (!nm->md5sum) || (!nm->time))
	    {
		errorExit("Out of memory in loadPendingModList");
	    }

	    safe_getline(&lineBuffer, &lineBufferSize, f);
	}

    }

    /* Load both new and pending lists here */
    if (safe_asprintf(&filenameBuffer, "%s/pendingmods", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in loadPendingModList");
    }

    f = fopen(filenameBuffer, "r");
    globus_libc_free(filenameBuffer);
    if (f)
    {
	/* might not always exist */
   
	safe_getline(&lineBuffer, &lineBufferSize, f);
	while (!feof(f))
	{
	    removeCrlf(lineBuffer);

	    /* parse this line */
	    spc1 = strchr(lineBuffer, ' ');
	    if (!spc1)
	    {
		logMessage(5, "Invalid line %s in pendingmods file", lineBuffer);
		safe_getline(&lineBuffer, &lineBufferSize, f);
		continue;
	    }
	    spc2 = strchr(spc1+1, ' ');
	    if (!spc2)
	    {
		logMessage(5, "Invalid line %s in pendingmods file", lineBuffer);
		safe_getline(&lineBuffer, &lineBufferSize, f);
		continue;
	    }

	    *spc1 = 0;
	    *spc2 = 0;

	    numPendingModifications_++;
	    pendingModifications_ = globus_libc_realloc(pendingModifications_,
							numPendingModifications_*sizeof(pendingModification_t));
	    if (!pendingModifications_)
	    {
		errorExit("Out of memory in loadPendingModList");
	    }
	    pm = &pendingModifications_[numPendingModifications_ - 1];
	    pm->lfn = safe_strdup(lineBuffer);
	    pm->host = safe_strdup(spc1+1);
	    pm->source = safe_strdup(spc2+1);

	    if ((!pm->lfn) || (!pm->host) || (!pm->source))
	    {
		errorExit("Out of memory in loadPendingModList");
	    }

	    safe_getline(&lineBuffer, &lineBufferSize, f);
	}
    }
}

/***********************************************************************
*   int isNewModification(char *lfn, char *host)
*    
*   Checks whether the specified file has been submitted to the
*   specified host as a new modification
*    
*   Parameters:                                           [I/O]
*
*     lfn   Logical filename to check                      I
*     host  host to check                                  I
*
*   Returns: 1 if file has been submitted for modification, 0 if not
************************************************************************/
int isNewModification(char *lfn, char *host)
{
    int i;
    for (i = 0; i < numNewModifications_; i++) {
	if ((!strcmp(newModifications_[i].lfn, lfn)) &&
	    (!strcmp(newModifications_[i].host, host))) {
	    return 1;
	}
    }
    return 0;
}

/***********************************************************************
*   void addPendingModification(char *lfn, char *host, char *source)
*    
*   Adds a new pending modification to the list
*    
*   Parameters:                                           [I/O]
*
*     lfn     logical file being modified                  I
*     host    host to modify on                            I
*     source  host to fetch new copy from                  I
*
*   Returns: (void)
************************************************************************/
static void addPendingModification(char *lfn, char *host, char *source)
{
    pendingModification_t *pm;

    numPendingModifications_++;
    pendingModifications_ = globus_libc_realloc(pendingModifications_,
						numPendingModifications_ *
						sizeof(pendingModification_t));
    if (!pendingModifications_)
    {
	errorExit("Out of memory in addPendingModification");
    }
    pm = &pendingModifications_[numPendingModifications_-1];

    pm->lfn = safe_strdup(lfn);
    pm->host = safe_strdup(host);
    pm->source = safe_strdup(source);

    if ((!pm->lfn) || (!pm->host) || (!pm->source))
    {
	errorExit("Out of memory in addPendingModification");
    }
}

/***********************************************************************
*   void removeNewModification(char *lfn, char *host)
*    
*   Removes an entry from the new modifications list
*    
*   Parameters:                                           [I/O]
*
*     lfn     logical file being modified                  I
*     host    host uploaded to                             I
*
*   Returns: (void)
************************************************************************/
static void removeNewModification(char *lfn, char *host)
{
    int i;

    for (i = 0; i < numNewModifications_; i++)
    {
	if ((!strcmp(newModifications_[i].lfn, lfn)) &&
	    (!strcmp(newModifications_[i].host, host)))
	{
	    break;
	}
    }
    if (i >= numNewModifications_)
    {
	/* not found */
	return;
    }

    /* free this storage */
    globus_libc_free(newModifications_[i].lfn);
    globus_libc_free(newModifications_[i].host);
    globus_libc_free(newModifications_[i].md5sum);
    globus_libc_free(newModifications_[i].time);

    /* move the others down the array */
    numNewModifications_--;
    for (; i < numNewModifications_; i++)
    {
	newModifications_[i] = newModifications_[i+1];
    }
}

/***********************************************************************
*   void prunePendingModList()
*    
*   Deletes any entries from the pending modification list that have
*   been marked for removal
*    
*   Parameters:                                           [I/O]
*
*     (none)
*
*   Returns: (void)
************************************************************************/
static void prunePendingModList()
{
    int i;
    int changed;
    
    do
    {
	changed = 0;
	for (i = 0; i < numPendingModifications_; i++)
	{
	    if (pendingModifications_[i].host == NULL)
	    {
		break;
	    }
	}
	if (i < numPendingModifications_)
	{
	    /* found one to delete */
	    globus_libc_free(pendingModifications_[i].lfn);
	    globus_libc_free(pendingModifications_[i].source);

	    numPendingModifications_--;
	    changed = 1;
	    for (; i < numPendingModifications_; i++)
	    {
		pendingModifications_[i] = pendingModifications_[i + 1];
	    }
	}
    } while (changed);
}


/***********************************************************************
*   int handleNewModification(char *lfn, char *realLfn, char *host)
*    
*   Processes a file submitted for modification.
*    
*   Parameters:                                             [I/O]
*
*     lfn   filename to modify - flat filename within inbox  I
*     realLfn  final lfn for file                            I
*     host  host it is on                                    I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int handleNewModification(char *lfn, char *realLfn, char *host)
{
    char *fullSrcPath;
    char *fullDestPath;
    char *inbox;
    char *checksum;
    struct storageElement *se;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;
    int i;
    newModification_t *nm = NULL;
    long long int length;
    char sizestr[30];
    char *loc;
    int copyHereAlready = 0;
    char *disk = NULL;
    char *tmpfile;

    logMessage(3, "Modifying file %s on %s", lfn, host);

    /* Get node structure and inbox */
    se = getNode(host);
    if (!se)
    {
	logMessage(ERROR, "Error getting node structure for %s", host);
	return 0;
    }

    inbox = getNodeInbox(host);
    if (asprintf(&fullSrcPath, "%s/%s", inbox, lfn) < 0)
    {
	errorExit("Out of memory in handleNewModification");
    }

    /* Find the new modification list entry */
    for (i = 0; i < numNewModifications_; i++)
    {
	if ((!strcmp(newModifications_[i].host, host)) &&
	    (!strcmp(newModifications_[i].lfn, realLfn)))
	{
	    nm = &newModifications_[i];
	}
    }
    if (!nm)
    {
	logMessage(ERROR, "Cannot find new modification for %s on %s",
		   realLfn, host);
	globus_libc_free(fullSrcPath);
	return 0;
    }

    /* Check that the file in the inbox matches the size and checksum in the list */
    result = se->digs_getChecksum(errbuf, fullSrcPath, host, &checksum, DIGS_MD5_CHECKSUM);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error getting checksum for modified file %s on %s: %s (%s)",
		   realLfn, host, digsErrorToString(result), errbuf);
	globus_libc_free(fullSrcPath);
	return 0;
    }
    result = se->digs_getLength(errbuf, fullSrcPath, host, &length);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error getting size for modified file %s on %s: %s (%s)",
		   realLfn, host, digsErrorToString(result), errbuf);
	globus_libc_free(checksum);
	globus_libc_free(fullSrcPath);
	return 0;
    }
    
    if ((strcmp(checksum, nm->md5sum)) || (length != nm->size))
    {
	logMessage(ERROR, "Checksum and size for modified file %s on %s don't match message",
		   realLfn, host);
	globus_libc_free(checksum);
	globus_libc_free(fullSrcPath);
	removeNewModification(realLfn, host);
	savePendingModList();
	return 0;
    }
    globus_libc_free(checksum);

    /* Update the RLS checksum and size to the new ones */
    setRLSAttribute(realLfn, "md5sum", nm->md5sum);
    sprintf(sizestr, "%qd", nm->size);
    setRLSAttribute(realLfn, "size", sizestr);

    /*
     * Add a "pending modification" for each other replica of the file
     */
    loc = getFirstFileLocationAll(realLfn);
    while (loc)
    {
	if (!strcmp(loc, host))
	{
	    copyHereAlready = 1;
	}
	else
	{
	    addPendingModification(realLfn, loc, host);
	}

	loc = getNextFileLocation();
    }


    /*
     * Copy the file to the proper storage on the node it was uploaded to. If there's
     * already a copy of it there, overwrite it. If there's no copy, choose a disk
     * and make a copy there and add an RLS entry as for new files
     */
    if (!copyHereAlready)
    {
	char *lastslash;

	disk = chooseDataDisk(host);
	if (!disk)
	{
	    logMessage(ERROR, "Error choosing data disk for modified file %s on %s",
		       realLfn, host);
	    globus_libc_free(fullSrcPath);
	    return 0;
	}

	if (asprintf(&fullDestPath, "%s/%s/%s", getNodePath(host), disk, realLfn) < 0)
	{
	    errorExit("Out of memory in handleNewModification");
	}

	/* make sure directory already exists */
	lastslash = strrchr(fullDestPath, '/');
	*lastslash = 0;
	se->digs_mkdirtree(errbuf, host, fullDestPath);
	*lastslash = '/';
    }
    else
    {
	fullDestPath = constructFilename(host, realLfn);
    }
    
    result = se->digs_copyFromInbox(errbuf, host, lfn, fullDestPath);
    if (result != DIGS_SUCCESS)
    {
	globus_libc_free(fullSrcPath);
	globus_libc_free(fullDestPath);
	if (disk) globus_libc_free(disk);
	logMessage(ERROR, "Error copying %s from inbox on %s: %s (%s)",
		   realLfn, host, digsErrorToString(result), errbuf);
	return 0;
    }
    result = se->digs_rm(errbuf, host, fullSrcPath);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error removing %s from inbox on %s: %s (%s)",
		   realLfn, host, digsErrorToString(result), errbuf);
    }
    globus_libc_free(fullSrcPath);
    
    if (!copyHereAlready)
    {
	registerFileWithRc(host, realLfn);
	setDiskInfo(host, realLfn, disk);
	globus_libc_free(disk);
    }

    /*
     * Remove the new modification from the list
     */
    removeNewModification(realLfn, host);
    savePendingModList();

    /*
     * Try to copy the new version of the file over each of its replicas.
     * For each one, if it succeeds, remove the pending modification entry.
     * If it fails, set the node to dead for now
     */
    /* get a temporary file */
    tmpfile = getTemporaryFile();

    /* copy the modified file over it */
    if (!copyToLocal(host, fullDestPath, tmpfile))
    {
	logMessage(ERROR, "Error fetching modified file %s from %s",
		   realLfn, host);
	globus_libc_free(fullDestPath);
	return 0;
    }
    globus_libc_free(fullDestPath);

    /* loop over pending modifications, updating them */
    for (i = 0; i < numPendingModifications_; i++)
    {
	if (!strcmp(realLfn, pendingModifications_[i].lfn))
	{
	    fullDestPath = constructFilename(pendingModifications_[i].host,
					     realLfn);
	    if (!copyFromLocal(tmpfile, pendingModifications_[i].host,
			       fullDestPath))
	    {
		/* failed - make node dead */
		logMessage(ERROR, "Error updating %s on %s; setting to dead",
			   realLfn, pendingModifications_[i].host);
		addToDeadList(pendingModifications_[i].host);
	    }
	    else
	    {
		/* succeeded - remove pending modification entry */
		globus_libc_free(pendingModifications_[i].host);
		pendingModifications_[i].host = NULL;
	    }

	    globus_libc_free(fullDestPath);
	}
    }

    /* remove temporary file */
    unlink(tmpfile);
    globus_libc_free(tmpfile);

    /* prune any removed pending modifications */
    prunePendingModList();

    savePendingModList();

    return 1;
}

/***********************************************************************
*   int tryModificationsOnHost(char *host)
*    
*   Tries to perform any pending modifications on a host.
*    
*   Parameters:                                           [I/O]
*
*     host  Host to try modifications on                   I
*
*   Returns: 1 if all pending modifications cleared, 0 if not
************************************************************************/
int tryModificationsOnHost(char *host)
{
    int i;
    int allSucceeded = 1;
    char *tmpfile;
    char *srcfile;
    char *destfile;

    /*
     * Look in pending modification list for modifications on this host.
     * For each one that is found, try to copy the new copy over it. If
     * that succeeds, remove the entry from the list
     */
    for (i = 0; i < numPendingModifications_; i++)
    {
	if (!strcmp(pendingModifications_[i].host, host))
	{
	    /*
	     * Found one on this host.
	     * Get a temporary copy of the source file
	     */
	    tmpfile = getTemporaryFile();
	    srcfile = constructFilename(pendingModifications_[i].source,
					pendingModifications_[i].lfn);
	    if (!copyToLocal(pendingModifications_[i].source,
			     srcfile, tmpfile))
	    {
		logMessage(ERROR, "Error fetching modified file %s from %s",
			   pendingModifications_[i].lfn,
			   pendingModifications_[i].source);
		allSucceeded = 0;
	    }
	    else
	    {
		/*
		 * Try to upload the source file on this host
		 */
		destfile = constructFilename(host, pendingModifications_[i].lfn);
		if (!copyFromLocal(tmpfile, host, destfile))
		{
		    logMessage(ERROR, "Error copying modified file %s to %s",
			       pendingModifications_[i].lfn, host);
		    allSucceeded = 0;
		}
		else
		{
		    /* success - mark for deletion */
		    globus_libc_free(pendingModifications_[i].host);
		    pendingModifications_[i].host = NULL;
		}
		globus_libc_free(destfile);
	    }
	    globus_libc_free(srcfile);
	    unlink(tmpfile);
	    globus_libc_free(tmpfile);
	}
    }

    prunePendingModList();
    savePendingModList();

    /*
     * If all pending modifications were successful, return 1 to flag
     * that the node may be re-enabled/set to not dead
     */
    return allSucceeded;
}

/***********************************************************************
*   int addFileModification(char *lfn, char *host, char *md5sum,
*                           char *size, char *modtime)
*    
*   Adds a modification request to the new modifications list.
*    
*   Parameters:                                           [I/O]
*
*     lfn     logical file to modify                       I
*     host    host to which new version has been uploaded  I
*     md5sum  checksum of new version                      I
*     size    size of new version in bytes                 I
*     modtime timestamp for new version of file            I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int addFileModification(char *lfn, char *host, char *md5sum,
			char *size, char *modtime)
{
    newModification_t *m; 

    numNewModifications_++;
    newModifications_ = globus_libc_realloc(newModifications_,
					    numNewModifications_ * sizeof(newModification_t));

    if (!newModifications_)
    {
	errorExit("Out of memory in addFileModification");
    }

    m = &newModifications_[numNewModifications_ - 1];

    m->lfn = safe_strdup(lfn);
    m->host = safe_strdup(host);
    m->md5sum = safe_strdup(md5sum);
    m->size = strtoll(size, NULL, 10);
    m->time = safe_strdup(modtime);

    if ((!m->lfn) || (!m->host) || (!m->md5sum) || (!m->time))
    {
	errorExit("Out of memory in addFileModification");
    }

    savePendingModList();

    return 1;
}

/***********************************************************************
*   int canModify(char *lfn, char *user)
*    
*   Checks whether a user is allowed to modify a file
*    
*   Parameters:                                           [I/O]
*
*     lfn    logical file to check                         I
*     user   DN of user to check                           I
*
*   Returns: 1 if allowed, 0 if denied
************************************************************************/
int canModify(char *lfn, char *user)
{
    if (modificationsPending(lfn)) {
	return 0;
    }

    /* if user can delete file, they are also allowed to modify it */
    return canDeleteFile(lfn, user);
}

/***********************************************************************
*   int lockFile(char *lfn, char *user)
*    
*   Locks the specified file so that only the given user can modify it
*   Permission checks already done when this is called
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file to lock                   I
*     user  DN of user who will hold the lock              I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int lockFile(char *lfn, char *user)
{
    logMessage(1, "lockFile(%s,%s)", lfn, user);
    return registerAttrWithRc(lfn, "lockedby", user);
}

/***********************************************************************
*   int unlockFile(char *lfn, char *user)
*    
*   Unlocks the specified file
*   Permission checks already done when this is called
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file to unlock                 I
*     user  DN of user who held the lock                   I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int unlockFile(char *lfn, char *user)
{
    logMessage(1, "unlockFile(%s,%s)", lfn, user);
    return removeLfnAttribute(lfn, "lockedby");
}

/***********************************************************************
*   int lockDirectoryCallback(char *lfn, void *param)
*    
*   Called for each file in a directory while being locked
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file                           I
*     param DN of user who will hold the lock              I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
static int lockDirectoryCallback(char *lfn, void *param)
{
    char *user = (char *)param;
    return lockFile(lfn, user);
}

/***********************************************************************
*   int lockDirectory(char *lfn, char *user)
*    
*   Locks the specified directory so that only the given user can
*   modify any of the files in it
*   Permission checks already done when this is called
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of directory to lock              I
*     user  DN of user who will hold the lock              I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int lockDirectory(char *lfn, char *user)
{
    logMessage(1, "lockDirectory(%s,%s)", lfn, user);
    return forEachFileInDirectory(lfn, lockDirectoryCallback,
				  (void *)user);
}

typedef struct
{
    qcdgrid_hash_table_t *ht;
    char *user;
} unlockDirectoryCallbackParam_t;

/***********************************************************************
*   int unlockDirectoryCallback(char *lfn, void *param)
*    
*   Called for each file in a directory while being unlocked
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file                           I
*     param struct of type unlockDirectoryCallbackParam_t  I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
static int unlockDirectoryCallback(char *lfn, void *param)
{
    char *lockedby;
    unlockDirectoryCallbackParam_t *p;

    p = (unlockDirectoryCallbackParam_t *)param;
    lockedby = lookupValueInHashTable(p->ht, lfn);
    if (lockedby)
    {
        if (strcmp(lockedby, "(null)"))
	{
	    if (!strcmp(lockedby, p->user))
	    {
		if (!unlockFile(lfn, p->user))
		{
		    globus_libc_free(lockedby);
		    return 0;
		}
	    }
	}
	globus_libc_free(lockedby);
    }
    return 1;
}

/***********************************************************************
*   int unlockDirectory(char *lfn, char *user)
*    
*   Unlocks the specified directory
*   Permission checks already done when this is called
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of directory to unlock            I
*     user  DN of user who held the lock                   I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int unlockDirectory(char *lfn, char *user)
{
    unlockDirectoryCallbackParam_t p;
    int result;

    logMessage(1, "unlockDirectory(%s,%s)", lfn, user);

    p.ht = getAllAttributesValues("lockedby");
    p.user = user;

    result = forEachFileInDirectory(lfn, unlockDirectoryCallback, &p);

    destroyKeyAndValueHashTable(p.ht);

    return result;
}

/***********************************************************************
*   char *fileLockedBy(char *lfn);
*    
*   Returns the DN of the user who holds a lock on a particular file,
*   or NULL if the file is not locked
*    
*   Parameters:                                           [I/O]
*
*     lfn  the file whose lock is to be checked            I
*
*   Returns: DN of user locking file (caller should free), or NULL if
*            file is not locked
************************************************************************/
char *fileLockedBy(char *lfn)
{
    char *lockedby;


    logMessage(1, "fileLockedBy(%s)", lfn);

    lockedby = getRLSAttribute(lfn, "lockedby");
    if (!lockedby)
    {
	return NULL;
    }
    if (!strcmp(lockedby, "(null)"))
    {
	globus_libc_free(lockedby);
	return NULL;
    }

    return lockedby;
}

/***********************************************************************
*   int modificationsPending(char *lfn)
*    
*   Checks whether there are modifications pending for the specified
*   file
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file to check                  I
*
*   Returns: 1 if there are pending modifications, 0 if not
************************************************************************/
int modificationsPending(char *lfn)
{
    int i;
    logMessage(1, "modificationsPending(%s)", lfn);

    for (i = 0; i < numNewModifications_; i++) {
	if (!strcmp(newModifications_[i].lfn, lfn)) {
	    return 1;
	}
    }
    for (i = 0; i < numPendingModifications_; i++) {
	if (!strcmp(pendingModifications_[i].lfn, lfn)) {
	    return 1;
	}
    }

    return 0;
}

/***********************************************************************
*   int canLockFile(char *lfn, char *user)
*    
*   Checks whether the given user is allowed to lock the specified file
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file to lock                   I
*     user  DN of user who will hold the lock              I
*
*   Returns: 1 if allowed, 0 if not
************************************************************************/
int canLockFile(char *lfn, char *user)
{
    char *lockedby;

    logMessage(1, "canLockFile(%s,%s)", lfn, user);

    if (modificationsPending(lfn))
    {
	return 0;
    }

    lockedby = fileLockedBy(lfn);
    if (lockedby == NULL)
    {
	return 1;
    }

    if (!strcmp(lockedby, user))
    {
	globus_libc_free(lockedby);
	return 1;
    }
    globus_libc_free(lockedby);
    return 0;
}

/***********************************************************************
*   int canUnlockFile(char *lfn, char *user)
*    
*   Checks whether the given user is allowed to unlock the specified file
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file to unlock                 I
*     user  DN of user trying to unlock                    I
*
*   Returns: 1 if allowed, 0 if not
************************************************************************/
int canUnlockFile(char *lfn, char *user)
{
    char *lockedby;

    logMessage(1, "canUnlockFile(%s,%s)", lfn, user);

    lockedby = fileLockedBy(lfn);
    if (!lockedby)
    {
	return 0;
    }
    if (!strcmp(lockedby, user))
    {
	globus_libc_free(lockedby);
	return 1;
    }
    globus_libc_free(lockedby);
    return 0;
}

typedef struct
{
    qcdgrid_hash_table_t *ht;
    char *user;
} canLockDirectoryCallbackParam_t;

/***********************************************************************
*   int canLockDirectoryCallback(char *lfn, void *param)
*    
*   Called for each file in a directory while checking if user can lock
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file                           I
*     param struct of type canLockDirectoryCallbackParam_t I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
static int canLockDirectoryCallback(char *lfn, void *param)
{
    canLockDirectoryCallbackParam_t *p;
    char *lockedby;

    p = (canLockDirectoryCallbackParam_t *)param;
    lockedby = lookupValueInHashTable(p->ht, lfn);
    if (lockedby)
    {
        if (strcmp(lockedby, "(null)"))
	{
	    if (strcmp(lockedby, p->user))
	    {
		globus_libc_free(lockedby);
		return 0;
	    }
	}
	globus_libc_free(lockedby);
    }
    return 1;
}

/***********************************************************************
*   int canLockDirectory(char *lfn, char *user)
*    
*   Checks whether the given user is allowed to lock the specified
*   directory
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of directory to lock              I
*     user  DN of user who will hold the lock              I
*
*   Returns: 1 if allowed, 0 if not
************************************************************************/
int canLockDirectory(char *lfn, char *user)
{
    canLockDirectoryCallbackParam_t p;
    int result;

    logMessage(1, "canLockDirectory(%s,%s)", lfn, user);

    p.ht = getAllAttributesValues("lockedby");
    p.user = user;

    result = forEachFileInDirectory(lfn, canLockDirectoryCallback, &p);

    destroyKeyAndValueHashTable(p.ht);

    return result;
}

typedef struct
{
    qcdgrid_hash_table_t *ht;
    char *user;
    int canUnlock;
} canUnlockDirectoryCallbackParam_t;

/***********************************************************************
*   int canUnlockDirectoryCallback(char *lfn, void *param)
*    
*   Called for each file in a directory while checking if user can
*   unlock
*    
*   Parameters:                                             [I/O]
*
*     lfn   logical name of file                             I
*     param struct of type canUnlockDirectoryCallbackParam_t I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
static int canUnlockDirectoryCallback(char *lfn, void *param)
{
    canUnlockDirectoryCallbackParam_t *p;
    char *lockedby;

    p = (canUnlockDirectoryCallbackParam_t *)param;
    lockedby = lookupValueInHashTable(p->ht, lfn);
    if (lockedby)
    {
        if (strcmp(lockedby, "(null)"))
	{
	    if (!strcmp(lockedby, p->user))
	    {
		p->canUnlock = 1;
	    }
	}
	globus_libc_free(lockedby);
    }
    return 1;
}

/***********************************************************************
*   int canUnlockDirectory(char *lfn, char *user)
*    
*   Checks whether the given user is allowed to unlock the specified
*   directory
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of directory to unlock            I
*     user  DN of user trying to unlock                    I
*
*   Returns: 1 if allowed, 0 if not
************************************************************************/
int canUnlockDirectory(char *lfn, char *user)
{
    canUnlockDirectoryCallbackParam_t p;

    logMessage(1, "canUnlockDirectory(%s,%s)", lfn, user);

    p.ht = getAllAttributesValues("lockedby");
    p.user = user;
    p.canUnlock = 0;

    forEachFileInDirectory(lfn, canUnlockDirectoryCallback, &p);

    destroyKeyAndValueHashTable(p.ht);

    return p.canUnlock;
}
