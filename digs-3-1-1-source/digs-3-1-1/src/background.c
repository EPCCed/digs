/***********************************************************************
*
*   Filename:   background.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Code specific to central control thread on QCDgrid
*
*   Contents:   Main function, control thread functions, utility
*               functions only used by this thread
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

/* FIXME: some of these includes probably superfluous */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include <globus_io.h>
#include <globus_common.h>

#include "node.h"
#include "replica.h"
#include "job.h"
#include "misc.h"
#include "config.h"
#include "verify.h"
#include "diskspace.h"

#include "background-delete.h"
#include "background-new.h"
#include "background-msg.h"
#include "background-permissions.h"
#include "repqueue.h"

#define TEMP_SPACE_THRESHOLD 10240

#define PROXY_LIFETIME_REQUIRED 21600

/*
 * Number of files to delete on a node with low space per iteration
 */
#define FREE_PER_ITERATION 4

/*
 * When the disk space on the grid reaches as low as this,
 * delete superfluous copies if possible to save space
 */
static int gridFreeThreshold_;

/*
 * When the disk space on the grid reaches as low as this,
 * inform the system administrator
 */
static int gridFreePanicThreshold_;

/*
 * The number of copies of each file required on the grid
 */
int copiesRequired_;

/*
 * The (maximum) number of file checksums to run each time around
 * the main loop
 */
static int maxChecksums_;

/*
 * Maximum number of files to process each iteration
 */
static int filesPerIteration_;

/*
 * Maximum number of files to count copies of each iteration
 */
static int countPerIteration_;

/*
 * Number of iterations between each complete check for new files
 */
static int newCheckFrequency_;

/*
 * Number of iterations between each replica catalogue verification
 */
static int verifyFrequency_;

/*
 * Whether modification of files by group members (rather than just
 * owner) is permitted
 */
int groupModification_;

/*
 * Whether thread should exit after current iteration. Set in response
 * to certain signals
 */
volatile int shouldExit_ = 0;


/***********************************************************************
*   int canSetReplCountFile(char *lfn, char *user)
*    
*   Checks whether the user is allowed to modify the replication count
*   of a file
*    
*   Parameters:                                           [I/O]
*
*     lfn    file to check for                             I
*     user   user to check for                             I
*
*   Returns: 1 if allowed, 0 if not
************************************************************************/
int canSetReplCountFile(char *lfn, char *user)
{
  char *submitter;

  logMessage(1, "canSetReplCountFile(%s,%s)", lfn, user);

  /*
   * Group must match if group based modification is turned on. User
   * identity must match otherwise. Administrator check is done
   * seperately
   */
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
      logMessage(1, "User %s is allowed to modify file %s (group based)",
		 user, lfn);
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
    
    logMessage(1, "Checking if user=%s can modify file=%s submitted by=%s", user, lfn, submitter);
    
    if(strcmp(user, submitter) == 0)
      {
	logMessage(1, "User is allowed to modify the file");
	globus_libc_free(submitter);
	return 1;
      }
    globus_libc_free(submitter);
  }
  
  logMessage(3, "User is NOT allowed to modify the file");
  return 0;
}

/***********************************************************************
*   int canSetReplCountDirCallback(char *lfn, void *param)
*    
*   Callback used for checking if user can modify replication count for
*   a directory
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical name of file being checked             I
*     param name of user to check                          I
*
*   Returns: 1 if allowed, 0 if not allowed
************************************************************************/
static int canSetReplCountDirCallback(char *lfn, void *param)
{
  return canSetReplCountFile(lfn, (char *)param);
}

/***********************************************************************
*   int canSetReplCountDir(char *lfn, char *user)
*    
*   Checks if a user is allowed to modify the replication count for a
*   directory
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical directory to check                     I
*     user  user identity                                  I
*
*   Returns: 1 if allowed, 0 if not allowed
************************************************************************/
int canSetReplCountDir(char *lfn, char *user)
{
  logMessage(1, "canSetReplCountDir(%s,%s)", lfn, user);

  return forEachFileInDirectory(lfn, canSetReplCountDirCallback,
				(void *)user);
}

/***********************************************************************
*   int setReplCountFile(char *lfn, int count)
*    
*   Sets the replication count for a file. Pass 0 as count to set file
*   to the default count
*    
*   Parameters:                                           [I/O]
*
*     lfn    logical file to modify count for              I
*     count  new replication count for file                I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int setReplCountFile(char *lfn, int count)
{
  char buf[50];

  logMessage(1, "setReplCountFile(%s,%d)", lfn, count);

  if (count == 0) {
    if (!removeLfnAttribute(lfn, "replcount")) {
      logMessage(5, "Error removing replcount for %s", lfn);
      return 0;
    }
  }
  else {
    sprintf(buf, "%d", count);
    if (!registerAttrWithRc(lfn, "replcount", buf)) {
      logMessage(5, "Error registering replcount for %s", lfn);
      return 0;
    }
  }

  return 1;
}

/***********************************************************************
*   int setReplCountDirCallback(char *lfn, void *param)
*    
*   Helper callback for setting replication count of a directory
*    
*   Parameters:                                           [I/O]
*
*     lfn    logical filename to set count for             I
*     param  points to new count for file                  I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
static int setReplCountDirCallback(char *lfn, void *param)
{
  int *count = (int *)param;

  return setReplCountFile(lfn, *count);
}

/***********************************************************************
*   int setReplCountDir(char *lfn, int count)
*    
*   Sets the replication count for a logical directory. Pass 0 to set
*   replication count to default
*    
*   Parameters:                                           [I/O]
*
*     lfn    logical name of directory to set count for    I
*     count  new replication count for directory           I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int setReplCountDir(char *lfn, int count)
{
  logMessage(1, "setReplCountDir(%s,%d)", lfn, count);
  return forEachFileInDirectory(lfn, setReplCountDirCallback, &count);
}

/***********************************************************************
*   int getFileReplicaCount(char *lfn)
*    
*   Gets the replication count for a logical file. Falls back to the
*   default one if no file-specific count is set
*    
*   Parameters:                                           [I/O]
*
*     lfn   logical file to get count for                  I
*
*   Returns: minimum copies required of this file
************************************************************************/
int getFileReplicaCount(char *lfn)
{
  char *rc;
  int count;
  int nn;

  logMessage(1, "getFileReplicaCount(%s)", lfn);
  
  rc = getRLSAttribute(lfn, "replcount");
  if ((rc) && (!strcmp(rc, "(null)"))) {
    count = atoi(rc);
  }
  else {
    count = copiesRequired_;
  }
  if (rc) globus_libc_free(rc);

  nn = getNumNodes();
  if (count > nn) {
      count = nn;
  }
  return count;
}

/***********************************************************************
*   int iLikeThisFile(char *destination, char *file)
*    
*   Creates a copy of a file on the given storage element if there isn't
*   one already
*    
*   Parameters:                                           [I/O]
*
*     destination  FQDN of the storage element to copy to  I
*     file         logical grid filename of file to copy   I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int iLikeThisFile(char *destination, char *file)
{
    char *source;

    logMessage(1, "iLikeThisFile(%s,%s)", destination, file);

    if (fileAtLocation(destination, file)) 
    {
	/* It's already on the desired storage element so don't do
	 * anything */
    } 
    else 
    {
	/* Find nearest copy to transfer from */
	source = getBestCopyLocation(file);
	if (!source) 
	{
	    logMessage(5, "Cannot find file %s on grid", file);
	    return 0;
	}

	/* Request a new replication */
	addToReplicationQueue(source, destination, file, REPTYPE_REQUESTED);
    }
    return 1;
}

/***********************************************************************
*   int touchDirectoryCallback(char *lfn, void *param)
*
*   Called for each file in a directory being touched
*    
*   Parameters:                                           [I/O]
*
*     lfn          logical grid file name to touch         I
*     destination  FQDN of the storage element to copy to  I
*
*   Returns: 1, to continue iteration
************************************************************************/
static int touchDirectoryCallback(char *lfn, void *param)
{
    char *dest = (char *)param;
    iLikeThisFile(dest, lfn);
    return 1;
}

/***********************************************************************
*   void touchDirectory(char *destination, char *dir)
*
*   Creates copies of all the files in a logical directory on the given
*   storage element    
*    
*   Parameters:                                           [I/O]
*
*     destination  FQDN of the storage element to copy to  I
*     dir          logical grid directory name             I
*
*   Returns: (void)
************************************************************************/
void touchDirectory(char *destination, char *dir)
{
    char *file;
    int dirlen;

    logMessage(1, "touchDirectory(%s,%s)", destination, dir);

    forEachFileInDirectory(dir, touchDirectoryCallback, destination);
}


/*
 * Array of administrators' certificate subjects. Used to check whether
 * message senders are allowed to delete files/alter the status of the
 * grid
 */
char **administrators_;
int numAdministrators_;

/*
 * E-mail address to send the status messages to
 */
char *adminEmailAddress_;

/***********************************************************************
*   void loadAdministratorNames()
*    
*   Reads all the administrators' names from the config file, storing
*   them in the array above
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: (void)
************************************************************************/
static void loadAdministratorNames()
{
    char *name;

    logMessage(1, "loadAdministratorNames()");

    /* Read administrator names (certificate subjects, actually) */
    administrators_ = NULL;
    numAdministrators_ = 0;

    name = getFirstConfigValue("miscconf", "administrator");
    while (name)
    {
	administrators_ = globus_libc_realloc(administrators_, 
					      (numAdministrators_+1)*sizeof(char*));
	if (!administrators_)
	{
	    errorExit("Out of memory in loadAdministratorNames");
	}

	administrators_[numAdministrators_] = safe_strdup(name);
	if (!administrators_[numAdministrators_])
	{
	    errorExit("Out of memory in loadAdministratorNames");
	}

	numAdministrators_++;

	logMessage(3, "`%s' is an administrator", name);

	name = getNextConfigValue("miscconf", "administrator");
    }

    /* Read e-mail address to report problems to */
    name = getFirstConfigValue("miscconf", "admin_email");
    if (!name)
    {
	adminEmailAddress_ = NULL;
	logMessage(5, "Warning: no administrator e-mail address specified. "
		   "No grid status messages will be sent");
    }
    else
    {
	adminEmailAddress_ = safe_strdup(name);
	if (!adminEmailAddress_)
	{
	    errorExit("Out of memory in loadAdministratorNames");
	}
	logMessage(3, "Administrator e-mail address is %s",
		   adminEmailAddress_);
    }
}

/*=====================================================================
 *
 * Host/file checking
 *
 *===================================================================*/

/*
 * These are used by the code which decides which copy of a file to delete.
 * Specifically, they ensure that deleting a certain copy is not going to
 * compromise the safety of the data.
 * fileSites_ is an array of all the sites which have copies of the file.
 * copiesAtSites_ is a corresponding array which holds the number of copies
 * of the file present at each site 
 */
static char **fileSites_ = NULL;
static int numFileSites_ = 0;
static int *copiesAtSites_ = NULL;


/***********************************************************************
*   void buildSiteLists(char *lfn)
*    
*   Builds a list of all the sites at which a given file is stored, and
*   how many copies are at each site, in the global variables above.
*   Used in deciding which copies of a file it would be safe to delete.
*    
*   Parameters:                                     [I/O]
*
*   lfn   Logical filename of the file in question   I
*    
*   Returns: (void)
***********************************************************************/
static void buildSiteLists(char *lfn)
{
    char *node;
    char *site;
    int i;
    int foundSiteAt;

    logMessage(1, "buildSiteLists(%s)", lfn);

    /* Iterate over all file's locations */
    node = getFirstFileLocation(lfn);
    while (node)
    {
	if ((!isNodeDead(node)) && (!isNodeRetiring(node)))
	{
	    site = getNodeSite(node);

	    if (!site)
	    {
		logMessage(5, "Node %s has no configuration entry", node);
	    }
	    else
	    {

		/* See if this node's at one of the sites already in our array */
		foundSiteAt = -1;
		for (i = 0; i < numFileSites_; i++)
		{
		    if (!strcmp(site, fileSites_[i]))
		    {
			foundSiteAt = i;
		    }
		}
		if (foundSiteAt < 0)
		{
		    /* Not in the array yet. Create a new entry for this site */
		    fileSites_ = globus_libc_realloc(fileSites_, 
						     (numFileSites_+1) * sizeof(char*));
		    copiesAtSites_ = globus_libc_realloc(copiesAtSites_, 
							 (numFileSites_+1) * sizeof(int));
		    if ((!fileSites_) || (!copiesAtSites_))
		    {
			errorExit("Out of memory in buildSiteLists");
		    }

		    fileSites_[numFileSites_] = safe_strdup(site);
		    if (!fileSites_[numFileSites_])
		    {
			errorExit("Out of memory in buildSiteLists");
		    }
		    
		    /* Only one copy found here so far */
		    copiesAtSites_[numFileSites_] = 1;
		    
		    numFileSites_++;
		}
		else
		{
		    /* Another copy found at this site */
		    copiesAtSites_[foundSiteAt]++;
		}
	    }
	}
	globus_libc_free(node);
	node=getNextFileLocation();
    }
}

/***********************************************************************
*   void destroySiteLists()
*    
*   Frees the storage used by the global arrays above
*    
*   Parameters:                                     [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
static void destroySiteLists()
{
    int i;

    logMessage(1, "destroySiteLists()");

    /* Free the arrays allocated */
    for (i = 0; i < numFileSites_; i++)
    {
	globus_libc_free(fileSites_[i]);
    }
    globus_libc_free(fileSites_);
    globus_libc_free(copiesAtSites_);

    numFileSites_ = 0;
    fileSites_ = NULL;
    copiesAtSites_ = NULL;
}

/***********************************************************************
*   int canSafelyDelete(char *lfn, char *location)
*    
*   Checks whether deleting the specified copy of the specified file
*   would violate the geographical constraints on data storage (i.e.
*   makes sure there would still be enough copies at separate sites if
*   this one was to be deleted)
*    
*   Parameters:                                         [I/O]
*
*   lfn       Logical filename of the file in question   I
*   location  FQDN of node to check                      I
*    
*   Returns: 1 if this copy can safely be deleted, 0 if it should not be
***********************************************************************/
static int canSafelyDelete(char *lfn, char *location)
{
    char *site;
    int i;
    int foundSiteAt = -1;
    int retval;

    logMessage(1, "canSafelyDelete(%s,%s)", lfn, location);

    /* don't try to delete if node is not working normally */
    if ((isNodeDead(location)) || (isNodeDisabled(location)) ||
	(isNodeRetiring(location)))
    {
	return 0;
    }

    /* Now check to see if we can delete the copy specified */
    retval = 0;

    /* If there are more sites of the file than there are copies required,
     * we can safely delete any one of the copies */
    if (numFileSites_ > getFileReplicaCount(lfn))
    {
	retval = 1;
    }
    else
    {
	/* Find which site the copy in question is at */
	site = getNodeSite(location);

	if (!site)
	{
	    logMessage(5, "Node %s not listed in configuration", location);
	    return 0;
	}

	for (i = 0; i < numFileSites_; i++)
	{
	    if (!strcmp(site, fileSites_[i]))
	    {
		foundSiteAt = i;
	    }
	}

	/* If there's more than one copy here, we can delete one */
	if (copiesAtSites_[foundSiteAt] > 1)
	{
	    retval = 1;
	}
    }

    /* Return the value */
    return retval;
}

/***********************************************************************
*   void makeFreeSpace()
*    
*   Tries to free up space on any nodes which are running low, by
*   deleting unnecessary extra copies of files.
*    
*   Parameters:                                     [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
static void makeFreeSpace()
{
    int i, j, nn;
    struct storageElement *se;
    char *node;
    char **locfiles;
    int freed = 0;
    int nc;
    char *path;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    /*
     * Loop over all nodes checking for low space condition
     */
    nn = getNumNodes();
    for (i = 0; i < nn; i++)
    {
	node = getNodeName(i);
	if ((!isNodeDisabled(node)) && (!isNodeDead(node)))
	{
	    se = getNode(node);
	    if (!se)
	    {
		logMessage(ERROR, "Error getting SE struct for host %s", node);
	    }
	    else
	    {
		if (se->freeSpace < ((long long)gridFreeThreshold_))
		{
		    /*
		     * This node is running low on space. Try to free some up
		     */
		    locfiles = listLocationFiles(node);
		    if (locfiles == NULL)
		    {
			logMessage(ERROR, "Error listing files at %s", node);
		    }
		    else
		    {
			j = 0;
			while (locfiles[j])
			{
			    /* stop if freed enough already */
			    if (freed >= FREE_PER_ITERATION) break;
			    nc = getNumCopies(locfiles[j], 0);
			    if (nc > getFileReplicaCount(locfiles[j]))
			    {
				/*
				 * There are extra copies of this file. See if
				 * we can delete the one on this node
				 */
				buildSiteLists(locfiles[j]);
				if (canSafelyDelete(locfiles[j], node))
				{
				    logMessage(ERROR, "Deleting %s from %s to make space", locfiles[j], node);
				    if (!removeFileFromLocation(node, locfiles[j]))
				    {
					logMessage(ERROR, "Error removing RLS entry for %s", locfiles[j]);
				    }
				    else
				    {
					path = constructFilename(node, locfiles[j]);
					if (!path)
					{
					    logMessage(ERROR, "constructFilename failed in makeFreeSpace");
					}
					else
					{
					    result = se->digs_rm(errbuf, node, path);
					    globus_libc_free(path);
					    if (result != DIGS_SUCCESS)
					    {
						logMessage(ERROR, "Error deleting %s on %s: %s (%s)",
							   locfiles[j], node, digsErrorToString(result),
							   errbuf);
					    }
					    else
					    {
						freed++;
					    }
					}
				    }
				}
				destroySiteLists(locfiles[j]);
			    }
			    j++;
			}
			freeLocationFileList(locfiles);
		    }
		}
	    }
	}

    }
}

/* how far we got in checking the list of files */
static int lfnListPos_ = 0;

/***********************************************************************
*   int checkFiles()
*    
*   Iterates over all the files on the grid checking that there are
*   sufficient copies of every file
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: 1 if the function made any changes to the grid (i.e. made
*            new replicas), 0 otherwise
***********************************************************************/
static int checkFiles()
{
    char *file;              /* File currently under consideration */
    char *fromHost;          /* Host being replicated from */
    char *toHost;            /* Host being replicated to */
    int nc;                  /* Number of copies of this file on the grid */
    int gridChanged;         /* Set if the function has changed the grid */
    int warnedAlready = 0;
    int fileCount;

    long long fileLen;
    long long freeTemp;     /* disk space free in temp directory */

    /* The list of files on the grid, and how far we got in checking it */
    static char **lfnList = NULL;
    static int numLfns = 0;
    static int lfnSpace = 0;
    int i;
    char *sizestr;

    logMessage(1, "checkFiles()");

    freeTemp = getFreeSpace(tmpDir_) * 1024;

    /*
     * Refresh the file list if necessary
     */
    if (numLfns <= lfnListPos_)
    {
	/* Got to the end of our list. Have to start again */
	/* First free the old list */
	if (lfnList)
	{
	    for (i = 0; i < numLfns; i++)
	    {
		globus_libc_free(lfnList[i]);
	    }
	    globus_libc_free(lfnList);
	}

	/* Now list the files again */
	lfnList = NULL;
	numLfns = 0;
	lfnSpace = 0;

	file = getFirstFile();
	while (file)
	{
	    /* Allocate more space if necessary */
	    if (numLfns >= lfnSpace)
	    {
		lfnSpace += 1000;
		lfnList = globus_libc_realloc(lfnList, lfnSpace * sizeof(char*));
		if (!lfnList)
		{
		    errorExit("Out of memory in checkFiles");
		}
	    }
	    lfnList[numLfns] = file;
	    numLfns++;
	    
	    file = getNextFile();
	}

	if (lfnListPos_ >= numLfns)
	{
	    lfnListPos_ = 0;
	}
    }

    gridChanged = 0;

    /* Check each file on the grid */

    /*
     * This counter clips the maximum files per iteration  - the counting
     * is quite slow using RLS and may limit the frequency of other
     * operations unacceptably.
     */
    fileCount = 0;

    for (i = 0; (i < filesPerIteration_ && fileCount < countPerIteration_);)
    {
	/* Check we haven't checked all yet */
	if (lfnListPos_ >= numLfns)
	{
	    break;
	}

	file = lfnList[lfnListPos_];

	/* See how many copies there are */
	nc = getNumCopies(file, 0);

	if (nc < getFileReplicaCount(file))
	{
	    logMessage(3, "Replicating %s", file);
	    if (nc == 0)
	    {
		/*
		 * If zero copies, check to see if there's a copy on a retiring node. If
		 * there is, continue with replication. If not, warn user
		 */

		if (!getNumCopies(file, GNC_COUNTRETIRING))
		{
		    /* For a file with no copies, just warn the user and don't do
		     * anything else */
		    logMessage(5, "Warning: no copies of file %s\nYou may want to run "
			       "verify-qcdgrid-rc to prune the replica catalogue "
			       "entry", file);
		    i++;
		    lfnListPos_++;
		    continue;
		}
	    }

	    /* Too few copies. Better make another one. */
	    fromHost = getFirstFileLocation(file);
	    if (!fromHost) 
	    {
		logMessage(5, "File %s has no locations! (maybe all are disabled)", 
			   file);
	    }
	    else
	    {
		i++;

		/* Find a good place to put another copy */
		if (!getAttrValueFromRLS(file, "size", &sizestr))
		{
		    logMessage(5, "Getting size attribute failed for %s", file);
		}
		else
		{
		    fileLen = strtoll(sizestr, NULL, 10);
		    globus_libc_free(sizestr);

		    if ((fileLen * 2) > freeTemp)
		    {
			logMessage(5, "Insufficient temporary disk space to replicate %s", file);
			globus_libc_free(fromHost);
			lfnListPos_++;
			continue;
		    }

		    toHost = getSuitableNodeForMirror(file, fileLen);
		    if (toHost) 
		    {
			logMessage(5, "Replicating %s from %s to %s", file, fromHost, toHost);
			
			freeTemp -= fileLen;

			/* Request a replication for this file */
			addToReplicationQueue(fromHost, toHost, file, REPTYPE_TOOFEWCOPIES);
		    } 
		    else  /* toHost==NULL */
		    {
			/* Limit the "Nowhere to put another copy" messages to one per iteration */
			if (!warnedAlready)
			{
			    logMessage(5, "No-where to put another copy of %s", file);
			    warnedAlready = 1;
			}
		    }
		}
	    }

	    if (fromHost)
		globus_libc_free(fromHost);
	}

	lfnListPos_++;
	fileCount++;
    }

    return gridChanged;
}

/*
 * This structure is used to count how many iterations each dead node has
 * been dead for, so that we can warn the user after a certain number of
 * iterations have elapsed since it last responded
 */
typedef struct deadNodeCounter_s
{
    char *name;
    int deadcount;
} deadNodeCounter_t;

static int numDeadNodeCounters = 0;
static deadNodeCounter_t *deadNodeCounters = NULL;

/***********************************************************************
*   int deadNodeCountIndex(char *node)
*    
*   Gets the index of the given node in the dead node counters array
*    
*   Parameters:                                [I/O]
*
*     node    fully qualified name of the node  I
*    
*   Returns: index of node in array, -1 if none
***********************************************************************/
static int deadNodeCountIndex(char *node)
{
    int i;
    for (i = 0; i < numDeadNodeCounters; i++)
    {
	if (!strcmp(deadNodeCounters[i].name, node))
	{
	    return i;
	}
    }
    return -1;
}

/***********************************************************************
*   int deadNodeCount(char *node)
*    
*   Adds another iteration to the node's "dead" counter and returns the
*   counter value.
*    
*   Parameters:                                [I/O]
*
*     node    fully qualified name of the node  I
*    
*   Returns: number of iterations the node has been dead for, including
*            the current one
***********************************************************************/
static int deadNodeCount(char *node)
{
    int idx;

    idx = deadNodeCountIndex(node);
    if (idx < 0)
    {
	idx = numDeadNodeCounters;
	numDeadNodeCounters++;
	deadNodeCounters = globus_libc_realloc(deadNodeCounters,
					       numDeadNodeCounters *
					       sizeof(deadNodeCounter_t));
	if (!deadNodeCounters)
	{
	    errorExit("Out of memory in deadNodeCount");
	}

	deadNodeCounters[idx].name = node;
	deadNodeCounters[idx].deadcount = 0;
    }

    deadNodeCounters[idx].deadcount++;
    return deadNodeCounters[idx].deadcount;
}

/***********************************************************************
*   void clearDeadNodeCount(char *node)
*    
*   Clears the specified node's "dead" counter, if it exists
*    
*   Parameters:                                [I/O]
*
*     node    fully qualified name of the node  I
*    
*   Returns: (void)
***********************************************************************/
static void clearDeadNodeCount(char *node)
{
    int idx;

    idx = deadNodeCountIndex(node);
    if (idx >= 0)
    {
	deadNodeCounters[idx].deadcount = 0;
    }
}

/***********************************************************************
*   long long getTotalFreeSpace()
*    
*   Contacts each node in turn, to get its used disk space and compares
* this to the disk's quota. Returns the total free space on the grid
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: Total amount of free space on the grid, in kilobytes
***********************************************************************/
static long long getTotalFreeSpace()
{
	char *fromHost; /* Name of host being processed */
	int i; /* Loop counter */
	int nn; /* Number of nodes on grid */
	long long totalFreeSpace; /* Accumulator for free space */
	long long du; /* Disk space used on current node */
	long long dq; /* Disk space quota on current node */
	long long df; /* Disk space free on current node */
	logMessage(DEBUG, "getTotalFreeSpace()");

	/* Contact each machine here  */
	nn = getNumNodes();

	if (nn == 0) {
		logMessage(FATAL, "No grid nodes defined! Check mainnodelist.conf...");
		return 0;
	}

	totalFreeSpace = 0;
	for (i = 0; i < nn; i++) { /* for each node */
		du = 0;
		fromHost = getNodeName(i);

		logMessage(DEBUG, "fromHost is %s ", fromHost);

		/* If host is flagged as disabled, don't try to contact it */
		if (isNodeDisabled(fromHost))
			continue;

		/* Get space used on the node */
		long long *usedFilespace= NULL;
		int numDisks = 0;
		int diskItr;
		usedFilespace = getFilespaceUsedOnNode(fromHost, &numDisks);
		/* for each disk on the node */
		for (diskItr = 0; diskItr<numDisks; diskItr++){
			du += usedFilespace[diskItr];
		}
		du /= 1024; /* Convert used filespace to kb*/
		du++; //round up.
		dq = getNodeTotalDiskQuota(fromHost);
		df = dq - du;
		logMessage(DEBUG, "total space used on %s is %qdk ", fromHost, du);
		logMessage(DEBUG, "total space quota on %s is %qdk ", fromHost, dq);
		logMessage(INFO, "total free space on %s is %qd k", fromHost, df);
		/* Got a number back. Put it in the host table as disk space 
		 * free 
		 */
		logMessage(WARN, "Host %s has %qdk free on disk", fromHost, df);
		setNodeDiskSpace(i, df);
		totalFreeSpace += df;
	}
	return totalFreeSpace;
}
/***********************************************************************
*   int pingNodes()
*    
*   Contacts each node in turn, checks that it's still there and 
* 	it's data directory is available. 
* 	Also maintains list of dead nodes and executes pending deletes. 
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: 1 if it could attempt to ping the nodes, 0 if failed badly.
***********************************************************************/
static int pingNodes()
{
    char *fromHost;       /* Name of host being processed */
    int i;                /* Loop counter */
    int nn;               /* Number of nodes on grid */
    int workedLastTime;
    char *emailBuffer;
    int pingWorked = 0;
    int deadcount;
    digs_error_code_t result;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];

	logMessage(DEBUG, "pingNodes()");

	/* Contact each machine here to make sure it's still up */
	nn = getNumNodes();

	if (nn == 0) {
		logMessage(FATAL, "No grid nodes defined! Check mainnodelist.conf...");
		return 0;
	}
	
	for (i = 0; i < nn; i++) {

		fromHost = getNodeName(i);

		logMessage(1, "fromHost is %s ", fromHost);
		
		/* If host is flagged as disabled, don't try to contact it */
		if (isNodeDisabled(fromHost))
			continue;

		workedLastTime = !isNodeDead(fromHost);

		/* ping node */
		struct storageElement *se;
		se = getNode(fromHost);
		if (!se)
		{
			logMessage(FATAL, "Error getting SE struct for host %s", fromHost);
			return 0;
		}
		else
		{
			pingWorked = 1;
			result = se->digs_ping(errbuf, fromHost);
			if (result != DIGS_SUCCESS)
			{
				pingWorked = 0;
				logMessage(FATAL, "Error ping failed on %s: %s (%s)",
						fromHost, digsErrorToString(result), errbuf);
			}
		}

		if ((!pingWorked) ){
			if (workedLastTime) {
				addToDeadList(fromHost);
				logMessage(FATAL, "Host %s has gone down!", fromHost);
			}

			deadcount = deadNodeCount(fromHost);
			if (deadcount == 3) {
				/* Notify administrator that a host has gone down */
				if (adminEmailAddress_) {
					if (safe_asprintf(&emailBuffer,
							"Host %s on the QCD grid is not responding\n"
								"Please check that it is working correctly.\n",
							fromHost)>=0) {
						sendEmail(adminEmailAddress_, "QCDgrid node failure",
								emailBuffer);
						globus_libc_free(emailBuffer);
					}
				}
			}
			logMessage(WARN, "Host %s is not responding", fromHost);
		} else { // ping did work

			if (!workedLastTime) {
				/* Make sure the host isn't flagged as dead */
				removeFromDeadList(fromHost);
				logMessage(FATAL, "Host %s is back", fromHost);
				clearDeadNodeCount(fromHost);
			}
			/* Execute any pending deletes and modifications for this host */
			tryDeletesOnHost(fromHost);
			tryModificationsOnHost(fromHost);
		}
	}
	return 1;
}

/* need access to this var in verify.c to save/restore it */
extern int checksumLfnListPos_;

/***********************************************************************
*   void writeControlThreadState()
*    
*   Saves the control thread's state to a file, ready for restoring
*   later
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
static void writeControlThreadState()
{
    char *filename;
    FILE *f;

    logMessage(1, "writeControlThreadState()");

    if (safe_asprintf(&filename, "%s/control-thread-state", getQcdgridPath())
	< 0)
    {
	errorExit("Out of memory in writeControlThreadState");
    }

    f = fopen(filename, "w");
    if (!f)
    {
	logMessage(5, "Error opening %s to save control thread state",
		   filename);
	globus_libc_free(filename);
	return;
    }
    globus_libc_free(filename);

    globus_libc_fprintf(f, "#\n# QCDgrid control thread saved state\n#\n\n");
    globus_libc_fprintf(f, "file_check_pos = %d\n", lfnListPos_);
    globus_libc_fprintf(f, "checksum_pos = %d\n", checksumLfnListPos_);

    fclose(f);
}

/***********************************************************************
*   void readControlThreadState()
*    
*   Reads a saved control thread state back from a file
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
static void readControlThreadState()
{
    char *filename;

    logMessage(1, "readControlThreadState()");

    if (safe_asprintf(&filename, "%s/control-thread-state", getQcdgridPath())
	< 0)
    {
	errorExit("Out of memory in readControlThreadState");
    }
    
    if (!loadConfigFile(filename, "ctstate"))
    {
	logMessage(5, "Unable to open control thread state file %s",
		   filename);
	globus_libc_fprintf(stderr, "Can't load control thread state\n");
	return;
    }
    globus_libc_free(filename);

    lfnListPos_ = getConfigIntValue("ctstate", "file_check_pos", 0);
    checksumLfnListPos_ = getConfigIntValue("ctstate", "checksum_pos", 0);

    freeConfigFile("ctstate");
}

/***********************************************************************
*   void runHousekeeping()
*    
*   Runs the housekeeping function on all nodes that are currently
*   active.
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
static void runHousekeeping()
{
    int nn;
    int i;
    char *fromHost;
    digs_error_code_t result;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];

    nn = getNumNodes();
    for (i = 0; i < nn; i++)
    {
	fromHost = getNodeName(i);
	if ((!isNodeDisabled(fromHost)) && (!isNodeDead(fromHost)))
	{
	    struct storageElement *se;
	    se = getNode(fromHost);
	    if (!se)
	    {
		logMessage(5, "Error getting SE struct for host %s", fromHost);
	    }
	    else
	    {
		result = se->digs_housekeeping(errbuf, fromHost);
		if (result != DIGS_SUCCESS)
		{
		    logMessage(5, "Error running housekeeping on %s: %s (%s)",
			       fromHost, digsErrorToString(result), errbuf);
		}
	    }
	}
    }
}

/***********************************************************************
*   void purgeInbox()
*    
*   Removes any files that more than 1 day old in the inbox of all 
*  nodes that are currently active.
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: (void)
 ***********************************************************************/
static void purgeInbox() {
	int nn;
	int i;
	int fileItr;
	char *fromHost;
	digs_error_code_t result;
	char errbuf[MAX_ERROR_MESSAGE_LENGTH];
	char ***list;
	list = globus_malloc(sizeof(char***));
	int lengthOfList = 0;
	int allFiles = 1;
	nn = getNumNodes();
	time_t modtime;

	for (i = 0; i < nn; i++) { /* for each node*/
		fromHost = getNodeName(i);
		if ((!isNodeDisabled(fromHost)) && (!isNodeDead(fromHost))) {
			struct storageElement *se;
			se = getNode(fromHost);
			if (!se) {
				logMessage(FATAL, "Error getting SE struct for host %s",
						fromHost);
			} else {
				result = se->digs_scanInbox(errbuf, fromHost, list,
						&lengthOfList, allFiles);

				if (result == DIGS_NO_INBOX) {
					logMessage(WARN, "Host %s has no inbox.", fromHost);
				} else {
					if (result != DIGS_SUCCESS) {
						logMessage(5,
								"Error running purge inbox on %s: %s (%s)",
								fromHost, digsErrorToString(result), errbuf);
					} else { // successfully got list of files from inbox
						// go through each file and check modification time.
						for (fileItr = 0; fileItr < lengthOfList; fileItr++) {
							logMessage(
									DEBUG,
									"Checking modification time of %s on host %s.",
									(*list)[fileItr], fromHost);
							result = se->digs_getModificationTime(errbuf, (*list)[fileItr], fromHost, &modtime);
							if (result != DIGS_SUCCESS) {
								logMessage(
										5,
										"Error running purge inbox on %s: %s (%s)",
										fromHost, digsErrorToString(result),
										errbuf);
							} else { // successfully got modification time
								/* see if it's old enough to safely delete - older than 1 day.*/
								if (difftime(time(NULL), modtime) > 24.0 * 60.0 * 60.0) {
									logMessage(WARN, "Removing %s", (*list)[fileItr]);
									result = se->digs_rm(errbuf, fromHost, (*list)[fileItr]);
									if (result != DIGS_SUCCESS) {
										logMessage(
												5,
												"Error running purge inbox on %s: %s (%s)",
												fromHost,
												digsErrorToString(result),
												errbuf);
									}
								}
							}
						}
						se->digs_free_string_array(list, &lengthOfList);
					}
				}
			}
		}
	}
}

/***********************************************************************
*   void backgroundProcess()
*    
*   The top level background process. Checks the nodes to make sure they 
*   are responding, checks the files to make sure there are enough copies
*   of each one, and processes messages from the other nodes
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
static void backgroundProcess()
{
    static int diskIsLow=0;
    static int iterationCounter=0;
    long long totalFreeSpace;  /* Free disk space on grid */
    char *emailBuffer;

    /* Read and deal with messages from other nodes */
    logMessage(WARN, "Processing messages from clients");
    processMessages();

    /* Check that nodes are working and get free space */
    logMessage(3, "Processing nodes");
    if (!pingNodes()){
        	shouldExit_ = 1;
        	logMessage(FATAL, "Couldn't even attempt to ping nodes.");
        	return;
        }
    totalFreeSpace = getTotalFreeSpace(); 
    logMessage(3, "total free space is: %qd", totalFreeSpace);

    if (totalFreeSpace < gridFreePanicThreshold_) 
    {
	if ((!diskIsLow) && (adminEmailAddress_))
	{
	    /* Notify administrator that disk space is very low */
	    if (safe_asprintf(&emailBuffer, "The disk space on QCD grid is running very low (%dKB).\n"
			      "Please install more disks as soon as possible\n", totalFreeSpace)>=0)
	    {
		sendEmail(adminEmailAddress_, "QCDgrid disk space low!", emailBuffer);
		globus_libc_free(emailBuffer);
	    }
	}
	diskIsLow = 1;
	logMessage(5, "Warning: free space critical! Install more disks now!");
    }
    else
    {
	diskIsLow = 0;
    }
    fflush( stderr );

    /* Check that there are at least 2 copies of each file (and try to free 
     * some space too if there's not much left) */
    logMessage(3, "Starting file checks");
    checkFiles();
    fflush( stderr );

    /* Free up some space if necessary */
    logMessage(3, "Making free space");
    makeFreeSpace();

    /* Move any new files to their correct locations */
    logMessage(3, "Handling new files");
    handleNewFiles(((iterationCounter % newCheckFrequency_) == 0));

    /* Handle permission changes on all replicas */
    logMessage(3, "Handling permission changes");
    handlePermissionsChanges();

    // call high level function here.
    logMessage(3, "Updating replications");
    updateReplicationQueue();
    fflush( stderr );

    /*
     * Every 20th time round the loop, run a consistency check and housekeeping on the grid
     */
#if 0
    if ((iterationCounter % verifyFrequency_) == 0)
    {
	logMessage(3, "Running replica catalogue verification");
	buildAllowedInconsistenciesList();
	if (runGridVerification(NULL, 0))
	{
	    /*
	    if (adminEmailAddress_)
	    {
		if (safe_asprintf(&emailBuffer, "The QCDgrid replica catalogue does not match the \n"
			          "files on the grid. Please run verify-qcdgrid-rc to repair the \n"
			          "problem.\n")>=0)
		{
		    sendEmail(adminEmailAddress_, "QCDgrid verification error", emailBuffer);
		    globus_libc_free(emailBuffer);
		}
	    }
	    */
	}
	fflush( stderr );

	/* run the housekeeping */
	runHousekeeping();
	purgeInbox();
    }
#endif

    /* Checksum the next few files */
    logMessage(3, "Running file checksums");

    int numberOfInconsistencies;
    numberOfInconsistencies = runChecksums(maxChecksums_);
    if (numberOfInconsistencies)
    {
	if (adminEmailAddress_)
	{
	    if (safe_asprintf(&emailBuffer, "Checksums on %d QCDgrid file(s) failed. "
			      "The grid was NOT stopped, but the node was disabled. Check logs for details.\n", numberOfInconsistencies)>=0)
	    {
		sendEmail(adminEmailAddress_, "QCDgrid checksum error", emailBuffer);
		globus_libc_free(emailBuffer);
	    }
	}
	
	//logMessage(5, "FATAL ERROR: checksum failed; halting grid as a precaution.");
	//fflush(stdout);
	
	// no need to exit anymore
	// just remove the corrupted copy
	//exit(1);
    }
    fflush( stderr );

    /* Record updated node information */
    /* First check for disk space */
    if (getFreeSpace(getQcdgridPath()) == ((long long) 0))
    {
	shouldExit_ = 1;
	logMessage(5, "Local disk full, exiting");
	return;
    }
    writeNodeTable();
    writeDeadList();
    writeDisabledList();
    writeRetiringList();

    writeControlThreadState();

    iterationCounter++;
}


/***********************************************************************
*   int checkProxy()
*
*   Checks whether the proxy has a reasonable length of life left
*   (i.e. 1 hour)
*    
*   Parameters:                                [I/O]
*
*     none
*    
*   Returns: 1 if proxy has >1 hour left, 0 if not
***********************************************************************/
#if 0
/*
 * Unfortunately the Globus implementation of gss_inquire_cred seems
 * to always return 0 in the lifetime field, so this neater
 * implementation of the function doesn't work.
 */
int checkProxy()
{
    gss_cred_id_t credentialHandle = GSS_C_NO_CREDENTIAL;
    OM_uint32 majorStatus;
    OM_uint32 minorStatus;
    OM_uint32 lifetime;

    majorStatus = globus_gss_assist_acquire_cred(&minorStatus,
						 GSS_C_INITIATE, /* or GSS_C_ACCEPT */
						 &credentialHandle);
    
    if (majorStatus != GSS_S_COMPLETE)
    {
	globus_libc_fprintf(stderr, "Error acquiring credential\n");
	return 0;
    }

    majorStatus=gss_inquire_cred(&minorStatus, credentialHandle,
				 NULL, &lifetime, NULL, NULL);
    if (majorStatus!=GSS_S_COMPLETE)
    {
	globus_libc_fprintf(stderr, "Error inquiring credential\n");
	return 0;
    }

    globus_libc_printf("Proxy lifetime: %d\n", lifetime);

    if (lifetime<3600)
    {
	return 0;
    }
    return 1;
}
#else
int checkProxy()
{
    FILE *f;
    char buffer[80];
    char *outputFile;
    int fd;
    char *sysBuffer;
    int timeLeft;

    logMessage(5, "Checking validity of proxy, exiting if less than %d hours.", PROXY_LIFETIME_REQUIRED/3600);

    /*
     * Don't use tmpDir_ here because this can be called before it's initialised
     */
    if (safe_asprintf(&outputFile, "/tmp/qcdgridproxinfXXXXXX") < 0)
    {
	errorExit("Out of memory in checkProxy");
    }

    /*
     * Yet another case where popen would be nice, but it doesn't seem
     * to want to play ball (probably due to Globus threading)
     */
    fd = mkstemp(outputFile);
    if (fd < 0) 
    {
	globus_libc_free(outputFile);
	logMessage(5, "Error checking proxy info: can't create temp file");
	return 1;
    }
    close(fd);

    if (safe_asprintf(&sysBuffer, "grid-proxy-info -timeleft > %s", outputFile) < 0)
    {
	errorExit("Out of memory in checkProxy");
    }
    if (system(sysBuffer) != 0)
    {
	/* if grid-proxy-info returned non-zero, there is no proxy at all */
	globus_libc_free(sysBuffer);
	unlink(outputFile);
	globus_libc_free(outputFile);
	return 0;
    }
    globus_libc_free(sysBuffer);

    f = fopen(outputFile, "r");
    if (!f)
    {
	logMessage(5, "Error checking proxy info: fopen failed");
	unlink(outputFile);
	globus_libc_free(outputFile);
	return 1;
    }

    /* new code uses grid-proxy-info's "timeleft" switch */
    /* read the single line containing the number of seconds remaining */
    fgets(buffer, 80, f);
    fclose(f);

    unlink(outputFile);
    globus_libc_free(outputFile);

    timeLeft = atoi(buffer);
    if (timeLeft < PROXY_LIFETIME_REQUIRED)
    {
	return 0;
    }
    return 1;
}
#endif

/***********************************************************************
*   void handleTermSignal(int signum)
*    
*   Installed as a signal handler for certain UNIX signals which
*   normally cause termination. Instead of exiting immediately, sets a
*   flag telling the thread to stop at the end of the current iteration.
*   This allows any configuration changes etc. to be cleanly saved.
*    
*   Parameters:                                [I/O]
*
*     signum  signal number received
*    
*   Returns: (void)
***********************************************************************/
static void handleTermSignal(int signum)
{
    switch (signum)
    {
    case SIGHUP:
	globus_libc_printf("Caught SIGHUP signal, exiting after this iteration\n");
	break;
    case SIGINT:
	globus_libc_printf("Caught SIGINT signal, exiting after this iteration\n");
	break;
    case SIGQUIT:
	globus_libc_printf("Caught SIGQUIT signal, exiting after this iteration\n");
	break;
    case SIGTERM:
	globus_libc_printf("Caught SIGTERM signal, exiting after this iteration\n");
	break;
    }

    shouldExit_ = 1;
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of background process executable. Sets everything
*   up and runs the checks
*    
*   Parameters:                                   [I/O]
*
*     "-v" or "--verbose" turns on verbose mode    I
*    
*   Returns: 1 on error. 0 if terminated in response to user signal
***********************************************************************/

int main(int argc, char *argv[])
{
    int i;
    long long tempdisk;
    int verbose = 0;
    extern int logLevel_;

    /* Parse arguments */
    for (i = 1; i < argc; i++)
    {
	if ((!strcmp(argv[i], "-v")) || (!strcmp(argv[i], "--verbose")))
	{
	    verbose = 1;
	}
	else
	{
	    globus_libc_printf("Unrecognised argument %s\n", argv[i]);
	    globus_libc_printf("Usage: %s [--verbose] [-v]\n", argv[0]);
	    return 0;
	}
    }
    
    /* Check proxy is good to go */
    if (!checkProxy())
    {
	globus_libc_fprintf(stderr, "Proxy lifetime too short, regenerating...\n");
	//	system("grid-proxy-init");
	system("voms-proxy-init -voms ildg");
    }

    /* Read config files, setup Globus, etc.*/
    if (!qcdgridInit(0)) 
    {
	return 1;
    }

    if ((verbose) && (logLevel_ > 3)) logLevel_ = 3;

    /* check disk space in temporary directory */
    tempdisk = getFreeSpace(tmpDir_);
    if (tempdisk < TEMP_SPACE_THRESHOLD)
    {
	globus_libc_fprintf(stderr, "WARNING: disk space in temporary directory is low!\n");
    }

    /* This seems to cause the program to hang on exit sometimes, so I've
     * commented it out for now. It probably all gets taken care of
     * automatically, anyway */
/*    atexit(qcdgridShutdown); */

    /* don't exit yet */
    shouldExit_ = 0;
    
    /* catch these signals to make sure state saved cleanly on exit */
    /*
     * Don't try to catch signals like SIGSEGV which indicate a
     * programming error. If they occur, data structures may be
     * corrupted and trying to save their state could do more harm
     * than good.
     */
    signal(SIGHUP, handleTermSignal);
    signal(SIGINT, handleTermSignal);
    signal(SIGQUIT, handleTermSignal);
    signal(SIGTERM, handleTermSignal);

    loadAdministratorNames();

    if (!listenForMessages()) 
    {
	return 0;
    }

    /* Restore list of files that still need to be deleted in case thread
     * was terminated with files still to remove */
    loadPendingDeleteList();

    /* Restore list of files that still need to be added in case thread
     * was terminated with files still to add */
    loadPendingAddList();

    /* Restore list of permissions that still need to be changed in case thread
     * was terminated with files still to be changed */
    loadPendingPermissionsList();

    /* Restore list of files that still need to be modified
     */
    loadPendingModList();

    /*
     * Read config information from qcdgrid.conf
     */
    gridFreeThreshold_ = getConfigIntValue("miscconf", "disk_space_low",
					   64000000);
    gridFreePanicThreshold_ = getConfigIntValue("miscconf", "disk_space_panic",
						1000000);
    copiesRequired_ = getConfigIntValue("miscconf", "min_copies", 2);
    maxChecksums_ = getConfigIntValue("miscconf", "checksums_per_iteration", 25);
    filesPerIteration_ = getConfigIntValue("miscconf", "files_per_iteration", 500);
    countPerIteration_ = getConfigIntValue("miscconf", "count_per_iteration", 2000);
    newCheckFrequency_ = getConfigIntValue("miscconf", "new_check_frequency", 5);
    verifyFrequency_ = getConfigIntValue("miscconf", "verify_frequency", 20);
    groupModification_ = getConfigIntValue("miscconf", "group_modification", 0);

    readControlThreadState();

    /* Repeat background check until a signal tells us to stop */
    while(!shouldExit_)
    {
	logMessage(5, "Starting main background loop");
	backgroundProcess();

	/* I'm not sure why this is necessary, but it seems to be */
	fflush(stdout);
	fflush(stderr);

	/* 
	 * Now check the proxy and renew it if necessary
	 */
	if (!checkProxy())
	{
	    logMessage(5, "Proxy has less than four hours to go; regenerating");
	    return 0;
	}
    }

    logMessage(5, "Exiting control thread");

    return 1;
}
