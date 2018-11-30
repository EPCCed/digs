/***********************************************************************
*
*   Filename:   client.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    QCDgrid client functions
*
*   Contents:   Functions for QCDgrid client library
*
*   Used in:    Command line utilities, APIs
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
#include <time.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "qcdgrid-client.h"
#include "replica.h"
#include "node.h"
#include "job.h"
#include "misc.h"

char **qcdgridList();
void qcdgridDestroyList(char **list);

/*=====================================================================
 *
 * Node administration functions, involving sending messages
 *
 *===================================================================*/
/***********************************************************************
*   int qcdgridRemoveNode(char *node)
*    
*   Removes a node from the QCDgrid. Only works if user is privileged
*    
*   Parameters:                                [I/O]
*
*     node  FQDN of node to remove              I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int qcdgridRemoveNode(char *node)
{
    char *msgBuffer;
    char **filenames;
    int i;
    int atRisk;
    char *location1, *location2;
    char c;

    logMessage(1, "qcdgridRemoveNode(%s)", node);

    if (!getNodePath(node))
    {
	logMessage(5, "qcdgridRemoveNode: node %s does not exist on grid",
		   node);
	return 0;
    }

    /*
     * Check whether all files that were on the node have a copy
     * somewhere else
     */
    logMessage(5, "Checking that all files on %s are mirrored elsewhere",
	       node);
    logMessage(5, "Please wait, this may take a while...");

    /*
     * Loop over all the files in the replica catalogue
     */
    filenames = qcdgridList();
    atRisk = 0;
    i = 0;
    while (filenames[i])
    {
	/*
	 * N.B.: we can't use getNumCopies here because it ignores
	 * copies on retiring nodes, and removing a retiring node is
	 * a pretty common case
	 */
	location1 = getFirstFileLocation(filenames[i]);
	location2 = getNextFileLocation();

	/*
	 * If there is only one copy of this file, check whether it's
	 * on the node being removed
	 */
	if ((location1 != NULL) && (location2 == NULL) &&
	    (!strcmp(location1, node)))
	{
	    atRisk++;

	    if (atRisk < 10)
	    {
		logMessage(5, "File %s is at risk", filenames[i]);
	    }
	    else if (atRisk == 10)
	    {
		logMessage(5, "...more files at risk...");
		logMessage(1, "File %s is at risk", filenames[i]);
	    }
	    else
	    {
		logMessage(1, "File %s is at risk", filenames[i]);
	    }
	}

	if (location1) globus_libc_free(location1);
	if (location2) globus_libc_free(location2);

	i++;
    }
    qcdgridDestroyList(filenames);

    if (atRisk == 0)
    {
	logMessage(5, "All files are duplicated, removing node...");
    }
    else
    {
	/*
	 * Warn the user if files are at risk, and get confirmation
	 * before removing the node
	 */
	globus_libc_printf("%d files are only present on %s\n", atRisk, node);
	globus_libc_printf("Are you sure you want to remove this node? (Y/N)\n");
	c = fgetc(stdin);
	if ((c != 'y') && (c != 'Y'))
	{
	    globus_libc_printf("Remove operation cancelled by user\n");
	    return 1;
	}
    }

    if (safe_asprintf(&msgBuffer, "remove %s", node) < 0) 
    {
	errorExit("qcdgridRemoveNode: out of memory");
    }
    
    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "qcdgridRemoveNode: error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);
    return 1;
}

/***********************************************************************
*   int qcdgridUnretireNode(char *node)
*    
*   Removes a QCDgrid node from the retiring list. Only works if the
*   user is privileged.
*    
*   Parameters:                                [I/O]
*
*     node  FQDN of node to unretire            I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int qcdgridUnretireNode(char *node)
{
    char *msgBuffer;

    logMessage(1, "qcdgridUnretireNode(%s)", node);

    if (!getNodePath(node))
    {
	logMessage(5, "Node %s does not exist on grid", node);
	return 0;
    }
    if (!isNodeRetiring(node))
    {
	logMessage(5, "Node %s is not retiring", node);
	return 0;
    }

    if (safe_asprintf(&msgBuffer, "unretire %s", node) < 0) 
    {
	errorExit("Out of memory in qcdgridUnretireNode");
    }

    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node in qcdgridUnretireNode");
	return 0;
    }

    globus_libc_free(msgBuffer);

    return 1;
}

/***********************************************************************
*   int qcdgridRetireNode(char *node)
*    
*   Adds a node to the retiring nodes list. A retiring node is still
*   accessible to retrieve files from but is not used to store new
*   files. Only works if the user is privileged
*    
*   Parameters:                                [I/O]
*
*     node  FQDN of node to retire              I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int qcdgridRetireNode(char *node)
{
    char *msgBuffer;

    logMessage(1, "qcdgridRetireNode(%s)", node);

    if (!getNodePath(node))
    {
	logMessage(5, "qcdgridRetireNode: node %s does not exist on grid",
		   node);
	return 0;
    }
    if (isNodeRetiring(node))
    {
	logMessage(5, "Node %s is already retiring", node);
	return 0;
    }

    if (safe_asprintf(&msgBuffer, "retire %s", node) < 0) 
    {
	errorExit("Out of memory in qcdgridRetireNode");
    }

    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);

    return 1;
}

/***********************************************************************
*   int qcdgridAddNode(char *node, char *site, char *path)
*    
*   Adds a node from the QCDgrid. Only works if user is privileged
*    
*   Parameters:                                [I/O]
*
*     node  FQDN of node to add                 I
*     site  Site at which node is located       I
*     path  Path to QCDgrid software on node    I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int qcdgridAddNode(char *node, char *site, char *path)
{
    char *msgBuffer;

    logMessage(1, "qcdgridAddNode(%s,%s,%s)", node, site, path);

    /* Check node's not already there */
    if (getNodePath(node))
    {
	logMessage(5, "Node %s is already on the grid", node);
	return 0;
    }

    /* Prepare message */
    if (safe_asprintf(&msgBuffer, "add %s %s %s", node, site, path) < 0) 
    {
	errorExit("Out of memory in qcdgridAddNode");
    }

    /* Send message to central control thread, which will take care of
     * actually adding the node */
    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);

    return 1;
}

/***********************************************************************
*   int qcdgridDisableNode(char *node)
*    
*   Temporarily disables a QCDgrid node. Only works if the user is
*   privileged
*    
*   Parameters:                                [I/O]
*
*     node  FQDN of node to disable             I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int qcdgridDisableNode(char *node)
{
    char *msgBuffer;

    logMessage(1, "qcdgridDisableNode(%s)", node);

    /* Do a couple of quick sanity checks */
    if (!getNodePath(node))
    {
	logMessage(5, "Node %s doesn't exist on the grid", node);
	return 0;
    }
    if (isNodeDisabled(node))
    {
	logMessage(5, "Node %s is already disabled", node);
	return 0;
    }

    if (safe_asprintf(&msgBuffer, "disable %s", node) < 0) 
    {
	errorExit("Out of memory in qcdgridDisableNode");
    }

    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);
    return 1;
}

/***********************************************************************
*   int qcdgridEnableNode(char *node)
*    
*   Enables a previously disabled node. Only works if the user is
*   privileged
*    
*   Parameters:                                [I/O]
*
*     node  FQDN of node to enable              I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int qcdgridEnableNode(char *node)
{
    char *msgBuffer;

    logMessage(1, "qcdgridEnableNode(%s)", node);

    if (!getNodePath(node))
    {
	logMessage(5, "Node %s does not exist on grid", node);
	return 0;
    }
    if (!isNodeDisabled(node))
    {
	logMessage(5, "Node %s is not disabled", node);
	return 0;
    }

    if (safe_asprintf(&msgBuffer, "enable %s", node) < 0)
    {
	errorExit("Out of memory in qcdgridEnableNode");
	return 0;
    }
    
    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);

    return 1;
}

/***********************************************************************
*   int qcdgridPing()
*    
*   Sends a message to the control thread to see if it is running
*    
*   Parameters:                                [I/O]
*
*     (none)
*
*   Returns: 1 if control thread responds that it is alive, 0 if not
************************************************************************/
int qcdgridPing()
{
    logMessage(1, "qcdgridPing");
    if (!sendMessageToMainNode("ping"))
    {
	return 0;
    }
    return 1;
}

/*=====================================================================
 *
 * Getting files/directories from grid
 *
 *===================================================================*/
static int copyFromStorage(char *node, char *lfn, char *pfn)
{
  char *pathBuffer;
  int result;

  pathBuffer = constructFilename(node, lfn);
  if (!pathBuffer) {
    logMessage(ERROR, "constructFilename failed for %s on %s in copyFromStorage",
	       lfn, node);
    return 0;
  }

  result = copyToLocal(node, pathBuffer, pfn);
  globus_libc_free(pathBuffer);
  return result;
}

/***********************************************************************
*   int qcdgridGetFile(char *lfn, char *pfn);
*    
*   Retrieves one file from the data grid to the local disk
*    
*   Parameters:                                [I/O]
*
*     lfn  Logical grid filename                I
*     pfn  Physical local filename              I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridGetFile(char *lfn, char *pfn)
{
    char *source;

//RADEK - change to single slash
    char *ssLFN = substituteChars(lfn, "//", "/");
//changed all references to "lfn" into "ssLFN"

    logMessage(1, "qcdgridGetFile(%s,%s)", ssLFN, pfn);

    /* Find nearest copy of file */
    source = getBestCopyLocation(ssLFN);
    if (!source) 
    {
	logMessage(5, "File %s not found on grid", ssLFN);
	return 0;
    }

    if (!makeLocalPathValid(pfn))
    {
	logMessage(5, "Error making directory structure on local file system");
	return 0;
    }

    /* Transfer it to local storage */    
    while (!copyFromStorage(source, ssLFN, pfn)) 
    {
	logMessage(3, "Error copying from %s", source);

	/* If it didn't work, try next best copy */
	source = getNextBestCopyLocation(ssLFN);
	if (!source) 
	{
	    logMessage(5, "All copies of %s are inaccessible", ssLFN);
	    return 0;
	}
    }

    logMessage(3, "File %s successfully retrieved from %s", ssLFN, source);

    globus_libc_free(ssLFN);

    return 1;
}

/***********************************************************************
*   int qcdgridGetDirectory(char *ldn, char *pdn);
*    
*   Retrieves a whole directory from the data grid to the local disk
*    
*   Parameters:                                [I/O]
*
*     ldn  Logical grid directory name          I
*     pdn  Physical local directory name        I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridGetDirectory(char *ldn, char *pdn)
{
    int dirFound = 0;
    char *dir, *lfn, *pfn;
    int result;
    int dlen;

//RADEK - change to single slash
    char *ssLDN = substituteChars(ldn, "//", "/");
//changed all references to "ldn" into "ssLDN"


    logMessage(1, "qcdgridGetDirectory(%s,%s)", ssLDN, pdn);

    dir = globus_libc_malloc(strlen(ssLDN) + 2);
    if (!dir)
    {
	errorExit("Out of memory in qcdgridGetDirectory");
    }
    strcpy(dir, ssLDN);
    if (dir[strlen(dir) - 1] != '/')
    {
	strcat(dir, "/");
    }
    
    dlen = strlen(dir);

    /*
     * Not using forEachFileInDirectory iterator here as it would
     * make construction of physical filename more complicated
     */
    lfn = getFirstFile();
    while (lfn)
    {
	if (!strncmp(lfn, dir, dlen))
	{
	    if (safe_asprintf(&pfn, "%s/%s", pdn, &lfn[dlen]) < 0)
	    {
		errorExit("Out of memory in qcdgridGetDirectory");
	    }
	    
	    logMessage(4, "%s -> %s", lfn, pfn);
	    dirFound = 1;
	    
	    result = qcdgridGetFile(lfn, pfn);
	    globus_libc_free(pfn);
	}
	globus_libc_free(lfn);
	lfn = getNextFile();
    }
    globus_libc_free(dir);
    if (!dirFound)
    {
	logMessage(5, "Directory %s not found on grid", ssLDN);
	return 0;
    }

    globus_libc_free(ssLDN);

    return 1;
}


/*=====================================================================
 *
 * Putting files/directories on grid
 *
 *===================================================================*/
/***********************************************************************
*   int copyToSEInbox(char *localFile, char *remoteHost, char *lfn)
*
*   Copies a file from the local file system to a remote SE's inbox
*    
*   Parameters:                                                     [I/O]
*
*     localFile   Full pathname to file on local machine             I
*     remoteHost  FQDN of SE to copy to                              I
*     lfn         Desired logical filename for file                  I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
static int copyToSEInbox(char *localFile, char *remoteHost, char *lfn)
{
  struct storageElement *se;
  int handle;
  time_t startTime, endTime;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  digs_error_code_t result;
  float timeout;
  int percentComplete;
  digs_transfer_status_t status;

  se = getNode(remoteHost);
  if (!se) {
    logMessage(ERROR, "Cannot find node %s", remoteHost);
    return 0;
  }

  result = se->digs_startCopyToInbox(errbuf, remoteHost, localFile,
				     lfn, &handle);
  if (result != DIGS_SUCCESS) {
	  if (result == DIGS_NO_INBOX){
		    logMessage(INFO, "Did not copy to inbox on %s: %s (%s)", remoteHost,
			       digsErrorToString(result), errbuf);
	  }
	  else{
		    logMessage(ERROR, "Error starting copy to inbox on %s: %s (%s)", remoteHost,
			       digsErrorToString(result), errbuf);
	  }
    return 0;
  }

  timeout = getNodeCopyTimeout(remoteHost);

  startTime = time(NULL);
  do {
    result = se->digs_monitorTransfer(errbuf, handle, &status, &percentComplete);
    if ((result != DIGS_SUCCESS) || (status == DIGS_TRANSFER_FAILED)) {
      logMessage(ERROR, "Error in put transfer to %s: %s (%s)", remoteHost,
		 digsErrorToString(result), errbuf);
      se->digs_endTransfer(errbuf, handle);
      return 0;
    }
    if (status == DIGS_TRANSFER_DONE) {
      break;
    }

    globus_libc_usleep(1000);

    endTime = time(NULL);
  } while (difftime(endTime, startTime) < timeout);

  result = se->digs_endTransfer(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error in end transfer to %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  return 1;
}

/***********************************************************************
*   int qcdgridPutFile(char *pfn, char *lfn, char *permissions)
*
*   Puts one file onto the data grid    
*    
*   Parameters:                                      [I/O]
*
*     pfn    Physical filename of file on local disk  I
*     lfn    Logical name for file on grid            I
*     permissions  'public' or 'private'              I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridPutFile(char *pfn, char *lfn, char *permissions)
{
    char *destination;
    char **list;
    char *msgBuffer;
    FILE *f;
    int i;
    int success = 0;

    char *DN, *tmpDN;

    char *group;
    long long size;
    unsigned char md5[16];
    char hexBuffer[40];
    
    //    char * permissions; //read from user or private by default
    // permissions = "private";
    
    time_t now;
    char * space = " ";
    char * plus  = "+";

    logMessage(1, "qcdgridPutFile(%s,%s)", pfn, lfn);

    logMessage(1, "Trying to get the DN of the user");
    if (getUserIdentity(&DN) == 0)
    {
      logMessage(5, "Error obtaining user identity");
    }
    logMessage(1, "DN of the user: %s", DN);
    logMessage(1, "Trying to get the group of the user");
    if(getUserGroup(DN, &group) == 0)
    {
      globus_libc_free(DN);
      globus_libc_free(group);
      logMessage(5, "Error obtaining user group");
      return 0;
    }
    logMessage(1, "User group: %s", group);

    /*
     * check user is allowed to submit files as this group. Control thread
     * will do the actual enforcement later but if we check here we can
     * actually tell the user what's wrong
     */
    if (!userInGroup(DN, group))
    {
      char **groups;
      int numGroups;
      logMessage(ERROR, "Error: %s not a member of group %s", DN, group);
      if (getUserGroups(DN, &numGroups, &groups)) {
	globus_libc_printf("Your groups: ");
	for (i = 0; i < numGroups; i++) {
	  globus_libc_printf("%s ", groups[i]);
	}
	globus_libc_printf("\n");
      }
      globus_libc_free(DN);
      globus_libc_free(group);
      return 0;
    }

    if (canAddFile(DN) == 0)
    {
      globus_libc_free(group);
      globus_libc_free(DN);
      logMessage(5, "User %s is not allowed to add to the grid", DN);
      return 0;
    }
    
    /* Check to make sure we can read the source file */
    f = fopen(pfn, "rb");
    if (!f)
    {
      globus_libc_free(group);
      globus_libc_free(DN);
      logMessage(5, "Unable to open file %s", pfn);
      return 0;
    }
    fclose(f);

    /* First find a suitable storage element for the file to go to */
    list = getSuitableNodeForPrimary(getFileLength(pfn));
    if (list == NULL) 
    {
      globus_libc_free(group);
      globus_libc_free(DN);
      logMessage(5, "No place to put file on grid");
      return 0;
    }

    i = 0;

    /*
     * Loop over all possible destinations until we find one that works
     */
    while ((!success) && (list[i] != NULL))
    {
	destination = list[i];

	logMessage(1, "Trying destination: %s", destination);
	
	i++;

        logMessage(1, "Checking if file already exists in RLS");

	/* Check with replica catalogue that file doesn't already exist */
	if (fileInCollection(lfn))
	{
	    logMessage(5, "Logical file %s already exists on grid", lfn);
	    globus_libc_free(group);
	    globus_libc_free(list);
	    globus_libc_free(DN);
	    return 0;
	}

	if (!copyToSEInbox(pfn, destination, lfn))
	{
	    logMessage(3, "Error copying file to storage element %s; trying another node",
		       destination);
	    continue;
	}

	success = 1;
    }

    globus_libc_free(list);

    if (!success)
    {
      globus_libc_free(group);
      globus_libc_free(DN);
      logMessage(5, "Could not put file %s on grid", lfn);
      return 0;
    }
   
    /*  Tell the control thread that new files have been added
     *  tell who the submitter is, what group he belongs to,
     *  what is the file size and checksum and permissions
     *  also submission time
     */

    size = getFileLength(pfn);
    logMessage(1, "Computing checksum");
    computeMd5Checksum(pfn, md5);
    for (i = 0; i < 16; i++)
    {
      sprintf(&hexBuffer[i*2], "%02X", md5[i]);
    }
    time(&now);

    tmpDN = substituteChars(DN, space, plus);

    //replace spaces in DN by +
    if(safe_asprintf(&msgBuffer, "putFile %s %s %s %lld %s %ld %s", 
		     lfn, group, permissions, size, hexBuffer, 
		     now, tmpDN) < 0)
    {
      errorExit("Out of memory in qcdgridPutFile");
    }
    globus_libc_free(tmpDN);
    globus_libc_free(group);
    globus_libc_free(DN);

    if (sendMessageToMainNode(msgBuffer) == 0)
    {
      errorExit("Could not contact control thread");
    }

    /* Tell main node to check this machine for new files */

    if (safe_asprintf(&msgBuffer, "check %s", destination) < 0)
    {
	errorExit("Out of memory in qcdgridPutFile");
    }
    sendMessageToMainNode(msgBuffer);
    globus_libc_free(msgBuffer);


    logMessage(3, "Put file %s on grid", lfn);

    /* Success! */
    return 1;
}

/***********************************************************************
*   int qcdgridModifyFile(char *pfn, char *lfn)
*
*   Modifies one file on the data grid
*    
*   Parameters:                                      [I/O]
*
*     pfn    Physical filename of file on local disk  I
*     lfn    Logical name of file on grid             I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridModifyFile(char *pfn, char *lfn)
{
    char *DN;
    int i;
    FILE *f;
    int success = 0;
    char **list;
    char *destination;
    time_t now;
    long long size;
    unsigned char md5[16];
    char hexBuffer[40];
    char *msgBuffer;

    logMessage(1, "qcdgridModifyFile(%s,%s)", pfn, lfn);

    
    /* Check with replica catalogue that file already exists */
    if (!fileInCollection(lfn))
    {
	logMessage(5, "Logical file %s does not exist on grid", lfn);
	return 0;
    }

    logMessage(1, "Trying to get the DN of the user");
    if (getUserIdentity(&DN) == 0)
    {
	logMessage(5, "Error obtaining user identity");
	return 0;
    }

    if (canAddFile(DN) == 0)
    {
	globus_libc_free(DN);
	logMessage(5, "User %s is not allowed to add to the grid", DN);
	return 0;
    }
    globus_libc_free(DN);
    
    /* Check to make sure we can read the source file */
    f = fopen(pfn, "rb");
    if (!f)
    {
	logMessage(5, "Unable to open file %s", pfn);
	return 0;
    }
    fclose(f);

    /* First find a suitable storage element for the file to go to */
    list = getSuitableNodeForPrimary(getFileLength(pfn));
    if (list == NULL) 
    {
	logMessage(5, "No place to put modified file on grid");
	return 0;
    }

    i = 0;

    /*
     * Loop over all possible destinations until we find one that works
     */
    while ((!success) && (list[i] != NULL))
    {
	destination = list[i];

	logMessage(1, "Trying destination: %s", destination);
	
	i++;

	if (!copyToSEInbox(pfn, destination, lfn))
	{
	    logMessage(3, "Error copying file to storage element %s; trying another node",
		       destination);
	    continue;
	}

	success = 1;
    }
    globus_libc_free(list);

    if (!success)
    {
	logMessage(5, "Could not modify file %s", lfn);
	return 0;
    }

    size = getFileLength(pfn);
    logMessage(1, "Computing checksum");
    computeMd5Checksum(pfn, md5);
    for (i = 0; i < 16; i++)
    {
	sprintf(&hexBuffer[i*2], "%02X", md5[i]);
    }
    time(&now);

    if (safe_asprintf(&msgBuffer, "modify %s %s %s %lld %d", lfn,
		      destination, hexBuffer, size, now) < 0)
    {
	errorExit("Out of memory in qcdgridModifyFile");
    }

    if (sendMessageToMainNode(msgBuffer) == 0)
    {
	errorExit("Could not contact control thread");
    }
    globus_libc_free(msgBuffer);

    /* Tell main node to check this machine for new files */

    if (safe_asprintf(&msgBuffer, "check %s", destination) < 0)
    {
	errorExit("Out of memory in qcdgridPutFile");
    }
    sendMessageToMainNode(msgBuffer);
    globus_libc_free(msgBuffer);

    logMessage(3, "Put file %s on grid", lfn);
    
    return 1;
}

/***********************************************************************
*   int qcdgridPutDirectory(char *pfn, char *lfn);
*
*   Puts a whole directory of files onto the grid, including any in
*   subdirectories. Logical filename structure on the grid is the same
*   as physical directory structure on disk.
*    
*   Parameters:                                    [I/O]
*
*     pfn  Physical filename of file on local disk  I
*     lfn  Logical name for file on grid            I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridPutDirectory(char *pfn, char *lfn, char *permissions)
{
    /* Need to recursively loop over all files within the directory, in case
     * any of them are sub-directories. In this case the logical filenames
     * on the grid will have the same directory structure as the physical
     * filenames on disk */
    struct dirent *entry;
    DIR *dir;
    char *newlfn, *newpfn;

    logMessage(1, "qcdgridPutDirectory(%s,%s)", pfn, lfn);

    /* Open directory */
    dir = opendir(pfn);
    if (!dir)
    {
	logMessage(5, "Unable to open directory %s", pfn);
	return 0;
    }

    /* Read each entry in turn */
    entry = readdir(dir);

    while (entry)
    {
	/* Work out new logical and physical filenames */
	if (safe_asprintf(&newlfn, "%s/%s", lfn, entry->d_name) < 0)
	{
	    errorExit("Out of memory in qcdgridPutDirectory");
	}
	if (safe_asprintf(&newpfn, "%s/%s", pfn, entry->d_name) < 0)
	{
	    errorExit("Out of memory in qcdgridPutDirectory");
	}

	/* Don't want to call recursively on '.' and '..' or we'll be in
	 * trouble... */
	if (entry->d_name[0] != '.')
	{
	    /* Find out if this is a subdirectory. Have to use 'stat' -
	     * checking the entry->d_type field doesn't seem to work on
	     * trumpton */
	    struct stat statbuf;

	    stat(newpfn, &statbuf);

	    /* Handle a subdirectory */
	    if (S_ISDIR(statbuf.st_mode))
	    {
		/* and call self recursively. */
		/* NOTE: readdir returns a pointer to a statically allocated
		 * buffer, so we mustn't rely on entry pointing to the same
		 * thing when the recursive call returns */
		if (!qcdgridPutDirectory(newpfn, newlfn, permissions))
		{
		    closedir(dir);
		    globus_libc_free(newpfn);
		    globus_libc_free(newlfn);
		    return 0;
		}
	    }
	    else
	    {
		/* Handle a regular file - just put it on the grid */
		logMessage(4, "%s -> %s", newpfn, newlfn);
		if (!qcdgridPutFile(newpfn, newlfn, permissions))
		{
		    logMessage(5, "Error processing file %s", newpfn);
		}
		/* Pull up-to-date space free values from main node */
		updateNodeDiskSpace();
	    }
	}
	globus_libc_free(newpfn);
	globus_libc_free(newlfn);
	entry = readdir(dir);
    }
    
    closedir(dir);
    return 1;
}

/*=====================================================================
 *
 * File/directory deleting functions
 *
 *===================================================================*/

/***********************************************************************
*   int qcdgridDeleteFile(char *lfn);
*    
*   Deletes one file from the data grid. Only works if the user is
*   privileged
*    
*   Parameters:                                [I/O]
*
*     lfn  Logical grid filename                I
*
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridDeleteFile(char *lfn)
{
    char *location;
    char *msgBuffer;

    logMessage(1, "qcdgridDeleteFile(%s)", lfn);

    /*
     * We don't really want to check the file's location, we just
     * want to make sure it exists. Better to do it here than on
     * the main node, as then we can report the error to the user
     */
    location = getFirstFileLocation(lfn);
    if (!location)
    {
	logMessage(5, "File %s does not exist on the grid", lfn);
	return 0;
    }
    globus_libc_free(location);
    
    /* Build message */
    if (safe_asprintf(&msgBuffer, "delete %s", lfn) < 0)
    {
	errorExit("Out of memory in qcdgridDeleteFile");
    }

    /*
     * Send delete message to main node. All deletes are handled by
     * the central control thread to avoid horrible race conditions
     * and so on
     */
    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);
    return 1;
}

/***********************************************************************
*   int qcdgridDeleteDirectory(char *lfn);
*    
*   Deletes a whole logical directory from the grid. Only works if the
*   user is privileged
*    
*   Parameters:                                [I/O]
*
*     lfn  Logical grid directory name          I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridDeleteDirectory(char *lfn)
{
    char *msgBuffer;
    char *location;

    logMessage(1, "qcdgridDeleteDirectory(%s)", lfn);

    /* Check it's not a regular file */
    location = getFirstFileLocation(lfn);
    if (location)
    {
	logMessage(5, "%s is a regular file", lfn);
	return 0;
    }
    
    /* Build message */
    if (safe_asprintf(&msgBuffer, "rmdir %s", lfn)<0) 
    {
	errorExit("Out of memory in qcdgridDeleteDirectory");
    }

    /*
     * Send delete message to main node. All deletes are handled by
     * the central control thread to avoid horrible race conditions
     * and so on
     */
    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }

    globus_libc_free(msgBuffer);
    return 1;
}

/*=====================================================================
 *
 * File listing functions
 *
 *===================================================================*/

/***********************************************************************
*   char **qcdgridList()
*    
*   Retrieves a list of all the files on the QCDgrid
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: pointer to a NULL-terminated array of filenames
***********************************************************************/
char **qcdgridList()
{
    char *filename;
    char **list;
    int listSize;
    int i;

    logMessage(1, "qcdgridList()");

    listSize = 1000;
    list = globus_libc_malloc(listSize*sizeof(char*));
    if (!list)
    {
	errorExit("Out of memory in qcdgridList");
    }

    i=0;

    filename = getFirstFile();
    while (filename)
    {
	list[i] = filename;
	i++;

	if (i >= (listSize-1))
	{
	    listSize += 1000;
	    list = globus_libc_realloc(list, listSize * sizeof(char*));
	    if (!list)
	    {
		errorExit("Out of memory in qcdgridList");
	    }
	}

	filename = getNextFile();
    }
    list[i] = NULL;

    return list;
}

/***********************************************************************
*   void qcdgridDestroyList(char **list)
*    
*   Destroys a list of filenames returned by qcdgridList
*    
*   Parameters:                                [I/O]
*
*     list  Pointer to the list to free         I
*    
*   Returns: (void)
***********************************************************************/
void qcdgridDestroyList(char **list)
{
    int i = 0;

    logMessage(1, "qcdgridDestroyList()");

    while(list[i])
    {
	globus_libc_free(list[i]);
	i++;
    }
    globus_libc_free(list);
}

/*=====================================================================
 *
 * File/directory touching code
 *
 *===================================================================*/

/***********************************************************************
*   int qcdgridTouchFile(char *lfn, char *dest);
*    
*   Touches a file, making it more likely to be stored on the desired
*   storage element
*    
*   Parameters:                                [I/O]
*
*     lfn  Logical grid filename to touch       I
*     dest Destination storage element's FQDN   I
*          (if NULL, a suitable one will be
*           chosen)
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridTouchFile(char *lfn, char *dest)
{
    char *msgBuffer;
    char *source;
    char **list;
    char *sizestr;
    long long fileLen;

    logMessage(1, "qcdgridTouchFile(%s,%s)", lfn, dest);

    source = getBestCopyLocation(lfn);
    if (!source)
    {
	logMessage(5, "File %s not found on grid", lfn);
	return 0;
    }

    if (dest)
    {
	/* Quick sanity check to make sure the specified destination exists */
	if (!getNodePath(dest))
	{
	    logMessage(5, "Storage element %s does not exist on grid", dest);
	    return 0;
	}
    }
    else
    {
	/* Choose a destination */

	/* Find a good place to put another copy */
	if (!getAttrValueFromRLS(lfn, "size", &sizestr))
	{
	    logMessage(5, "Getting size attribute failed for %s", lfn);
	    return 0;
	}
	else
	{
	    fileLen = strtoll(sizestr, NULL, 10);
	    globus_libc_free(sizestr);

	    /* Find nearest place to create cache copy */
	    list = getSuitableNodeForPrimary(fileLen);
	    if (!list) 
	    {
		logMessage(5, "No suitable place for cache copy of file");
		return 0;
	    }
	    dest = list[0];
	    globus_libc_free(list);
	}
    }

    if (safe_asprintf(&msgBuffer, "touch %s %s", lfn, dest)<0)
    {
	errorExit("Out of memory in qcdgridTouchFile");
    }

    if (!sendMessageToMainNode(msgBuffer))
    {
	logMessage(5, "Error sending message to main node");
	globus_libc_free(msgBuffer);
	return 0;
    }
    globus_libc_free(msgBuffer);

    return 1;
}

/***********************************************************************
*   int qcdgridTouchDirectoryCallback(char *lfn, char *dest);
*    
*   Callback used to calculate size of logical directory
*    
*   Parameters:                                [I/O]
*
*     lfn    lfn to check size of               I
*     param  pointer to running total of size   I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int qcdgridTouchDirectoryCallback(char *lfn, void *param)
{
    long long *total;
    long long size;
    char *sizestr;

    total = (long long *)param;

    if (!getAttrValueFromRLS(lfn, "size", &sizestr))
    {
	logMessage(5, "Error getting size for file %s", lfn);
	return 0;
    }

    size = strtoll(sizestr, NULL, 10);
    globus_libc_free(sizestr);

    *total += size;

    return 1;
}

/***********************************************************************
*   int qcdgridTouchDirectory(char *lfn, char *dest);
*    
*   Touches a whole logical directory, making the files therein more 
*   likely to be stored on the desired storage element
*    
*   Parameters:                                [I/O]
*
*     lfn  Logical grid directory name to touch I
*     dest Destination storage element's FQDN   I
*          (if NULL, a suitable one will be
*           chosen)
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridTouchDirectory(char *lfn, char *dest)
{
    char *msgBuffer;
    char **list;
    long long trySize;

    logMessage(1, "qcdgridTouchDirectory(%s,%s)", lfn, dest);

    if (dest)
    {
	/* Quick sanity check to make sure the specified destination exists */
	if (!getNodePath(dest))
	{
	    logMessage(5, "Storage element %s does not exist on grid", dest);
	    return 0;
	}
    }
    else
    {
	/* Choose a destination */

	trySize = 0;
	if (!forEachFileInDirectory(lfn, qcdgridTouchDirectoryCallback,
				    &trySize))
	{
	    return 0;
	}

	list = getSuitableNodeForPrimary(trySize);
	if (!list)
	{
	    logMessage(5, "No suitable place to replicate %s", lfn);
	    return 0;
	}

	dest = list[0];
	globus_libc_free(list);
    }

    if (safe_asprintf(&msgBuffer, "touchdir %s/ %s", lfn, dest)<0)
    {
	errorExit("Out of memory in qcdgridTouchDirectory");
    }

    if (!sendMessageToMainNode(msgBuffer))
    {
	globus_libc_free(msgBuffer);
	logMessage(5, "Error sending message to main node");
	return 0;
    }
    globus_libc_free(msgBuffer);

    return 1;
}


/***********************************************************************
*   int digsMakeP(char *permissions, char *lfn, int recursive)
*    
*   Asks control thread to change permissions of certain file(s)
*   Used in digs-make-public and digs-make-private, thus the name
*    
*   Parameters:                                [I/O]
*
*     permissions "private" or "public"         I
                  user group will be picked up
*     lfn  Logical grid file or dir to change   I
*     recursive 1 if dir, 0 if file             I
*    
*   Returns: 0 on success, 1 on failure
***********************************************************************/

/* needs to be initiated before digsmakeP is called
    if (!qcdgridInit(0)) 
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	return 1;
    }
    atexit(qcdgridShutdown);
*/

int digsMakeP(char *permissions, char *lfn, int recursive)
{
    
    char *msgBuffer;
    char *DN;
    char *group;
    
// check if this file exists

    if (!fileInCollection(lfn))
    {
	logMessage(5, "Error lfn=%s does NOT exist!\n", lfn);
	return 1;
    }

    // extract DN and group

    logMessage(1, "Trying to get the DN of the user");
    if (getUserIdentity(&DN) == 0)
    {
      logMessage(5, "Error obtaining user identity");
    }
    logMessage(1, "DN of the user: %s", DN);
    logMessage(1, "Trying to get the group of the user");
    if(getUserGroup(DN, &group) == 0)
    {
      logMessage(5, "Error obtaining user group");
    }
    logMessage(1, "User group: %s", group);

    //RADEK check if no problems with freeing
    globus_libc_free(DN);

    // works only for single files  not for directories!!!
    // get group from RLS to see if a user is allowed to change the file/s

    // if (s)he is then send a message to the control thread

    if (!recursive)
    {
      if(safe_asprintf(&msgBuffer, "chmod false %s %s %s", 
		       group, lfn, permissions) < 0)
      {
	errorExit("Out of memory in digsMakeP");
      }
      logMessage(1, "sending msg: %s", msgBuffer);
      if (sendMessageToMainNode(msgBuffer) == 0)
      {
	errorExit("Could not contact control thread");
      }

      //call on single file
      globus_libc_free(group);
      globus_libc_free(msgBuffer);
      return 0;
    }
    else
      {
      /* when the recurence flag was set */
      if(safe_asprintf(&msgBuffer, "chmod true %s %s %s", 
		       group, lfn, permissions) < 0)
      {
	errorExit("Out of memory in digsMakeP");
      }

      logMessage(1, "sendimg msg: %s", msgBuffer);
      if (sendMessageToMainNode(msgBuffer) == 0)
      {
	errorExit("Could not contact control thread");
      }
      // call recursively on a directory
      globus_libc_free(group);
      globus_libc_free(msgBuffer);
      return 0;
      }

    return 1;
}

