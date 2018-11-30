/***********************************************************************
*
*   Filename:   background-new.c
*
*   Authors:    James Perry, Radoslaw Ostrowski (jamesp, radek)   EPCC.
*
*   Purpose:    Code specific for moving new files onto the QCDgrid
*
*   Contents:   Functions related to moving new files onto grid
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
#include <globus_rls_client.h>

#include "background-new.h"
#include "replica.h"
#include "job.h"
#include "node.h"
#include "misc.h"


/*
 * Add operations which have been requested but have not yet been
 * done due to a host being unreachable are stored in an array of this
 * structure. Then each time a host comes back from the dead (list),
 * see if there were any pending deletes for that host and execute them.
 *
 * The list also needs to be saved to disk when it changes in case the
 * control thread goes down.
 */
typedef struct pendingAdds_s
{
    char *lfn;
    char *group;
    char *permissions;
    char *size;
    char *md5sum;
    char *time;
    char *submitter;

} pendingAdds_t;

int numPendingAdds_;
pendingAdds_t *pendingAdds_;

/***********************************************************************
*   int isInPenndingAddsList(char *lfn)
*
*   Checks if a LFN is in the pendingadds list (in memory)
*   
*   Parameters:                                           [I/O]
*
*     lfn     to check in the pendingadds list              I
*
*   Returns: 
*     
*     position in the array or -1 if not found
***********************************************************************/
int isInPenndingAddsList(char *lfn)
{
  int i;
  
  for(i=0; i<numPendingAdds_; i++)
  {
    if (!strcmp(pendingAdds_[i].lfn, lfn))
    {      
      return i;
    }
  }
  return -1;
}

/***********************************************************************
*   void savePendingAddsList()
*    
*   Saves the pending adds list into file pendingadds. It's copied
*   here whenever it changes in case this thread is terminated with
*   add operations pending
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: (void)
************************************************************************/
void savePendingAddList()
{
    char *filenameBuffer;
    FILE *f;
    int i;

    logMessage(1, "savePendingAddList()");

    if (safe_asprintf(&filenameBuffer, "%s/pendingadds", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in savePendingAddList");
    }

    f = fopen(filenameBuffer, "w");

    if (!f)
    {
    	logMessage(FATAL, "Couldn't save pending add list to %s", filenameBuffer);
    	errorExit("Could not save pending add list.");
    }
    
    for (i = 0; i < numPendingAdds_; i++)
    {
	globus_libc_fprintf(f, "%s %s %s %s %s %s %s\n", 
			    pendingAdds_[i].lfn,
			    pendingAdds_[i].group,
			    pendingAdds_[i].permissions,
			    pendingAdds_[i].size,
			    pendingAdds_[i].md5sum,
			    pendingAdds_[i].time,
			    pendingAdds_[i].submitter);
    }
    fclose(f);
    globus_libc_free(filenameBuffer);
}

/***********************************************************************
*   int loadPendingAddList()
*    
*   Loads the pending add list from the file pendingdels (if it
*   exists). Should be called as soon as the background process starts
*   up.
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: 1 on success, 0 on error
************************************************************************/
int loadPendingAddList()
{
    char *filenameBuffer;
    char *lineBuffer = NULL;
    int lineBufferSize=0;
    FILE *f;

    int i, j;
    int numParams = 1;
    char ** params = NULL;
    char *nextSpc;
    char *paramStart;

    logMessage(1, "loadPendingAddList()");

    numPendingAdds_ = 0;
    pendingAdds_ = NULL;

    if (safe_asprintf(&filenameBuffer, "%s/pendingadds", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in loadPendingAddList");
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
        
	nextSpc = strchr(lineBuffer, ' ');
        
	while (nextSpc)
	{
	    numParams++;
	    nextSpc = strchr(nextSpc + 1, ' ');

	}

	if(numParams != 7)
	{
	    logMessage(5, "Warning: malformed pending add list:\n%s", lineBuffer);
	    break;
	}

	params = globus_libc_malloc(sizeof(char*) * numParams);
	if (!params)
	{
	    errorExit("Out of memory in parseMessage loadPendingAddList");
	}

	nextSpc = lineBuffer - 1;
	i = 0;
        
	while (i < numParams)
	{
	    paramStart = nextSpc + 1;
	    nextSpc = strchr(paramStart, ' ');
	    if (nextSpc)
	    {
		*nextSpc = 0;
	    }
	   
	    params[i] = safe_strdup(paramStart);

	    if (!params[i])
	    {
		errorExit("Out of memory in parseMessage loadPendingAddList");
	    }

	    i++;
	}

	pendingAdds_ = globus_libc_realloc(pendingAdds_, (numPendingAdds_+1)
					      *sizeof(pendingAdds_t));
	if (!pendingAdds_)
	{
	    fclose(f);
	    globus_libc_free(lineBuffer);

	    if(numParams > 1)
	    {
		for (j=0; j < numParams; j++)
		{
		    logMessage(1, "freeing params %d-a", j+1);
		    globus_libc_free(params[j]);
		}	 
		logMessage(1, "freeing params final-a");
		globus_libc_free(params);
		logMessage(1, "After freeing params final-a");
	    }
	

	    return 0;
	}


	pendingAdds_[numPendingAdds_].lfn =  safe_strdup(params[0]);
	pendingAdds_[numPendingAdds_].group = safe_strdup(params[1]);
	pendingAdds_[numPendingAdds_].permissions = safe_strdup(params[2]);
	pendingAdds_[numPendingAdds_].size = safe_strdup(params[3]);
	pendingAdds_[numPendingAdds_].md5sum = safe_strdup(params[4]);
	pendingAdds_[numPendingAdds_].time = safe_strdup(params[5]);
        pendingAdds_[numPendingAdds_].submitter = safe_strdup(params[6]);

	numPendingAdds_++;        

	safe_getline(&lineBuffer, &lineBufferSize, f);

	 // reset numParams before reading another line
	 numParams = 1;
    }

    globus_libc_free(lineBuffer);
    fclose(f);


//RADEK make sure it is segfault free    
    if(numParams > 1)
    {
	for (j=0; j < numParams; j++)
	{
	    logMessage(1, "freeing params %d-b", j+1);
	    globus_libc_free(params[j]);
	}
	logMessage(1, "freeing params final-b");
	globus_libc_free(params);
	logMessage(1, "After freeing params final-b");
    }

    return 1;
}

/***********************************************************************
*   void addPendingAdd(int numParams, char ** params)
*    
*   Adds an add operation to the list of pending adds.
*    
*   Parameters:                                        [I/O]
*
*     numParams  Number of parameters                   I
*     params     An array of parameters                 I  
*
*   Returns: (void)
************************************************************************/

void addPendingAdd(int numParams, char ** params)
{
    int i;

    for(i=0; i<numParams; i++)
    {
      logMessage(1, "arg[%d] = %s", i, params[i]);
    }
 
    pendingAdds_ = globus_libc_realloc(pendingAdds_, 
					  (numPendingAdds_+1)*sizeof(pendingAdds_t));
    if (!pendingAdds_)
    {
	errorExit("Out of memory in addPendingAdd");
    }


    pendingAdds_[numPendingAdds_].lfn =  safe_strdup(params[0]);
    pendingAdds_[numPendingAdds_].group = safe_strdup(params[1]);
    pendingAdds_[numPendingAdds_].permissions = safe_strdup(params[2]);
    pendingAdds_[numPendingAdds_].size = safe_strdup(params[3]);
    pendingAdds_[numPendingAdds_].md5sum = safe_strdup(params[4]);
    pendingAdds_[numPendingAdds_].time = safe_strdup(params[5]);
    pendingAdds_[numPendingAdds_].submitter = safe_strdup(params[6]); 

      logMessage(1, "done submitter = %s", pendingAdds_[numPendingAdds_].submitter);

    if ((!pendingAdds_[numPendingAdds_].lfn)         ||
	(!pendingAdds_[numPendingAdds_].group)       ||
	(!pendingAdds_[numPendingAdds_].permissions) ||
	(!pendingAdds_[numPendingAdds_].size)        ||
	(!pendingAdds_[numPendingAdds_].md5sum)      ||
	(!pendingAdds_[numPendingAdds_].time)        ||
	(!pendingAdds_[numPendingAdds_].submitter))
    {
	errorExit("Out of memory in addPendingAdd");
    }

    numPendingAdds_++;
    savePendingAddList();
}

/***********************************************************************
*   void removePendingAdd(char *lfn)
*    
*   Removes an add operation from the list of pending adds
*    
*   Parameters:                                        [I/O]
*
*     lfn  The Logical Filename                         I
*
*   Returns: (void)
************************************************************************/
void removePendingAdd(char *lfn)
{
    int i;

    for (i=0; i<numPendingAdds_; i++)
    {
        if (!strcmp(pendingAdds_[i].lfn, lfn))
	{	
	    globus_libc_free(pendingAdds_[i].lfn);
	    globus_libc_free(pendingAdds_[i].group);
	    globus_libc_free(pendingAdds_[i].permissions);
	    globus_libc_free(pendingAdds_[i].size);
	    globus_libc_free(pendingAdds_[i].md5sum);
	    globus_libc_free(pendingAdds_[i].time);
	    globus_libc_free(pendingAdds_[i].submitter);

	    numPendingAdds_--;
	    for (; i < numPendingAdds_; i++)
	    {
		pendingAdds_[i] = pendingAdds_[i + 1];
	    }

	    pendingAdds_=globus_libc_realloc(pendingAdds_, (numPendingAdds_+1)
						*(sizeof(pendingAdds_t)));

	    if (!pendingAdds_)
	    {
		errorExit("Out of memory in removePendingAdd");
	    }

	    savePendingAddList();
	    return;
	}
    }
}

/*
 * This is a list of all the nodes which should be checked for new files
 */
int checkListLength_ = 0;
char **checkList_ = NULL;

/***********************************************************************
*   void addToCheckList(char *node)
*
*   Adds the given node to the list of nodes to check for new files
*   
*   Parameters:                                           [I/O]
*
*     node  FQDN of node to check                          I
*
*   Returns: (void)
***********************************************************************/
void addToCheckList(char *node)
{
    int i;

    logMessage(1, "addToCheckList(%s)", node);

    /*
     * See if it's already there
     */
    for (i = 0; i < checkListLength_; i++)
    {
	if (!strcmp(node, checkList_[i]))
	    return;
    }

    checkList_ = globus_libc_realloc(checkList_, (checkListLength_+1) * sizeof(char*));
    if (!checkList_)
    {
	errorExit("Out of memory in addToCheckList");
    }

    checkList_[checkListLength_] = safe_strdup(node);
    if (!checkList_[checkListLength_])
    {
	errorExit("Out of memory in addToCheckList");
    }
    checkListLength_++;
    logMessage(4, "Checking node %s for new files", node);
}


/***********************************************************************
*  void getPendingAdd(char *lfn, char **group, char **permissions, 
*                     char **size, char **md5sum, char **submitter)
*
*   Get attributes from pendingadds list (in memory)
*   
*   Parameters:                                           [I/O]
*
*     lfn     to check in the pendingadds list              I
*    other      remaining attributes                        O
*
*   Returns: 
*            void
*
*   Note that this is not thread safe
***********************************************************************/
void getPendingAdd(char *lfn, char **group, char **permissions, char **size, char **md5sum, char **submitter)
{

  int i;
  i = isInPenndingAddsList(lfn);

  *group       = pendingAdds_[i].group; 
  *permissions = pendingAdds_[i].permissions;
  *size        = pendingAdds_[i].size;
  *md5sum      = pendingAdds_[i].md5sum;
  *submitter   = substituteChars(pendingAdds_[i].submitter, "+", " ");

}


/***********************************************************************
*   int registerFileAndAttribsWithRc(char *node, char *lfn)
*    
*   Top level function for registering a new copy of a file
*   together with attributes (introducesd in DiGS 2)
*    
*   Parameters:                                                    [I/O]
*
*     node FQDN of machine to add file to                           I
*     lfn  Logical grid filename to add                             I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int registerFileAndAttribsWithRc(char *node, char *lfn)
{
    char *group, *permissions, *size, *md5sum, *submitter;
    int error = 0;

    logMessage(1, "running registerFileWithRc");
    if (!registerFileWithRc(node, lfn))
    {
	logMessage(3, "Error registering file %s at node %s", lfn, node);
	return 0;
    }

    // here also add all the attributes
    logMessage(1, "getting attributes");
    getPendingAdd(lfn, &group, &permissions, &size, &md5sum, &submitter);
    

    logMessage(1, "got group=%s, permissions=%s, size=%s, md5sum=%s, submitter=%s", group, permissions, size, md5sum, submitter);
   
    logMessage(1, "running registerAttrWithRc");
    error += registerAttrWithRc(lfn, "group", group);
    error += registerAttrWithRc(lfn, "permissions", permissions);
    error += registerAttrWithRc(lfn, "size", size);
    error += registerAttrWithRc(lfn, "md5sum", md5sum);
    error += registerAttrWithRc(lfn, "submitter", submitter);

    /* only submitter needs to be freed */
    globus_libc_free(submitter);

    if(error < 5)
    {
	logMessage(3, "registerFileAndAttribsWithRc error registering attributes");

	/* remove mapping if attribute store failed */
	removeFileFromLocation(node, lfn);
	return 0;
    }

    updateLastChecked(lfn, node);

    return 1;
}

/***********************************************************************
*   char *newToNormalName(char *newname)
*
*   Utility function to convert a new file's temporary name in the NEW/
*   directory to its permanent logical filename. This is done by 
*   substituting slashes for each occurrence of -DIR- in the name.
*    
*   Parameters:                                           [I/O]
*
*     newname  Pointer to the name in the NEW/ directory   I
*
*   Returns: real logical filename. Should be freed by the caller
***********************************************************************/
static char *newToNormalName(char *newname)
{
    char *normalName;
    int i, j;

    logMessage(1, "newToNormalName(%s)", newname);

    normalName = globus_libc_malloc(strlen(newname)+1);
    if (!normalName)
    {
	errorExit("Out of memory in newToNormalName");
    }

    i=0;
    j=0;
    while (newname[i])
    {
	/* Replace '-DIR-' with '/' */
	if (!strncmp(&newname[i], "-DIR-", 5))
	{
	    i+=5;
	    normalName[j] = '/';
	}
	else
	{
	    normalName[j] = newname[i];
	    i++;
	}

	j++;
    }
    normalName[j] = 0;
    return normalName;
}

/***********************************************************************
*   int unNewFile(char *lfn, struct storageElement *se)
*
*   Takes care of the whole operation of putting a new file onto the
*   grid. Copies the file from the inbox directory of the storage
*   element to its final destination, and updates the replica catalogue
*   to reflect this change.
*    
*   Parameters:                                           [I/O]
*
*     lfn    file's current logical name                   I
*     se     storage element containing file               I
*
*   Returns: 1 on success, 0 on error
***********************************************************************/
static int unNewFile(char *lfn, struct storageElement *se)
{
    /*
     * 1. copy data/NEW/lfn to data/real lfn
     * 2. remove data/NEW/lfn
     * 3. add RC entry for data/real lfn
     */
    char *fullSrcPath;
    char *realLfn;
    char *fullDestPath;
    char *disk;

    char *host;
    char *lastSlash;

    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    host = se->name;

    logMessage(1, "unNewFile(%s,%s)", lfn, host);

    /* First convert the file's temporary name to its permanent logical name */
    realLfn = newToNormalName(lfn);
    if (!realLfn)
    {
	return 0;
    }

    /* Check if this LFN is already in the pendingadds list
       if it is NOT don't add this file to the grid */
    if (isInPenndingAddsList(realLfn) == -1)
    {
	/* Check if it's a modification */
	if (isNewModification(realLfn, host))
	{
	    if (!handleNewModification(lfn, realLfn, host))
	    {
		logMessage(5, "Error modifying file %s on %s", realLfn, host);
		globus_libc_free(realLfn);
		return 0;
	    }
	    globus_libc_free(realLfn);
	    return 1;
	}

	logMessage(5, "File %s was not found in the pending list! Not adding.", realLfn);
	globus_libc_free(realLfn);
	return 0;
    }
	logMessage(1, "File %s was found in the pending list on position %d!", realLfn, isInPenndingAddsList(realLfn));


    /* Check if this LFN is already in the catalogue, do nothing if it is */
    if (fileInCollection(realLfn))
    {
	logMessage(5, "New file %s is already in catalogue", realLfn);
	globus_libc_free(realLfn);
	return 0;
    }

    /* Get full paths to file's current location and new location */
    if (safe_asprintf(&fullSrcPath, "%s/%s", getNodeInbox(host), lfn)<0)
    {
	errorExit("Out of memory in unNewFile");
    }

    /*
     * Decide which disk to put it on if multidisk SE
     */
    disk = chooseDataDisk(host);
    if (!disk)
    {
	logMessage(5, "Unable to choose data disk for new file %s on %s",
		   realLfn, host);
	globus_libc_free(realLfn);
	globus_libc_free(fullSrcPath);
	return 0;
    }

    if (safe_asprintf(&fullDestPath, "%s/%s/%s", getNodePath(host), disk,
		      realLfn)<0)
    {
	errorExit("Out of memory in unNewFile");
    }

    /* Make sure any subdirectories in which the new file resides already
     * exist */
    lastSlash = strrchr(fullDestPath, '/');
    *lastSlash = 0;
    /* will fail if already exists, so ignore result */
    se->digs_mkdirtree(errbuf, host, fullDestPath);
    *lastSlash = '/';

    /* copy file into place */
    result = se->digs_copyFromInbox(errbuf, host, lfn, fullDestPath);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error moving file %s to destination on %s: %s (%s)",
		   realLfn, host, digsErrorToString(result), errbuf);
	globus_libc_free(fullDestPath);
	globus_libc_free(fullSrcPath);
	globus_libc_free(realLfn);
	globus_libc_free(disk);
	return 0;
    }
	/* delete the file from the inbox. */
	result = se->digs_rm(errbuf, host, fullSrcPath);
	if (result != DIGS_SUCCESS) {
		logMessage(ERROR, "Error deleting file %s from on %s: %s (%s)",
				fullSrcPath, host, digsErrorToString(result), errbuf);
		globus_libc_free(fullDestPath);
		globus_libc_free(fullSrcPath);
		globus_libc_free(realLfn);
		globus_libc_free(disk);
		return 0;
	}
	globus_libc_free(fullSrcPath);


    //RADEK - maybe here I should run chown and chmod on remote file?
	/* Change its permissions */
	// what if a user has no rights?

    // HERE GET PERMISSIONS AND GROUP FROM PENDING-ADDS
    char *group, *permissions, *size, *md5sum, *submitter;

    getPendingAdd(realLfn, &group, &permissions, &size, &md5sum, &submitter);

    // don't need it (submitter is the only one that has to be freed)
    globus_libc_free(submitter);
    
    logMessage(1, "got group=%s, permissions=%s", group, permissions);

    logMessage(1, "chmod and chown Remotely(%s, qcdgrid, %s, 0, %s, %s)", 
	       permissions, group, fullDestPath, host);

    if(!chownRemotely("qcdgrid", group, fullDestPath, host))
    {
      logMessage(5, "Error changing permissions of the file");
      /*globus_libc_free(fullDestPath);
	globus_libc_free(disk);
	globus_libc_free(realLfn);
	globus_libc_fprintf(stderr, "[%s] Error executing remote chown\n", textTime());
	return 0;*/
    }

    if(!chmodRemotely(permissions, fullDestPath, host))
    {
      logMessage(5, "Error changing permissions of the file");
	globus_libc_free(disk);
	globus_libc_free(realLfn);
	globus_libc_free(fullDestPath);
	globus_libc_fprintf(stderr, "[%s] Error executing remote chmod \n", textTime());
      return 0;
    }

    // RADEK: replaced with registerFileAndAttribsWithRc
    /* Register the file's final location with the replica catalogue. */
    //if (!registerFileWithRc(host, realLfn))

    logMessage(1, "Registering files and attributes with RLS");
    if (!registerFileAndAttribsWithRc(host, realLfn))
    {
	/* don't leave the copy lying around without a catalogue entry */
	se->digs_rm(errbuf, host, fullDestPath);

	logMessage(5, "Error adding RLS entry for %s on %s", host, realLfn);
	globus_libc_free(disk);
	globus_libc_free(realLfn);
	globus_libc_free(fullDestPath);
	globus_libc_fprintf(stderr, "[%s] Error adding RC entry\n", textTime());
	return 0;
    }
    globus_libc_free(fullDestPath);

    if (!setDiskInfo(host, realLfn, disk))
    {
	logMessage(5, "Error setting disk attribute to %s for %s on %s", disk,
		   realLfn, host);
    }

    globus_libc_free(disk);

    /* Also remove entry in pendingadds list */
    removePendingAdd(realLfn);
    globus_libc_free(realLfn);

    return 1;
}

/***********************************************************************
*   int checkNodeForNewFiles(char *node)
*
*   Scans a storage element to see if there are any files in its inbox
*   directory, and moves them to their proper locations if it finds
*   any
*    
*   Parameters:                                           [I/O]
*
*     node  FQDN of storage element to check               I
*
*   Returns: 1 on success, 0 on error
***********************************************************************/
static int checkNodeForNewFiles(char *node)
{
    struct storageElement *se;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;
    char **list;
    int listLength;
    int i;
    char *inbox, *ibx;
    int inboxLength;

    logMessage(1, "checkNodeForNewFiles(%s)", node);

    /* Don't bother scanning for new files on dead or disabled nodes */
    if ((isNodeDead(node)) || (isNodeDisabled(node)))
    {
	return 1;
    }

    /* or if the node doesn't have an inbox */
    ibx = getNodeInbox(node);
    if (!ibx)
    {
	return 1;
    }
    if (ibx[strlen(ibx) - 1] != '/')
    {
	if (safe_asprintf(&inbox, "%s/", ibx) < 0)
	{
	    logMessage(ERROR, "Out of memory in checkNodeForNewFiles");
	    return 0;
	}
    }
    else
    {
	inbox = safe_strdup(ibx);
	if (!inbox)
	{
	    logMessage(ERROR, "Out of memory in checkNodeForNewFiles");
	    return 0;
	}
    }

    se = getNode(node);
    if (!se)
    {
	logMessage(ERROR, "Error getting node structure for %s", node);
	globus_libc_free(inbox);
	return 0;
    }

    /* do the scan */
    result = se->digs_scanInbox(errbuf, node, &list, &listLength, 0);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error scanning for new files on %s: %s (%s)",
		   node, digsErrorToString(result), errbuf);
	globus_libc_free(inbox);
	return 0;
    }

    inboxLength = strlen(inbox);
    for (i = 0; i < listLength; i++)
    {
	/* found a file in the inbox */
	unNewFile(&list[i][inboxLength], se);
    }

    globus_libc_free(inbox);
    se->digs_free_string_array(&list, &listLength);
    return 1;
}

/***********************************************************************
*   int handleNewFiles(int checkAll)
*
*   Top level function for dealing with new files. Should be called
*   each time round the control thread's main loop. Normally only
*   checks those nodes which have been specifically put on the check
*   list, but every 5th iteration the calling function passes a 1 and
*   all the grid nodes are checked. This avoids a situation where a
*   new file ends up "stranded" in the NEW/ directory, unknown to the
*   grid
*    
*   Parameters:                                           [I/O]
*
*     checkAll  0 means check only specified nodes
*               1 means check all grid nodes               I
*
*   Returns: 1 on success, 0 on error
***********************************************************************/
int handleNewFiles(int checkAll)
{
    int i;
    int nn;

    logMessage(1, "handleNewFiles(%d)", checkAll);

    /* See if it's time to check all the nodes yet */
    if (!checkAll)
    {
	/* No, just do the ones on the check list */
	for (i = 0; i < checkListLength_; i++)
	{
	    checkNodeForNewFiles(checkList_[i]);
	}
    }
    else
    {
	/* Check every grid node */
	nn = getNumNodes();

	for (i = 0; i < nn; i++)
	{
	    checkNodeForNewFiles(getNodeName(i));
	}
    }

    /* Empty the check list ready for next iteration */
    if (checkList_)
    {
	for (i = 0; i < checkListLength_; i++)
	{
	    globus_libc_free(checkList_[i]);
	}
	globus_libc_free(checkList_);
	checkList_ = NULL;
    }
    checkListLength_ = 0;

    return 1;
}

