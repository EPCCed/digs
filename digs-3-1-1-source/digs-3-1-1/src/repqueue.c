/***********************************************************************
*
*   Filename:   repqueue.c
*
*   Authors:    James Perry, Radoslaw Ostrowski (jamesp, radek)   EPCC.
*
*   Purpose:    This module provides a mechanism for queueing up
*               pending replication operations and executing them in
*               order of priority
*
*   Contents:   Functions and structures for managing replication queue
*
*   Used in:    Central control thread of QCDgrid
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

#include "repqueue.h"
#include "replica.h"
#include "node.h"
#include "misc.h"

/*
 * This structure is the basis of the replication queue, containing
 * details of one pending (or executing) replication
 */
typedef struct replicationInfo_s
{
    int id;

    int reason;                 /* Reason (see enum above) */

    /*
     * Number of copies of the file that exist on the grid currently. This
     * can be used to prioritise replications so that files of which there
     * is only one copy get replicated before files which a user requested
     * be closer, but are not in danger of actually being lost
     */
    int numCopies;

    char *lfn;                  /* Logical file name */

    char *fromNode;             /* Node file is being copied from */
    char *toNode;               /* Node file is being copied to */

    /*
     * Directory/disk name to which file is being copied
     */
    char *toDir;

    /*
     * Temporary filename on control node
     */
    char *tempName;

    /*
     * Whether waiting for file to come from source, or go to dest
     * See enum above
     */
    int stage;

    /*
     * SE handle of operation in progress
     */
    int handle;

} replicationInfo_t;

static int nextRepId_ = 0;

/*
 * Queue of replications that need to be carried out, in order of
 * priority. 
 */
static int replicationQueueLength_ = 0;
static int replicationQueueAlloced_ = 0;
static replicationInfo_t *replicationQueue_ = NULL;

/***********************************************************************
*   void addToReplicationQueue(char *from, char *to, char *lfn, int reason)
*    
*   Adds a new replication operation to the queue
*    
*   Parameters:                                                     [I/O]
*
*    from    Node to copy file from                                  I
*    to      Node to copy file to                                    I
*    lfn     Logical filename to copy                                I
*    reason  Reason for replication (see enumeration in repqueue.h)  I
*    
*   Returns: (void)
***********************************************************************/
void addToReplicationQueue(char *from, char *to, char *lfn, int reason)
{
    int i;
    int ncopies;
    int rep = -1;

    logMessage(1, "addToReplicationQueue(%s,%s,%s,%d)", from, to, lfn,
	       reason);

    /*
     * Check if this is already in the queue and don't add it again if
     * it is
     */
    for (i = 0; i < replicationQueueLength_; i++)
    {
	if ((!strcmp(replicationQueue_[i].lfn, lfn)) &&
	    (replicationQueue_[i].reason == reason))
	{

	    /*
	     * If this was a requested replication, the destination
	     * must match too
	     */
	    if (reason == REPTYPE_REQUESTED)
	    {
		if (!strcmp(replicationQueue_[i].toNode, to))
		{
		    logMessage(1, "Replication already in queue");
		    return;
		}
	    }
	    else
	    {
		/*
		 * This is not a requested replication. We should
		 * update the source and destination to the later
		 * ones as they reflect a more recent state of the
		 * grid. But only if the replication hasn't
		 * started yet...
		 */
		if (replicationQueue_[i].stage == REPSTAGE_WAITING)
		{
		    globus_libc_free(replicationQueue_[i].toNode);
		    globus_libc_free(replicationQueue_[i].fromNode);
		    globus_libc_free(replicationQueue_[i].lfn);
		    globus_libc_free(replicationQueue_[i].tempName);

		    logMessage(1, "Updating existing replication");
		    rep = i;
		    break;
		}
		return;
	    }
	}
    }

    /*
     * Get number of copies of this file
     */
    ncopies = getNumCopies(lfn, 0);

    /*
     * Expand the size of the queue if necessary
     */
    if (rep < 0)
    {
	replicationQueueLength_++;
	if (replicationQueueAlloced_ < replicationQueueLength_)
	{
	    replicationQueue_ = globus_libc_realloc(replicationQueue_,
						    replicationQueueLength_*
						    sizeof(replicationInfo_t));
	    if (!replicationQueue_)
	    {
		errorExit("Out of memory in addToReplicationQueue");
	    }
	}

	/*
	 * Work out its correct position in the priority queue
	 */
	for (i = replicationQueueLength_ - 1; i >= 1; i--)
	{
	    if (replicationQueue_[i-1].numCopies <= ncopies)
	    {
		break;
	    }
	    replicationQueue_[i] = replicationQueue_[i-1];
	}
	rep = i;

	replicationQueue_[rep].id = nextRepId_++;
    }

    /*
     * Insert new replication
     */
    replicationQueue_[rep].reason = reason;
    replicationQueue_[rep].numCopies = ncopies;
    replicationQueue_[rep].lfn = safe_strdup(lfn);
    replicationQueue_[rep].toNode = safe_strdup(to);
    replicationQueue_[rep].fromNode = safe_strdup(from);
    replicationQueue_[rep].stage = REPSTAGE_WAITING;
    replicationQueue_[rep].handle = -1;
    replicationQueue_[rep].toDir = NULL;

    /* get temp filename */
    replicationQueue_[rep].tempName = getTemporaryFile();
}

/*
 * The number of copies of each file required, for it to be considered 'safe'
 */
int getFileReplicaCount(char *lfn);

/***********************************************************************
*   void updateReplicationQueue()
*    
*   Updates all the replications in the queue, checking to see if any
*   have completed and should be removed, or if there are any FTP slots
*   free for starting new replications. This should be called
*   periodically
*    
*   Parameters:                                                     [I/O]
*
*     (none)
*    
*   Returns: (void)
***********************************************************************/
void updateReplicationQueue()
{
    int i;
    int slotsFree;
    int changed;
    int nc;

    struct storageElement *seFrom, *seTo;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;
    digs_transfer_status_t status;
    int percent;

    char *pfn;
    char *group;
    char *rlsperms, *permissions;

    logMessage(1, "updateReplicationQueue()");

    /*
     * For each replication in the queue:
     */
    slotsFree = 1;
    for (i = 0; i < replicationQueueLength_; i++)
    {
      /* get SE structs for source and destination */
      seFrom = getNode(replicationQueue_[i].fromNode);
      seTo = getNode(replicationQueue_[i].toNode);

      if (replicationQueue_[i].handle >= 0) {
	/* an operation is in progress for this one */
	if (replicationQueue_[i].stage == REPSTAGE_GETTING) {
	  /* in the get phase */
	  result = seFrom->digs_monitorTransfer(errbuf, replicationQueue_[i].handle,
						&status, &percent);
	  if (result != DIGS_SUCCESS) {
	    seFrom->digs_endTransfer(errbuf, replicationQueue_[i].handle);
	    logMessage(ERROR, "Transferring %s from %s failed: %s (%s)",
		       replicationQueue_[i].lfn, replicationQueue_[i].fromNode,
		       digsErrorToString(result), errbuf);
	    replicationQueue_[i].stage = REPSTAGE_DELETEME;
	    replicationQueue_[i].handle = -1;
	  }
	  else {
	    if (status == DIGS_TRANSFER_DONE) {
	      /* get transfer is complete */
	      result = seFrom->digs_endTransfer(errbuf, replicationQueue_[i].handle);
	      if (result != DIGS_SUCCESS) {
		logMessage(ERROR, "Transferring %s from %s failed: %s (%s)",
			   replicationQueue_[i].lfn, replicationQueue_[i].fromNode,
			   digsErrorToString(result), errbuf);
		replicationQueue_[i].stage = REPSTAGE_DELETEME;
		replicationQueue_[i].handle = -1;
	      }
	      else {
		/* get phase completed successfully, wait to start the put */
		replicationQueue_[i].stage = REPSTAGE_WAITING2;
		replicationQueue_[i].handle = -1;
	      }
	    }
	  }
	}
	else {
	  /* in the put phase */
	  result = seTo->digs_monitorTransfer(errbuf, replicationQueue_[i].handle,
					      &status, &percent);
	  if (result != DIGS_SUCCESS) {
	    seTo->digs_endTransfer(errbuf, replicationQueue_[i].handle);
	    logMessage(ERROR, "Transferring %s to %s failed: %s (%s)",
		       replicationQueue_[i].lfn, replicationQueue_[i].toNode,
		       digsErrorToString(result), errbuf);
	    replicationQueue_[i].stage = REPSTAGE_DELETEME;
	    replicationQueue_[i].handle = -1;
	  }
	  else {
	    if (status == DIGS_TRANSFER_DONE) {
	      /* put transfer is complete */
	      result = seTo->digs_endTransfer(errbuf, replicationQueue_[i].handle);
	      if (result != DIGS_SUCCESS) {
		logMessage(ERROR, "Transferring %s to %s failed: %s (%s)",
			   replicationQueue_[i].lfn, replicationQueue_[i].toNode,
			   digsErrorToString(result), errbuf);
		replicationQueue_[i].stage = REPSTAGE_DELETEME;
		replicationQueue_[i].handle = -1;
	      }
	      else {
		/* put phase completed successfully, finalise replication */
		/* set group */
		if (safe_asprintf(&pfn, "%s/%s/%s", getNodePath(replicationQueue_[i].toNode),
				  replicationQueue_[i].toDir, replicationQueue_[i].lfn) < 0) {
		  logMessage(ERROR, "Out of memory processing replication queue");
		  return;
		}
		if (!getAttrValueFromRLS(replicationQueue_[i].lfn, "group", &group)) {
		  logMessage(3, "Using default group ukq for file %s", replicationQueue_[i].lfn);
		  group = safe_strdup("ukq");
		}
		result = seTo->digs_setGroup(errbuf, pfn, replicationQueue_[i].toNode,
					     group);
		if (result != DIGS_SUCCESS) {
		  logMessage(ERROR, "Error setting group %s for file %s on %s: %s (%s)", group,
			     replicationQueue_[i].lfn, replicationQueue_[i].toNode,
			     digsErrorToString(result), errbuf);
		}
		globus_libc_free(group);

		/* set permissions */
		if (!getAttrValueFromRLS(replicationQueue_[i].lfn, "permissions", &rlsperms)) {
		  logMessage(3, "Using default permissions private for file %s", replicationQueue_[i].lfn);
		  rlsperms = safe_strdup("private");
		}
		permissions = "0644";
		if (!strcmp(rlsperms, "private")) {
		  permissions = "0640";
		}
		globus_libc_free(rlsperms);
		result = seTo->digs_setPermissions(errbuf, pfn, replicationQueue_[i].toNode,
						   permissions);
		if (result != DIGS_SUCCESS) {
		  logMessage(ERROR, "Error setting permissions %s for file %s on %s: %s (%s)",
			     permissions, replicationQueue_[i].lfn, replicationQueue_[i].toNode,
			     digsErrorToString(result), errbuf);
		}

		globus_libc_free(pfn);

		/* Inform the replica catalogue of the file's new
		 * location */
		if (!registerFileWithRc(replicationQueue_[i].toNode,
					replicationQueue_[i].lfn)) 
		{
		  logMessage(5, "Error registering new location in "
			     "replica catalogue");
		}
		/*
		 * Add disk attribute here
		 */
		if (!setDiskInfo(replicationQueue_[i].toNode,
				 replicationQueue_[i].lfn,
				 replicationQueue_[i].toDir))
		{
		  logMessage(5, "Error setting disk attribute in "
			     "replica catalogue");
		}

		updateLastChecked(replicationQueue_[i].lfn, replicationQueue_[i].fromNode);
		updateLastChecked(replicationQueue_[i].lfn, replicationQueue_[i].toNode);

		/* done! */
		replicationQueue_[i].stage = REPSTAGE_DELETEME;
		replicationQueue_[i].handle = -1;
	      }
	    }
	  }
	}
      }

      if (slotsFree) {
	if (replicationQueue_[i].stage == REPSTAGE_WAITING) {
	  /* time to start get operation */
	  /* check this is still necessary */
	  nc = getNumCopies(replicationQueue_[i].lfn, 0);
	  if ((nc >= getFileReplicaCount(replicationQueue_[i].lfn)) &&
	      (replicationQueue_[i].reason == REPTYPE_TOOFEWCOPIES)) {
	    replicationQueue_[i].stage = REPSTAGE_DELETEME;
	  }
	  else {
	    /* start get operation */
	    pfn = constructFilename(replicationQueue_[i].fromNode, replicationQueue_[i].lfn);
	    if (!pfn) {
	      logMessage(ERROR, "Error constructing filename for %s on %s",
			 replicationQueue_[i].lfn, replicationQueue_[i].fromNode);
	      replicationQueue_[i].stage = REPSTAGE_DELETEME;
	    }
	    else {
	      result = seFrom->digs_startGetTransfer(errbuf, replicationQueue_[i].fromNode,
						     pfn, replicationQueue_[i].tempName,
						     &replicationQueue_[i].handle);
	      if (result == DIGS_SUCCESS) {
		replicationQueue_[i].stage = REPSTAGE_GETTING;
	      }
	      else {
		logMessage(ERROR, "Error starting get transfer of %s from %s: %s (%s)",
			   replicationQueue_[i].lfn, replicationQueue_[i].fromNode,
			   digsErrorToString(result), errbuf);
		replicationQueue_[i].stage = REPSTAGE_DELETEME;
	      }
	      globus_libc_free(pfn);
	    }
	  }
	}
	else if (replicationQueue_[i].stage == REPSTAGE_WAITING2) {
	  /* time to start put operation */
	  /* choose data disk */
	  replicationQueue_[i].toDir = chooseDataDisk(replicationQueue_[i].toNode);
	  if (!replicationQueue_[i].toDir) {
	    replicationQueue_[i].toDir = safe_strdup("data");
	  }

	  /* start put */
	  if (safe_asprintf(&pfn, "%s/%s/%s", getNodePath(replicationQueue_[i].toNode),
			    replicationQueue_[i].toDir, replicationQueue_[i].lfn) < 0) {
	    logMessage(ERROR, "Out of memory processing replication queue");
	    return;
	  }

	  result = seTo->digs_startPutTransfer(errbuf, replicationQueue_[i].toNode,
					       replicationQueue_[i].tempName, pfn,
					       &replicationQueue_[i].handle);
	  if (result == DIGS_SUCCESS) {
	    replicationQueue_[i].stage = REPSTAGE_PUTTING;
	  }
	  else {
	    logMessage(ERROR, "Error putting %s onto %s: %s (%s)", replicationQueue_[i].lfn,
		       replicationQueue_[i].toNode, digsErrorToString(result), errbuf);
	    replicationQueue_[i].stage = REPSTAGE_DELETEME;
	  }
	  globus_libc_free(pfn);
	}
      }
    }

    /*
     * Delete any "DELETEME" replications
     */
    do
    {
	changed = 0;
	
	for (i = 0; i < replicationQueueLength_; i++)
	{
	    if (replicationQueue_[i].stage == REPSTAGE_DELETEME)
	    {
		changed = 1;
		/* make sure the temporary file gets deleted */
		unlink(replicationQueue_[i].tempName);
		globus_libc_free(replicationQueue_[i].lfn);
		globus_libc_free(replicationQueue_[i].toNode);
		globus_libc_free(replicationQueue_[i].fromNode);
		globus_libc_free(replicationQueue_[i].tempName);
		if (replicationQueue_[i].toDir)
		{
		    globus_libc_free(replicationQueue_[i].toDir);
		}
		
		replicationQueueLength_--;

		for (; i < replicationQueueLength_; i++)
		{
		    replicationQueue_[i] = replicationQueue_[i+1];
		}

		break;
	    }
	}
    } while (changed);
}

/*
 * The list of allowed inconsistencies, in verification module
 */
extern char **allowedInconsistencies_;

/***********************************************************************
*   void buildAllowedInconsistenciesList()
*    
*   Builds the list of inconsistencies which we ignore when verifying
*   the replica catalogue. Basically it's a list of all the files that
*   are currently being replicated - they won't yet have been registered
*   with the RC, so they may have inconsistencies.
*    
*   Parameters:                                                     [I/O]
*
*     (none)
*    
*   Returns: (void)
***********************************************************************/
void buildAllowedInconsistenciesList()
{
    int i;
    int ai;

    logMessage(1, "buildAllowedInconsistenciesList()");

    /* Free the old list */
    if (allowedInconsistencies_)
    {
	i=0;
	while (allowedInconsistencies_[i])
	{
	    globus_libc_free (allowedInconsistencies_[i]);
	    i++;
	}

	globus_libc_free(allowedInconsistencies_);
	allowedInconsistencies_ = NULL;
    }

    /* Count how many entries the new list will have */
    ai = 0;
    for (i = 0; i < replicationQueueLength_; i++)
    {
	if (replicationQueue_[i].handle >= 0)
	{
	    ai++;
	}
    }

    /* If no inconsistencies now, return */
    if (!ai)
    {
	return;
    }

    /* Allocate space for this lot */
    allowedInconsistencies_ = globus_libc_malloc((ai+1) * sizeof(char*));
    if (!allowedInconsistencies_)
    {
	errorExit("Out of memory in buildAllowedInconsistenciesList");
    }

    /* Build the new allowed inconsistencies list */
    ai = 0;
    for (i = 0; i < replicationQueueLength_; i++)
    {
	if (replicationQueue_[i].handle >= 0)
	{
	    allowedInconsistencies_[ai] = safe_strdup(replicationQueue_[i].lfn);
	    if (!allowedInconsistencies_[ai])
		errorExit("Out of memory in buildAllowedInconsistenciesList");
	    ai++;
	}
    }
    allowedInconsistencies_[ai] = NULL;
}
