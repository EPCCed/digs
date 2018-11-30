/***********************************************************************
*
*   Filename:   node.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Manages information about nodes on the QCDgrid
*
*   Contents:   Data structures storing node information, functions to
*               access and manipulate this information
*
*   Used in:    Called by other modules to get/set information about
*               grid nodes
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003-2010 The University of Edinburgh
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

#include "node.h"
#include "replica.h"
#include "config.h"
#include "gridftp.h"

#ifdef OMERO
#include "omero.h"
#endif

#ifdef WITH_SRM
#include "srm.h"
#endif

/* FQDN of the main node of the system (the one on which the control thread
   runs). The two pointers are there for historical reasons */
char *primaryNode_=NULL;
char *mainNode_=NULL;


char *mdcLocation_=NULL;

/*
 * FQDN of this machine
 */
char thisHostname_[256];

/*
 * Path to temporary directory to use
 */
char *tmpDir_=NULL;

/*
 * The relative importance given to a node's location and free space
 * when choosing where to put files
 */
int locationWeight_;
int spaceWeight_;

/***********************************************************************
*   int copyFromControlNode(char *remoteFile, char *localFile)
*    
*   Copy a file from the control node to the local machine. Used in
*   startup code before the node structures are initialised. Always
*   calls Globus versions of the functions.
*    
*   Parameters:                                                    [I/O]
*
*     remoteFile   full path of file to copy on control node        I
*     localFile    name to save file as on local machine            I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int copyFromControlNode(char *remoteFile, char *localFile)
{
  int handle;
  time_t startTime, endTime;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  digs_error_code_t result;
  float timeout;
  int percentComplete;
  digs_transfer_status_t status;

  result = digs_startGetTransfer_globus(errbuf, mainNode_, remoteFile,
					localFile, &handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error starting get transfer from %s: %s (%s)", mainNode_,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  timeout = 30.0;

  startTime = time(NULL);
  do {
    result = digs_monitorTransfer_globus(errbuf, handle, &status, &percentComplete);
    if ((result != DIGS_SUCCESS) || (status == DIGS_TRANSFER_FAILED)) {
      logMessage(ERROR, "Error in get transfer from %s: %s (%s)", mainNode_,
		 digsErrorToString(result), errbuf);
      digs_endTransfer_globus(errbuf, handle);
      return 0;
    }
    if (status == DIGS_TRANSFER_DONE) {
      break;
    }

    globus_libc_usleep(1000);

    endTime = time(NULL);
  } while (difftime(endTime, startTime) < timeout);

  result = digs_endTransfer_globus(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error in end transfer from %s: %s (%s)", mainNode_,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  return 1;  
}

/***********************************************************************
*   char *getHostname()
*    
*   Returns a pointer to the FQDN of the local host. The storage pointed
*   to should not be modified or freed.
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: pointer to FQDN of local host
***********************************************************************/
char *getHostname()
{
    return thisHostname_;
}

/***********************************************************************
*   char *getTemporaryFile()
*
*   Creates a uniquely named file in the temporary directory and returns
*   its name. File should be deleted and name should be freed by the
*   caller after use.
*
*   Parameters:                                                    [I/O]
*
*     (none)
*   
*   Returns: filename pointer, or NULL on failure
***********************************************************************/
/*
 * Can't call before tmpDir_ is initialised!
 */
char *getTemporaryFile()
{
    char *tmpFile;
    int fd;

    /* make temporary file to copy it to locally */
    if (safe_asprintf(&tmpFile, "%s/qcdgridtmpXXXXXX", tmpDir_)<0)
    {
	errorExit("Out of memory in getTemporaryFile");
    }
    fd=mkstemp(tmpFile);
    if (fd<0) 
    {
	globus_libc_free(tmpFile);
	logMessage(3, "mkstemp failed in getTemporaryFile");
	return NULL;
    }
    close(fd);

    return tmpFile;
}

/***********************************************************************
*   void determineTmpDir()
*
*   Sets tmpDir_ to the location to use for temporary files
*
*   Parameters:                                                    [I/O]
*
*     (none)
*   
*   Returns: (void)
***********************************************************************/
static void determineTmpDir()
{
    char *tmp;

    tmp = getenv("QCDGRID_TMP");
    if (tmp)
    {
	tmpDir_ = safe_strdup(tmp);
    }
    else
    {
	tmpDir_ = safe_strdup("/tmp");
    }
    if (!tmpDir_) errorExit("Out of memory in determineTmpDir");
    logMessage(1, "Temporary directory is %s", tmpDir_);
}

/*=====================================================================
 *
 * Path related stuff
 *
 *===================================================================*/
/* Directory in which qcdgrid software lives on this machine... */
char *installPath_=NULL;

/* ...and the same for the primary node */
char *primaryNodePath_=NULL;

/***********************************************************************
*   char *getQcdgridPath()
*    
*   Retrieves the path to the qcdgrid software on the local machine.
*   Should not be altered or freed
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: pointer to path to software
***********************************************************************/
char *getQcdgridPath()
{
    return installPath_;
}

/*=====================================================================
 *
 * Main node data structures
 *
 *===================================================================*/
/* Number of nodes currently in the grid */
int numGridNodes_=0;

#if 0
/* Actual node data structure */
typedef struct gridNode_s
{
    char *name;
    char *site;
    char *path;
    long long freeSpace;
    char *extraRsl;
    char *extraJssContact;

    /*
     * All the timeouts are measured in seconds. ftpTimeout is used for
     * 'short' ftp operations, like making directories or checking
     * existence. copyTimeout is used when copying data files, which
     * may take considerably longer. Defaults are 30 seconds for
     * jobTimeout and ftpTimeout, and 600 seconds for copyTimeout
     */
    float jobTimeout;
    float ftpTimeout;
    float copyTimeout;

    int gpfs;

} gridNode_t;

gridNode_t *gridNodes_=NULL;
#endif

struct storageElement *gridNodes_ = NULL;

/* Order in which the local machine likes the nodes */
nodeList_t *prefList_ = NULL;

/* List of nodes which have gone down and it's not known why */
nodeList_t *deadList_ = NULL;

/* List of nodes which are disabled and are known to be coming back */
nodeList_t *disabledList_ = NULL;

/* List of nodes which are retiring (available for retrieving data but
 * should not be used for new storage) */
nodeList_t *retiringList_ = NULL;

/*===================================================================*/

void initSEtoGlobus(struct storageElement *se) {
	se->digs_getLength = digs_getLength_globus;
	se->digs_getChecksum = digs_getChecksum_globus;
	se->digs_doesExist = digs_doesExist_globus;
	se->digs_isDirectory = digs_isDirectory_globus;
	se->digs_free_string = digs_free_string_globus;
	se->digs_getOwner = digs_getOwner_globus;
	se->digs_getGroup = digs_getGroup_globus;
	se->digs_setGroup = digs_setGroup_globus;
	se->digs_getPermissions = digs_getPermissions_globus;
	se->digs_setPermissions = digs_setPermissions_globus;
	se->digs_getModificationTime = digs_getModificationTime_globus;
	se->digs_startPutTransfer = digs_startPutTransfer_globus;
	se->digs_startCopyToInbox = digs_startCopyToInbox_globus;
	se->digs_monitorTransfer = digs_monitorTransfer_globus;
	se->digs_endTransfer = digs_endTransfer_globus;
	se->digs_cancelTransfer = digs_cancelTransfer_globus;
	se->digs_startGetTransfer = digs_startGetTransfer_globus;
	se->digs_mkdir = digs_mkdir_globus;
	se->digs_mkdirtree = digs_mkdirtree_globus;
	se->digs_mv = digs_mv_globus;
	se->digs_rm = digs_rm_globus;
	se->digs_rmdir = digs_rmdir_globus;
	se->digs_rmr = digs_rmr_globus;
	se->digs_copyFromInbox = digs_copyFromInbox_globus;
	se->digs_scanNode = digs_scanNode_globus;
	se->digs_scanInbox = digs_scanInbox_globus;
	se->digs_free_string_array = digs_free_string_array_globus;
	se->digs_ping = digs_ping_globus;
	se->digs_housekeeping = digs_housekeeping_globus;
}

void initSEtoSRM(struct storageElement *se) {
#ifdef WITH_SRM
	se->digs_getLength = digs_getLength_srm;
	se->digs_getChecksum = digs_getChecksum_srm;
	se->digs_doesExist = digs_doesExist_srm;
	se->digs_isDirectory = digs_isDirectory_srm;
	se->digs_free_string = digs_free_string_srm;
	se->digs_getOwner = digs_getOwner_srm;
	se->digs_getGroup = digs_getGroup_srm;
	se->digs_setGroup = digs_setGroup_srm;
	se->digs_getPermissions = digs_getPermissions_srm;
	se->digs_setPermissions = digs_setPermissions_srm;
	se->digs_getModificationTime = digs_getModificationTime_srm;
	se->digs_startPutTransfer = digs_startPutTransfer_srm;
	se->digs_startCopyToInbox = digs_startCopyToInbox_srm;
	se->digs_monitorTransfer = digs_monitorTransfer_srm;
	se->digs_endTransfer = digs_endTransfer_srm;
	se->digs_cancelTransfer = digs_cancelTransfer_srm;
	se->digs_startGetTransfer = digs_startGetTransfer_srm;
	se->digs_mkdir = digs_mkdir_srm;
	se->digs_mkdirtree = digs_mkdirtree_srm;
	se->digs_mv = digs_mv_srm;
	se->digs_rm = digs_rm_srm;
	se->digs_rmdir = digs_rmdir_srm;
	se->digs_rmr = digs_rmr_srm;
	se->digs_copyFromInbox = digs_copyFromInbox_srm;
	se->digs_scanNode = digs_scanNode_srm;
	se->digs_scanInbox = digs_scanInbox_srm;
	se->digs_free_string_array = digs_free_string_array_srm;
	se->digs_ping = digs_ping_srm;
	se->digs_housekeeping = digs_housekeeping_srm;
#else
	logMessage(ERROR, "SRM implementation not compiled in. Cannot initalise  %s",
			se);
#endif
}

#ifdef OMERO
void initSEtoOMERO(struct storageElement *se) {
	se->digs_getLength = digs_getLength_omero;
	se->digs_getChecksum = digs_getChecksum_omero;
	se->digs_doesExist = digs_doesExist_omero;
	se->digs_isDirectory = digs_isDirectory_omero;
	se->digs_free_string = digs_free_string_omero;
	se->digs_getOwner = digs_getOwner_omero;
	se->digs_getGroup = digs_getGroup_omero;
	se->digs_setGroup = digs_setGroup_omero;
	se->digs_getPermissions = digs_getPermissions_omero;
	se->digs_setPermissions = digs_setPermissions_omero;
	se->digs_getModificationTime = digs_getModificationTime_omero;
	se->digs_startPutTransfer = digs_startPutTransfer_omero;
	se->digs_startCopyToInbox = digs_startCopyToInbox_omero;
	se->digs_monitorTransfer = digs_monitorTransfer_omero;
	se->digs_endTransfer = digs_endTransfer_omero;
	se->digs_cancelTransfer = digs_cancelTransfer_omero;
	se->digs_startGetTransfer = digs_startGetTransfer_omero;
	se->digs_mkdir = digs_mkdir_omero;
	se->digs_mkdirtree = digs_mkdirtree_omero;
	se->digs_mv = digs_mv_omero;
	se->digs_rm = digs_rm_omero;
	se->digs_rmdir = digs_rmdir_omero;
	se->digs_rmr = digs_rmr_omero;
	se->digs_copyFromInbox = digs_copyFromInbox_omero;
	se->digs_scanNode = digs_scanNode_omero;
	se->digs_scanInbox = digs_scanInbox_omero;
	se->digs_free_string_array = digs_free_string_array_omero;
	se->digs_ping = digs_ping_omero;
	se->digs_housekeeping = digs_housekeeping_omero;
}
#endif

/*==============================================================================
 *
 * Node list manipulation functions
 *
 *=============================================================================*/
/***********************************************************************
*   nodeList_t *readNodeList(char *filename)
*
*   Reads a list of nodes from a text file with one node name per line,
*   into an index-based nodeList_t structure
*
*   Parameters:                                                    [I/O]
*
*     filename  file to read list from                              I
*   
*   Returns: pointer to list structure
***********************************************************************/
nodeList_t *readNodeList(char *filename)
{
    char *lineBuffer;
    FILE *f;
    int bufSize=0;
    nodeList_t *nl;
    int idx;

    logMessage(1, "readNodeList(%s)", filename);

    f = fopen(filename, "r");
    if (!f) 
    {
	logMessage(3, "Cannot open node list file %s", filename);
	return NULL;
    }

    nl = globus_libc_malloc(sizeof(nodeList_t));
    if (!nl)
    {
	errorExit("Out of memory in readNodeList");
    }

    nl->count = 0;
    nl->alloced = 0;
    nl->nodes = NULL;

    safe_getline(&lineBuffer, &bufSize, f);
    while(!feof(f)) 
    {
	removeCrlf(lineBuffer);
	idx = nodeIndexFromName(lineBuffer);
	if (idx >= 0)
	{
	    addNodeToList(nl, idx);
	}
	safe_getline(&lineBuffer, &bufSize, f);
    }
    fclose(f);
    globus_libc_free(lineBuffer);

    return nl;
}

/***********************************************************************
*   void destroyNodeList(nodeList_t *nl)
*
*   Frees all storage used by a nodeList_t structure
*
*   Parameters:                                                    [I/O]
*
*     nl    structure to free                                       I
*   
*   Returns: (void)
***********************************************************************/
void destroyNodeList(nodeList_t *nl)
{
    if (nl == NULL) return;

    if (nl->nodes != NULL)
    {
	globus_libc_free(nl->nodes);
    }
    globus_libc_free(nl);
}

/***********************************************************************
*   void addNodeToList(nodeList_t *nl, int idx)
*
*   Adds a node to a node list structure
*
*   Parameters:                                                    [I/O]
*
*     nl    structure to add to                                     I
*     idx   index (in main node list) of node to add                I
*   
*   Returns: (void)
***********************************************************************/
void addNodeToList(nodeList_t *nl, int idx)
{
    if (isNodeOnList(nl, idx)) return;

    if (nl->alloced <= nl->count)
    {
	nl->alloced += 10;
	nl->nodes = globus_libc_realloc(nl->nodes, nl->alloced * sizeof(int));

	if (!nl->nodes)
	{
	    errorExit("Out of memory in addNodeToList");
	}
    }

    nl->nodes[nl->count] = idx;
    nl->count++;
}

/***********************************************************************
*   void removeNodeFromList(nodeList_t *nl, int idx)
*
*   Removes a node from a node list structure
*
*   Parameters:                                                    [I/O]
*
*     nl    structure to remove from                                I
*     idx   index (in main node list) of node to remove             I
*   
*   Returns: (void)
***********************************************************************/
void removeNodeFromList(nodeList_t *nl, int idx)
{
    int i;

    for (i = 0; i < nl->count; i++)
    {
	if (nl->nodes[i] == idx)
	{
	    break;
	}
    }
    if (i == nl->count) return;

    nl->count--;

    for (; i < nl->count; i++)
    {
	nl->nodes[i] = nl->nodes[i+1];
    }
}

/***********************************************************************
*   int isNodeOnList(nodeList_t *nl, int idx)
*
*   Checks for the presence of a certain node on a list
*
*   Parameters:                                                    [I/O]
*
*     nl    structure to check                                      I
*     idx   index (in main node list) of node to check for          I
*   
*   Returns: 1 if node is present, 0 if not
***********************************************************************/
int isNodeOnList(nodeList_t *nl, int idx)
{
    int i;

    for (i = 0; i < nl->count; i++)
    {
	if (nl->nodes[i] == idx)
	{
	    return 1;
	}
    }

    return 0;
}

/***********************************************************************
*   void updateNodeListIndices(nodeList_t *nl, int idx)
*
*   Updates the indices in a nodeList_t structure to reflect a node
*   being removed from the main node list. Needs to be called on each
*   node list whenever a node is removed, otherwise the indices become
*   incorrect
*
*   Parameters:                                                    [I/O]
*
*     nl    structure to update                                     I
*     idx   index (in main node list) of node which was removed     I
*   
*   Returns: (void)
***********************************************************************/
void updateNodeListIndices(nodeList_t *nl, int idx)
{
    int i;

    for (i = 0; i < nl->count; i++)
    {
	if (nl->nodes[i] > idx)
	{
	    nl->nodes[i]--;
	}
    }
}

/***********************************************************************
*   void writeNodeList(nodeList_t *nl, char *filename)
*
*   Writes a node list structure to a text file (with one node name per
*   line). Tries to do the update atomically so that another process
*   reading the file will never get a partial file
*
*   Parameters:                                                    [I/O]
*
*     nl        structure to save                                   I
*     filename  file to write list into                             I
*   
*   Returns: (void)
***********************************************************************/
void writeNodeList(nodeList_t *nl, char *filename)
{
    FILE *f;
    int i;
    char *filename2;

    logMessage(1, "writeNodeList(%s)", filename);

    if (safe_asprintf(&filename2, "%s.new", filename) < 0)
    {
	errorExit("Out of memory in writeNodeList");
    }

    f = fopen(filename2, "w");
    if (!f) 
    {
	logMessage(3, "Unable to open file %s for writing", filename2);
	globus_libc_free(filename2);
	return;
    }

    for (i = 0; i < nl->count; i++)
    {
	globus_libc_fprintf(f, "%s\n", getNodeName(nl->nodes[i]));
    }
  
    fclose(f);

    /* Attempt to prevent another node reading the list when only half
       of it's been written */
    rename(filename2, filename);
    globus_libc_free(filename2);
    return;
}

/*=====================================================================
 *
 * Dead/disabled list management
 *
 *===================================================================*/

/***********************************************************************
*   int isNodeDead(char *node)
*    
*   Checks if the specified node is currently on the dead list
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to check                               I
*   
*   Returns: 1 if node is dead, 0 if not
***********************************************************************/
int isNodeDead(char *node)
{
    int ni;

    ni=nodeIndexFromName(node);
    if (ni<0) return 0;

    return isNodeOnList(deadList_, ni);
}

/***********************************************************************
*   int isNodeDisabled(char *node)
*    
*   Checks if the specified node is currently on the disabled list
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to check                               I
*   
*   Returns: 1 if node is disabled, 0 if not
***********************************************************************/
int isNodeDisabled(char *node)
{
    int ni;

    ni=nodeIndexFromName(node);
    if (ni<0) return 0;

    return isNodeOnList(disabledList_, ni);
}

/***********************************************************************
*   int isNodeRetiring(char *node)
*    
*   Checks if the specified node is currently on the retiring list
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to check                               I
*   
*   Returns: 1 if node is retiring, 0 if not
***********************************************************************/
int isNodeRetiring(char *node)
{
    int ni;

    ni=nodeIndexFromName(node);
    if (ni<0) return 0;

    return isNodeOnList(retiringList_, ni);
}

/***********************************************************************
*   void addToDeadList(char *node)
*
*   Adds the specified node to the list of dead nodes   
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to add                                 I
*   
*   Returns: (void)
***********************************************************************/
void addToDeadList(char *node)
{
    int ni;

    if (isNodeDead(node)) return;

    ni=nodeIndexFromName(node);
    addNodeToList(deadList_, ni);
}

/***********************************************************************
*   void addToDisabledList(char *node)
*
*   Adds the specified node to the list of disabled nodes   
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to add                                 I
*   
*   Returns: (void)
***********************************************************************/
void addToDisabledList(char *node)
{
    int ni;

    if (isNodeDisabled(node)) return;

    ni=nodeIndexFromName(node);
    addNodeToList(disabledList_, ni);
}

/***********************************************************************
*   void addToRetiringList(char *node)
*
*   Adds the specified node to the list of retiring nodes   
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to add                                 I
*   
*   Returns: (void)
***********************************************************************/
void addToRetiringList(char *node)
{
    int ni;

    if (isNodeRetiring(node)) return;

    ni=nodeIndexFromName(node);
    addNodeToList(retiringList_, ni);
}

/***********************************************************************
*   void removeFromDeadList(char *node)
*
*   Removes the specified node from the list of dead nodes   
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to remove                              I
*   
*   Returns: (void)
***********************************************************************/
void removeFromDeadList(char *node)
{
    int ni;

    ni=nodeIndexFromName(node);
    removeNodeFromList(deadList_, ni);
}

/***********************************************************************
*   void removeFromDisabledList(char *node)
*
*   Removes the specified node from the list of disabled nodes   
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to remove                              I
*   
*   Returns: (void)
***********************************************************************/
void removeFromDisabledList(char *node)
{
    int ni;

    ni=nodeIndexFromName(node);
    removeNodeFromList(disabledList_, ni);
}

/***********************************************************************
*   void removeFromRetiringList(char *node)
*
*   Removes the specified node from the list of retiring nodes   
*    
*   Parameters:                                                    [I/O]
*
*     node  FQDN of the node to remove                              I
*   
*   Returns: (void)
***********************************************************************/
void removeFromRetiringList(char *node)
{
    int ni;

    ni=nodeIndexFromName(node);
    removeNodeFromList(retiringList_, ni);
}

/***********************************************************************
*   int writeDeadList()
*
*   Writes the current list of dead nodes out to disk. Will only be any
*   use on the central node since that's where the list is stored
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int writeDeadList()
{
    char *filename;

    if (safe_asprintf(&filename, "%s/deadnodes.conf", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in writeDeadList");
    }
    writeNodeList(deadList_, filename);
    globus_libc_free(filename);
    return 1;
}

/***********************************************************************
*   int writeDisabledList()
*
*   Writes the current list of disabled nodes out to disk. Will only be any
*   use on the central node since that's where the list is stored
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int writeDisabledList()
{
    char *filename;

    if (safe_asprintf(&filename, "%s/disablednodes.conf", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in writeDisabledList");
    }
    writeNodeList(disabledList_, filename);
    globus_libc_free(filename);
    return 1;
}

/***********************************************************************
*   int writeRetiringList()
*
*   Writes the current list of retiring nodes out to disk. Will only be any
*   use on the central node since that's where the list is stored
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int writeRetiringList()
{
    char *filename;

    if (safe_asprintf(&filename, "%s/retiringnodes.conf", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in writeRetiringList");
    }
    writeNodeList(retiringList_, filename);
    globus_libc_free(filename);
    return 1;
}

/*=====================================================================
 *
 * Simple node querying functions
 *
 *===================================================================*/
/***********************************************************************
*  struct storageElement *getNode(char *nodeName)
*
*   Gets the storage element with the given name.
*    
*   Parameters:                                                    [I/O]
*
*     nodeName   Node's FQDN                                            I
*   
*   Returns: storage element, or NULL on error
***********************************************************************/
struct storageElement *getNode(char *nodeName) {
	
	int i;
	i=nodeIndexFromName(nodeName);
	if (i<0)
		return NULL;
	return &gridNodes_[i];
}

/***********************************************************************
*   int nodeSupportsAddScan(char *nodeName)
*
*   Checks whether the node supports adding new files via a scan.
*    
*   Parameters:                                                    [I/O]
*
*     nodeName    Node's FQDN                                       I
*   
*   Returns: 1 if node supports add via scan, 0 if not
***********************************************************************/
int nodeSupportsAddScan(char *nodeName)
{
  struct storageElement *se = getNode(nodeName);
  if (se->storageElementType == OMERO_SE) {
    return 1;
  }
  return 0;
}

/***********************************************************************
*   char *getMainNodeName()
*
*   Returns the FQDN of the central node running the control thread and 
*   replica catalogue. Should not be altered or freed
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: Pointer to the main node's FQDN
***********************************************************************/
char *getMainNodeName()
{
    return mainNode_;
}

/***********************************************************************
*   char *getMetadataCatalogueLocation()
*
*   Returns the FQDN of the metadata catalogue, which is configured via
*   the mdc parameter in nodes.conf. Should just be the FQDN of the host;
*   the service is always assumed to be on port 8080 (we can change this
*   later if necessary).
*
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: Pointer to the main node's FQDN
***********************************************************************/
char *getMetadataCatalogueLocation()
{
    return mdcLocation_;
}


/***********************************************************************
*   int getNumNodes()
*
*   Returns the number of nodes currently on the grid
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: Number of registered QCDgrid nodes
***********************************************************************/
int getNumNodes()
{
    return numGridNodes_;
}

/***********************************************************************
*   char *getNodeProperty(char *node, char *prop)
*
*   Gets a property of a node
*    
*   Parameters:                                                    [I/O]
*
*     node   FQDN of node                                           I
*     prop   property name                                          I
*   
*   Returns: property value. Caller should free. NULL on error
***********************************************************************/
char *getNodeProperty(const char *node, char *prop)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0) return NULL;
    return lookupValueInHashTable(gridNodes_[i].properties, prop);
}

/***********************************************************************
*   char *getNodeName(int i)
*
*   Translates a node number to its FQDN
*    
*   Parameters:                                                    [I/O]
*
*     i   Index of node in table                                    I
*   
*   Returns: FQDN of node. Do not alter or free
***********************************************************************/
static char unknown_node[] = { "<unknown node>" };

char *getNodeName(int i)
{
    if ((i < 0) || (i >= numGridNodes_))
    {
	return unknown_node;
    }
    return &gridNodes_[i].name[0];
}

/***********************************************************************
*   int nodeIndexFromName(char *node)
*
*   Translate a node's FQDN to its index in the node table
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: index of node in table, or -1 if it's not listed
***********************************************************************/
int nodeIndexFromName(const char *node)
{
    int i;

    for (i=0; i<numGridNodes_; i++) 
    {
	if (!strcmp(node, gridNodes_[i].name)) 
	{
	    return i;
	}
    }

    return -1;
}

/***********************************************************************
*   char *getNodeInbox(char *node)
*
*   Gets the inbox of a node or NULL if none specified.
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: site name, or NULL on error
***********************************************************************/
char *getNodeInbox(const char *node)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0) return NULL;/*TODO Should distinguish between error and valid NULL.*/
    return &gridNodes_[i].inbox[0];
}

/***********************************************************************
*   char *getNodePath(char *node)
*
*   Gets the path where the QCDgrid software is installed on a node
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: path to QCDgrid software on node, or NULL on error
***********************************************************************/
char *getNodePath(const char *node)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0)
    {
	/*
	 * Main node might need to get its own path but might not be in
	 * the node list
	 */
	if (!strcmp(node, thisHostname_))
	{
	    return installPath_;
	}
	return NULL;
    }
    return &gridNodes_[i].path[0];
}

/***********************************************************************
*   char *getNodeSite(char *node)
*
*   Gets the site where a node is physically located
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: site name, or NULL on error
***********************************************************************/
char *getNodeSite(char *node)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0) return NULL;
    return &gridNodes_[i].site[0];
}

/***********************************************************************
*   char *getNodeExtraRsl(char *node)
*
*   Gets the extra RSL needed for submitting a job to a node
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: extra RSL, NULL if none or on error
***********************************************************************/
char *getNodeExtraRsl(char *node)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0) return NULL;
    return gridNodes_[i].extraRsl;
}

/***********************************************************************
*   char *getNodeExtraJssContact(char *node)
*
*   Gets the extra JSS contact string data needed for submitting a job
*   to a node
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: extra JSS contact string, NULL if none or on error
***********************************************************************/
char *getNodeExtraJssContact(char *node)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0) return NULL;
    return gridNodes_[i].extraJssContact;
}

/***********************************************************************
*   float getNodeJobTimeout(char *node)
*
*   Gets the timeout value to use when submitting jobs to this node
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: timeout in seconds, negative value indicates error
***********************************************************************/
float getNodeJobTimeout(char *node)
{
    int i;
    float timeout;

    i=nodeIndexFromName(node);
    if (i<0) return -1.0;

    timeout=gridNodes_[i].jobTimeout;
    if (timeout<0.0)
    {
	timeout=45.0;
    }
    return timeout;
}

/***********************************************************************
*   float getNodeFtpTimeout(char *node)
*
*   Gets the timeout value to use when performing simple FTP operations
*   on this node
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: timeout in seconds, negative value indicates error
***********************************************************************/
float getNodeFtpTimeout(char *node)
{
    int i;
    float timeout;

    i=nodeIndexFromName(node);
    if (i<0) return -1.0;

    timeout=gridNodes_[i].ftpTimeout;
    if (timeout<0.0)
    {
	timeout=45.0;
    }
    return timeout;
}

/***********************************************************************
*   float getNodeCopyTimeout(char *node)
*
*   Gets the timeout value to use when copying data files to or from
*   this node
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: timeout in seconds, negative value indicates error
***********************************************************************/
float getNodeCopyTimeout(char *node)
{
    int i;
    float timeout;

    i=nodeIndexFromName(node);
    if (i<0) return 600.0;

    timeout=gridNodes_[i].copyTimeout;
    if (timeout<0.0)
    {
	timeout=600.0;
    }
    return timeout;
}

/***********************************************************************
*   int getNodeGpfs(char *node)
*
*   Returns 1 if the specified node is using GPFS
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: 1 if the specified node is using GPFS, 0 for simple disk
***********************************************************************/
int getNodeGpfs(char *node)
{
    int i;

    i = nodeIndexFromName(node);
    if (i < 0) return -1;

    return gridNodes_[i].gpfs;
}
/***********************************************************************
*   long long getNodeTotalDiskQuota(char *node)
*
*   Returns the total disk space quota for all of the disks on this node.
*    
*   Parameters:                                                    [I/O]
*
*     node   Node's FQDN                                            I
*   
*   Returns: the total disk space quota for all of the disks on this node.
***********************************************************************/
long long getNodeTotalDiskQuota(char *node)
{
    int i;
    long long totalQuota = 0;
    int diskItr;
    
    i = nodeIndexFromName(node);
    if (i < 0) return -1;
    
    for (diskItr = 0; diskItr < gridNodes_[i].numDisks; diskItr++){
    	totalQuota += gridNodes_[i].diskQuota[diskItr];
    }
    return totalQuota;
}

/***********************************************************************
*   long long getNodeDiskSpace(char *node)
*    
*   Returns the disk space free, in kilobytes, on a node
*    
*   Parameters:                                     [I/O]
*
*     node  FQDN of node to get disk space on        I
*    
*   Returns: disk space free on node, in kilobytes, or -1 on error
***********************************************************************/
long long getNodeDiskSpace(char *node)
{
    int i;
    i=nodeIndexFromName(node);
    if (i<0) return -1;
    return gridNodes_[i].freeSpace;
}

/***********************************************************************
*   void updateNodeDiskSpace()
*    
*   Pulls up-to-date node disk space values from the central node.
*   Without doing this, disk space would quickly get out of sync with
*   our data during a recursive put operation
*    
*   Parameters:                                     [I/O]
*
*     none
*    
*   Returns: (void)
***********************************************************************/
void updateNodeDiskSpace()
{
    char *remoteName;
    char *nodeName;
    int nodeIndex;
    char *tmpFile;

    logMessage(1, "updateNodeDiskSpace");

    tmpFile = getTemporaryFile();
    if (!tmpFile)
    {
	return;
    }

    /* Copy the main node list file */
    if (safe_asprintf(&remoteName, "%s/mainnodelist.conf", primaryNodePath_) < 0)
    {
	errorExit("Out of memory in updateNodeDiskSpace");
    }
    if (!copyFromControlNode(remoteName, tmpFile))
    {
	unlink(tmpFile);
	globus_libc_free(tmpFile);
	globus_libc_free(remoteName);
	return;
    }
    globus_libc_free(remoteName);

    /* Dispose of old config file */
    freeConfigFile("nodelist");

    /* and read new copy in */
    if (!loadConfigFile(tmpFile, "nodelist"))
    {
	unlink(tmpFile);
	globus_libc_free(tmpFile);
	logMessage(3, "Cannot parse mainnodelist.conf");
	return;
    }
    unlink(tmpFile);
    globus_libc_free(tmpFile);

    nodeName = getFirstConfigValue("nodelist", "node");
    while (nodeName)
    {
	nodeIndex = nodeIndexFromName(nodeName);
	if (nodeIndex >= 0)
	{
	    gridNodes_[nodeIndex].freeSpace = 
		strtoll(getNextConfigValue("nodelist", "disk"), NULL, 10);
	}
	nodeName = getNextConfigValue("nodelist", "node");
    }
}

/*=====================================================================
 *
 * Node management functions - used only by control thread
 *
 *===================================================================*/
/***********************************************************************
*   void setNodeDiskSpace(int node, int df)
*
*   Updates a node's free disk space. Called by the control thread when 
*   it has just queried the node for the actual space free
*    
*   Parameters:                                                    [I/O]
*
*     node  Table index of node in question                         I
*     df    Free disk space on node in kilobytes                    I
*   
*   Returns: (void)
***********************************************************************/
void setNodeDiskSpace(int node, long long df)
{
    gridNodes_[node].freeSpace = df;
}

/***********************************************************************
*   void removeNode(int node)
*
*   Removes a node from the main node table
*    
*   Parameters:                                                    [I/O]
*
*     node  Table index of node in question                         I
*   
*   Returns: (void)
***********************************************************************/
void removeNode(int node)
{
    int i;

    logMessage(1, "removeNode(%d)", node);

    for (i = node; i < (numGridNodes_-1); i++) 
    {
	gridNodes_[i] = gridNodes_[i+1];
    }
    numGridNodes_--;

    /* Disabled, dead and retiring lists are index based, so have to update
     * them now as well */
    updateNodeListIndices(prefList_, node);
    updateNodeListIndices(deadList_, node);
    updateNodeListIndices(disabledList_, node);
    updateNodeListIndices(retiringList_, node);
    
    gridNodes_ = globus_libc_realloc(gridNodes_, numGridNodes_ *
				     sizeof(struct storageElement));
    if (!gridNodes_)
    {
	errorExit("Out of memory in removeNode");
    }
}

/***********************************************************************
*   void addNode(char *name, char *site, int df, char *path)
*
*   Adds a node to the main node table
*    
*   Parameters:                                                    [I/O]
*
*     name  FQDN of node to add                                     I
*     site  Physical site at which node is based                    I
*     df    Free disk space on node                                 I
*     path  Path to QCDgrid software on node                        I
*   
*   Returns: (void)
***********************************************************************/
void addNode(char *name, char *site, long long df, char *path)
{
    logMessage(1, "addNode(%s,%s,%s)", name, site, path);
    gridNodes_ = globus_libc_realloc(gridNodes_, (numGridNodes_+1)*
				     sizeof(struct storageElement));
    if (!gridNodes_) errorExit("Out of memory in addNode");
    gridNodes_[numGridNodes_].name = safe_strdup(name);
    if (!gridNodes_[numGridNodes_].name)
	errorExit("Out of memory in addNode");
    gridNodes_[numGridNodes_].site = safe_strdup(site);
    if (!gridNodes_[numGridNodes_].site)
	errorExit("Out of memory in addNode");
    gridNodes_[numGridNodes_].freeSpace = df;
    gridNodes_[numGridNodes_].path = safe_strdup(path);
    if (!gridNodes_[numGridNodes_].path)
	errorExit("Out of memory in addNode");
    gridNodes_[numGridNodes_].extraRsl = NULL;
    gridNodes_[numGridNodes_].extraJssContact = NULL;
    gridNodes_[numGridNodes_].jobTimeout = -1.0;
    gridNodes_[numGridNodes_].copyTimeout = -1.0;
    gridNodes_[numGridNodes_].ftpTimeout = -1.0;
    gridNodes_[numGridNodes_].gpfs = 0;
    numGridNodes_++;
}

static void writePropertyCallback(char *key, char *value, void *param)
{
    FILE *f = (FILE *)param;

    if ((!strcmp(key, "node")) || (!strcmp(key, "site")) ||
	(!strcmp(key, "disk")) || (!strcmp(key, "path")))
    {
	return;
    }

    fprintf(f, "%s=%s\n", key, value);
}

/***********************************************************************
*   void writeNodeTable()
*
*   Writes the node table back out to disk
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: (void)
***********************************************************************/
void writeNodeTable()
{
    FILE *f;
    int i, j;
    char *filenameBuffer;
    char *filenameBuffer2;

    logMessage(1, "writeNodeTable()");

    if (safe_asprintf(&filenameBuffer, "%s/mainnodelist.new", getQcdgridPath()) < 0) 
    {
	errorExit("Out of memory in writeNodeTable");
    }
    f = fopen(filenameBuffer, "w");

    if (!f)
    {
	logMessage(3, "Cannot open %s for writing", filenameBuffer);
	return;
    }

    globus_libc_fprintf(f, "#\n# Main QCDgrid node list\n# This file is automatically "
			"generated\n# Don't edit it unless you know what you're doing!\n#"
			"\n\n");
    for (i = 0; i < numGridNodes_; i++) 
    {
	globus_libc_fprintf(f, "node=%s\nsite=%s\ndisk=%qd\npath=%s\n", 
			    gridNodes_[i].name, gridNodes_[i].site, 
			    gridNodes_[i].freeSpace, gridNodes_[i].path);
#if 0
	switch (gridNodes_[i].storageElementType)
	{
	case GLOBUS:
	    globus_libc_fprintf(f, "type=globus\n");
	    break;
	case SRM:
	    globus_libc_fprintf(f, "type=srm\n");
	    break;
	case OMERO_SE:
	    globus_libc_fprintf(f, "type=omero\n");
	    break;
	}
	if (gridNodes_[i].extraRsl)
	{
	    globus_libc_fprintf(f, "extrarsl=%s\n", gridNodes_[i].extraRsl);
	}
	if (gridNodes_[i].extraJssContact)
	{
	    globus_libc_fprintf(f, "extrajsscontact=%s\n", gridNodes_[i].extraJssContact);
	}
	if (gridNodes_[i].jobTimeout >= 0.0)
	{
	    globus_libc_fprintf(f, "jobtimeout=%f\n", gridNodes_[i].jobTimeout);
	}
	if (gridNodes_[i].ftpTimeout >= 0.0)
	{
	    globus_libc_fprintf(f, "ftptimeout=%f\n", gridNodes_[i].ftpTimeout);
	}
	if (gridNodes_[i].copyTimeout >= 0.0)
	{
	    globus_libc_fprintf(f, "copytimeout=%f\n", gridNodes_[i].copyTimeout);
	}
	if (gridNodes_[i].gpfs)
	{
	    globus_libc_fprintf(f, "gpfs=%d\n", gridNodes_[i].gpfs);
	}
	if (gridNodes_[i].inbox)
	{
	    globus_libc_fprintf(f, "inbox=%s\n", gridNodes_[i].inbox);
	}

	for (j = 0; j < gridNodes_[i].numDisks; j++)
	{
	    if (j == 0)
	    {
	        globus_libc_fprintf(f, "data=%qd\n", gridNodes_[i].diskQuota[j]);
	    }
	    else
	    {
	        globus_libc_fprintf(f, "data%d=%qd\n", j, gridNodes_[i].diskQuota[j]);
	    }
	}
#endif
	/* Write extra properties out */
	forEachHashTableKeyAndValue(gridNodes_[i].properties, writePropertyCallback, (void *)f);

	globus_libc_fprintf(f, "\n");
    }
    fclose(f);

    if (safe_asprintf(&filenameBuffer2, "%s/mainnodelist.conf", 
		      getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in writeNodeTable");
    }
    /* Attempt to eliminate race conditions - other nodes can grab this file
     * by GridFTP */
    rename(filenameBuffer, filenameBuffer2);
    globus_libc_free(filenameBuffer);
    globus_libc_free(filenameBuffer2);
}

/*=====================================================================
 *
 * Node choosing functions
 *
 *===================================================================*/
/***********************************************************************
*   char **getSuitableNodeForPrimary(long long size)
*
*   Finds nodes suitable for holding the primary copy of a file
*    
*   Parameters:                                                    [I/O]
*
*     size  Size of file which needs to be stored, in bytes         I
*   
*   Returns: Pointer to NULL-terminated list of FQDNs of nodes
*            Caller should free the list pointer when done, but not the
*            node names it points to
*            NULL if no suitable node available
***********************************************************************/
char **getSuitableNodeForPrimary(long long size)
{
    int i, j;
    long long bestScoreSoFar;
    long long thisScore;
    long long space;
    char **list;
    long long *scores;
    int listpos;
    int bestIdx;

    logMessage(1, "getSuitableNodeForPrimary(%d)", (long) size);

    /* allocate space for list, and for scores */
    list = globus_libc_malloc((prefList_->count + 1) * sizeof(char*));
    scores = globus_libc_malloc(prefList_->count * sizeof(long long));
    if ((list == NULL) || (scores == NULL))
    {
	errorExit("Out of memory in getSuitableNodeForPrimary");
    }

    /* Search the node preference list, assigning scores */
    for (i = 0; i < prefList_->count; i++)
    {
	scores[i] = -1;

	/* Check if this node is a potential candidate (must have enough
	 * space, and not be dead or disabled) */
	space=(gridNodes_[prefList_->nodes[i]].freeSpace)*1024;
	if ((space>size)&&
	    (!isNodeDisabled(getNodeName(prefList_->nodes[i])))&&
	    (!isNodeDead(getNodeName(prefList_->nodes[i])))&&
	    (!isNodeRetiring(getNodeName(prefList_->nodes[i]))))
	{
	    /* Assign the node a score based on its position in the preference
	     * list, and on the amount of free space remaining. */
	    thisScore = (((long long)(numGridNodes_-i))*((long long)locationWeight_)*100000000)+
		(((long long)gridNodes_[prefList_->nodes[i]].freeSpace)*(long long)spaceWeight_);

	    scores[i] = thisScore;
	}
    }

    listpos = 0;

    do
    {
	/* select next best score */
	bestScoreSoFar = -1;
	bestIdx = -1;
	for (j = 0; j < prefList_->count; j++)
	{
	    if (scores[j] >= bestScoreSoFar)
	    {
		bestIdx = j;
		bestScoreSoFar = scores[j];
	    }
	}

	/* add it to the list */
	if (bestScoreSoFar > 0)
	{
	    list[listpos] = &gridNodes_[prefList_->nodes[bestIdx]].name[0];
	    listpos++;
	    scores[bestIdx] = -1;
	}
	
	/* loop until done all */
    } while (bestScoreSoFar > 0);


    list[listpos] = NULL;
    globus_libc_free(scores);

    /* Return the highest scoring node */
    return list;
}

/***********************************************************************
*   char *getSuitableNodeForMirror(char *file, long long size)
*
*   Finds a node suitable for holding the mirror copy of a file. Has to
*   be at a different site from where primary copy is stored
*    
*   Parameters:                                                    [I/O]
*
*     file     name of file to be stored                            I
*     size     Size of file which needs to be stored, in bytes      I
*   
*   Returns: Pointer to FQDN of node (do not alter or free).
*            NULL if no suitable node available
***********************************************************************/
char *getSuitableNodeForMirror(char *file, long long size)
{
    char **filesCurrentSites = NULL;
    int numCurrentSites = 0;
    char *node;
    int i, j;
    char *chosenNode;
    int isNodeSuitable;
    int bestScoreSoFar;
    long long space;
    char *site;

    logMessage(1, "getSuitableNodeForMirror(%s,%d)", file, (long) size);

    /*
     * First find all the sites where the file currently resides, so we
     * can choose a node away from them all
     */
    node = getFirstFileLocation(file);
    while(node)
    {
	site = getNodeSite(node);
	if (site == NULL)
	{
	    logMessage(3, "No site found for node %s", node);
	}
	else
	{
	    /* We simply add all the nodes' sites to this list - no point in
	     * checking for duplicates, they don't do any harm */
	    filesCurrentSites = globus_libc_realloc(filesCurrentSites, 
						    (numCurrentSites+1)*sizeof(char*));
	    if (!filesCurrentSites)
	    {
		errorExit("Out of memory in getSuitableNodeForMirror");
	    }
	    filesCurrentSites[numCurrentSites] = safe_strdup(getNodeSite(node));
	    numCurrentSites++;
	}

	globus_libc_free(node);
	node = getNextFileLocation();
    }

    /* Initialise search results */
    bestScoreSoFar = 0;
    chosenNode = NULL;

    /* Try every node in turn */
    for (i = 0; i < numGridNodes_; i++)
    {
	space = (gridNodes_[i].freeSpace) * 1024;
	if ((space > size) &&
	    (!isNodeDisabled(getNodeName(i))) &&
	    (!isNodeDead(getNodeName(i))) &&
	    (!isNodeRetiring(getNodeName(i)))) 
	{
	    /* We've found one that's worth checking out - it's got
	     * enough space free and is not dead or disabled. Now see
	     * if it satisfies the geographical requirements as well */
	    isNodeSuitable = 1;
	    for (j = 0; j < numCurrentSites; j++)
	    {
		if (!strcmp(gridNodes_[i].site, filesCurrentSites[j]))
		{
		    /* there's already a copy of the file at this site, no
		     * point making another one */
		    isNodeSuitable = 0;
		}
	    }
	    if (isNodeSuitable)
	    {
		/* Just use the free space value as a score since it's the
		 * only value that we can take into account here */
		if (gridNodes_[i].freeSpace > bestScoreSoFar)
		{
		    bestScoreSoFar = gridNodes_[i].freeSpace;
		    chosenNode = &gridNodes_[i].name[0];
		}
	    }
	}
    }

    /* Free the array of current file sites */
    for (i = 0; i < numCurrentSites; i++)
    {
	globus_libc_free(filesCurrentSites[i]);
    }
    globus_libc_free(filesCurrentSites);

    return chosenNode;
}

/*=====================================================================
 *
 * Node preference list handling
 *
 *===================================================================*/
/***********************************************************************
*   void addToNodePreferenceList(int ni)
*
*   Adds a node to the preference list by its array index. Always adds to 
*   the end
*    
*   Parameters:                                                    [I/O]
*
*     ni   Index of node in table                                   I
*   
*   Returns: (void)
***********************************************************************/
void addToNodePreferenceList(int ni)
{
    addNodeToList(prefList_, ni);
}

/***********************************************************************
*   int isInPreferenceList(int ni)
*
*   Checks whether a node is in the preference list or not
*    
*   Parameters:                                                    [I/O]
*
*     ni   Index of node in table                                   I
*   
*   Returns: 1 if node is in list, 0 if not
***********************************************************************/
int isInPreferenceList(int ni)
{
    return isNodeOnList(prefList_, ni);
}

/***********************************************************************
*   int writeNodePreferenceList()
*
*   Writes the node preference list back to disk
*
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int writeNodePreferenceList()
{
    char *filename;

    if (safe_asprintf(&filename, "%s/nodeprefs.conf", installPath_) < 0) 
    {
	errorExit("Out of memory in writeNodePreferenceList");
    }

    writeNodeList(prefList_, filename);
    globus_libc_free(filename);
    return 1;
}

/***********************************************************************
*   void updateNodePreferenceList()
*
*   Called each time the node lists are read. Updates the local 
*   preference list to include any new nodes/remove any ex-nodes. (Still 
*   has to be edited by hand to get them in the right order)
*    
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: (void)
***********************************************************************/
void updateNodePreferenceList()
{
    int i;

    for (i=0; i<numGridNodes_; i++) 
    {
	if (!isInPreferenceList(i)) 
	{
	    addToNodePreferenceList(i);
	}
    }

    writeNodePreferenceList();
}

/*=====================================================================
 *
 * Startup functions
 *
 *===================================================================*/

/***********************************************************************
*   nodeList_t *readRemoteNodeList(char *name)
*
*   Copies a node list from the central node and reads it
*
*   Parameters:                                                    [I/O]
*
*     name  name of the node list file (within QCDgrid directory)   I
*   
*   Returns: pointer to node list structure, NULL on failure
***********************************************************************/
static nodeList_t *readRemoteNodeList(char *name)
{
    char *path;
    char *tmpFile;
    nodeList_t *list;

    logMessage(1, "readRemoteNodeList(%s)", name);

    /* get full path to node list file */
    if (safe_asprintf(&path, "%s/%s", primaryNodePath_, name) < 0)
    {
	errorExit("Out of memory in readRemoteNodeList");
    }

    /* make temporary file to copy it to locally */
    tmpFile = getTemporaryFile();
    if (!tmpFile)
    {
	globus_libc_free(path);
	return NULL;
    }

    /* copy the file */
    if (!copyFromControlNode(path, tmpFile))
    {
	unlink(tmpFile);
	globus_libc_free(tmpFile);
	globus_libc_free(path);
	logMessage(3, "Unable to copy %s from main node", name);
	return NULL;
    }
    globus_libc_free(path);

    /* load it in */
    list = readNodeList(tmpFile);
    unlink(tmpFile);
    globus_libc_free(tmpFile);
    return list;
}

/***********************************************************************
*   int determineLocalQcdgridPath()
*
*   Determines the path of the local QCDgrid installation and sets
*   installPath_ to this directory
*
*   Parameters:                                                    [I/O]
*
*     (none)
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int determineLocalQcdgridPath()
{
    char *commandBuffer;
    char *tmpFile;
    FILE *f;
    int bufSize;
    char *tmp;

    logMessage(1, "determineLocalQcdgridPath");

    tmpFile = getTemporaryFile();
    if (!tmpFile)
    {
	return 0;
    }

    /* First work out where we are in the file system */
    if (safe_asprintf(&commandBuffer, "which digs-put > %s", 
		      tmpFile) < 0) 
    {
	errorExit("Out of memory in determineLocalQcdgridPath");
    }
   
    system(commandBuffer);
    globus_libc_free(commandBuffer);

    f = fopen(tmpFile, "r");
    bufSize = 0;
    installPath_ = NULL;
    if (safe_getline(&installPath_, &bufSize, f) < 0)
    {
	fclose(f);
	unlink(tmpFile);
	globus_libc_free(tmpFile);

	logMessage(3, "Error parsing 'which' output in determineLocalQcdgridPath");
	return 0;
    }  
    else
    {
	/* if the line says "Warning: ridiculously long" then we attempt
	   to read another to get the actual path */
	if( strncmp( installPath_, "Warning:", 7 ) == 0 ) 
	{
	    if (safe_getline(&installPath_, &bufSize, f)<0) 
	    {
		fclose(f);
		unlink(tmpFile);
		globus_libc_free(tmpFile);
		logMessage(3, "Error parsing 'which' output in determineLocalQcdgridPath");
		return 0;
	    }
	}
    }

    fclose(f);
    unlink(tmpFile);
    globus_libc_free(tmpFile);

    tmp=installPath_+strlen(installPath_)-strlen("/digs-put");
    *tmp=0;

    return 1;
}

/***********************************************************************
*   int loadRemoteConfigFile(char *filename, char *cfgname)
*
*   Copies a config file from the main node and loads it
*
*   Parameters:                                                    [I/O]
*
*     filename  name of the file (within main node's QCDgrid dir)   I
*     cfgname   name to load it as                                  I
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int loadRemoteConfigFile(char *filename, char *cfgname)
{
    char *path;
    char *tmpFile;
    int result;

    logMessage(1, "loadRemoteConfigFile(%s,%s)", filename, cfgname);
    
    tmpFile = getTemporaryFile();
    if (!tmpFile)
    {
	return 0;
    }

    /* try to copy config file */
    if (safe_asprintf(&path, "%s/%s", primaryNodePath_, filename)
	< 0)
    {
	errorExit("Out of memory in loadRemoteConfigFile");
    }
    if (!copyFromControlNode(path, tmpFile))
    {
	unlink(tmpFile);
	globus_libc_free(tmpFile);
	globus_libc_free(path);
	return 0;
    }
    globus_libc_free(path);

    /* try to load it */
    result = loadConfigFile(tmpFile, cfgname);

    unlink(tmpFile);
    globus_libc_free(tmpFile);

    return result;
}

/***********************************************************************
*   int loadRemoteGroupMapFile(char *filename)
*
*   Transfers the group map file from the control node and reads it in.
*    
*   Parameters:                                               [I/O]
*
*     filename  name of group map file, assumed to be in       I
*               DiGS install directory on control node
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int loadRemoteGroupMapFile(char *filename)
{
    char *path;
    char *tmpFile;
    int result;

    logMessage(1, "loadRemoteGroupMapFile(%s)", filename);
    
    tmpFile = getTemporaryFile();
    if (!tmpFile)
    {
	return 0;
    }

    /* try to copy config file */
    if (safe_asprintf(&path, "%s/%s", primaryNodePath_, filename)
	< 0)
    {
	errorExit("Out of memory in loadRemoteGroupMapFile");
    }
    if (!copyFromControlNode(path, tmpFile))
    {
	unlink(tmpFile);
	globus_libc_free(tmpFile);
	globus_libc_free(path);
	return 0;
    }
    globus_libc_free(path);

    /* try to load it */
    result = loadGroupMapFile(tmpFile);

    unlink(tmpFile);
    globus_libc_free(tmpFile);

    return result;
}

/***********************************************************************
*   int initialiseFromMainNode()
*
*   Loads config files necessary from main node and opens replica
*   catalogue
*
*   Parameters:                                                    [I/O]
*
*     (none)
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int initialiseFromMainNode()
{
    logMessage(1, "initialiseFromMainNode()");

    if (!loadRemoteConfigFile("qcdgrid.conf", "miscconf"))
    {
	return 0;
    }

    if (!loadRemoteConfigFile("mainnodelist.conf", "nodelist"))
    {
	freeConfigFile("miscconf");
	return 0;
    }

    if (!loadRemoteGroupMapFile("group-mapfile"))
    {
        return 0;
    }

    if (!openReplicaCatalogue(mainNode_))
    {
	freeConfigFile("nodelist");
	freeConfigFile("miscconf");
	return 0;
    }
    return 1;
}

/***********************************************************************
*   int getNodeInfo()
*
*   Main node startup function. Reads node lists (main list, dead list,
*   disabled list, preference list). Works out local hostname and path
*   to QCDgrid software.
*
*   Parameters:                                                    [I/O]
*
*     secondaryOK  set if it is acceptable to use the secondary
*                  (backup) node instead of the primary             I
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int getNodeInfo(int secondaryOK)
{
    char *tmp;
    char *filenameBuffer;

    logMessage(1, "getNodeInfo(%d)", secondaryOK);

    /* Work out temp dir first */
    determineTmpDir();

    /* Get our local path */
    if (!determineLocalQcdgridPath())
    {
	return 0;
    }

    /* Get our hostname */
    globus_libc_gethostname(thisHostname_, 256);

    /* Read primary node name from config file */
    if (safe_asprintf(&filenameBuffer, "%s/nodes.conf", getQcdgridPath())<0) 
    {
	errorExit("Out of memory in getNodeInfo");
    }
    if (!loadConfigFile(filenameBuffer, "mainnodeinfo"))
    {
	globus_libc_free(filenameBuffer);
	logMessage(5, "Cannot parse nodes.conf");
	return 0;
    }
    globus_libc_free(filenameBuffer);

    tmp = getFirstConfigValue("mainnodeinfo", "node");
    if (!tmp)
    {
	logMessage(5, "No node specified in nodes.conf");
	return 0;
    }
    primaryNode_ = safe_strdup(tmp);
    tmp = getFirstConfigValue("mainnodeinfo", "path");
    if (!tmp)
    {
	logMessage(5, "No path specified in nodes.conf");
	return 0;
    }
    primaryNodePath_ = safe_strdup(tmp);

    /* Add Metadata Catalogue server location */
    tmp = getFirstConfigValue( "mainnodeinfo", "mdc" );
    if (tmp)
    {
        mdcLocation_ = safe_strdup(tmp);
    }
    else
    {	
        logMessage(5, "No metadata catalogue server specified. Use mdc=server:port in nodes.conf" );
        return 0;
    }

    /* Try copying main config file from primary node */
    mainNode_=primaryNode_;

    if (!initialiseFromMainNode())
    {
	/* primary node failed, try secondary */
	if (!secondaryOK)
	{
	    logMessage(5, "Primary node is down and secondary cannot be used for "
		       "this operation");
	    return 0;
	}

	/* Look up secondary info */
	globus_libc_free(primaryNode_);
	globus_libc_free(primaryNodePath_);

	tmp=getFirstConfigValue("mainnodeinfo", "backup_node");
	if (!tmp)
	{
	    logMessage(5, "Primary node is down and no backup node specified");
	    return 0;
	}
	primaryNode_=safe_strdup(tmp);
	tmp=getFirstConfigValue("mainnodeinfo", "backup_path");
	if (!tmp)
	{
	    logMessage(5, "No path specified on backup node");
	    return 0;
	}
	primaryNodePath_=safe_strdup(tmp);
	mainNode_=primaryNode_;

	if (!initialiseFromMainNode())
	{
	    return 0;
	}
	logMessage(5, "Warning: primary node is down, using secondary backup node instead");
    }

    /* process main node list */
    tmp=getFirstConfigValue("nodelist", "node");
    while (tmp)
    {
	char *key, *value;

	gridNodes_=globus_libc_realloc(gridNodes_, (numGridNodes_+1)*sizeof(struct storageElement));
	if (!gridNodes_) 
	{
	    errorExit("Out of memory in getNodeInfo");
	}

	/* Initialise node structure */
	gridNodes_[numGridNodes_].name=safe_strdup(tmp);
	if (!gridNodes_[numGridNodes_].name)
	{
	    errorExit("Out of memory in getNodeInfo");
	}
	gridNodes_[numGridNodes_].site=NULL;
	gridNodes_[numGridNodes_].freeSpace=0;
	gridNodes_[numGridNodes_].diskQuota = globus_libc_malloc(sizeof(long long));
	gridNodes_[numGridNodes_].diskQuota[0] = 0;
	gridNodes_[numGridNodes_].numDisks =0;
	gridNodes_[numGridNodes_].path=NULL;
	gridNodes_[numGridNodes_].extraRsl=NULL;
	gridNodes_[numGridNodes_].extraJssContact=NULL;
	gridNodes_[numGridNodes_].jobTimeout=-1.0;
	gridNodes_[numGridNodes_].ftpTimeout=-1.0;
	gridNodes_[numGridNodes_].copyTimeout=-1.0;
	gridNodes_[numGridNodes_].gpfs = 0;
	gridNodes_[numGridNodes_].inbox = NULL;
	gridNodes_[numGridNodes_].storageElementType = INVALID_SE_TYPE;
	gridNodes_[numGridNodes_].properties = newKeyAndValueHashTable();

	/* Iterate over all node's other parameters in config file */
	key=getNextConfigKeyValue("nodelist", &value);

	/* Loop until end of file or next node is reached */
	while ((key!=NULL)&&(strcmp(key, "node")))
	{
	    addKeyAndValueToHashTable(gridNodes_[numGridNodes_].properties, key, value);
	    if (!strcmp(key, "site"))
	    {
		gridNodes_[numGridNodes_].site=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].site)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "type"))
	    {
		if (!strcmp(value, "globus"))
		{
		    gridNodes_[numGridNodes_].storageElementType = GLOBUS;
		    initSEtoGlobus(&gridNodes_[numGridNodes_]);
		}
		else if (!strcmp(value, "srm"))
		{
		    gridNodes_[numGridNodes_].storageElementType = SRM;
		    initSEtoSRM(&gridNodes_[numGridNodes_]);
		}
#ifdef OMERO
		else if (!strcmp(value, "omero"))
		{
		    gridNodes_[numGridNodes_].storageElementType = OMERO_SE;
		    initSEtoOMERO(&gridNodes_[numGridNodes_]);
		}
#endif
		else
		{
		    logMessage(ERROR, "Unrecognised storage type: %s", value);
		}
	    }
	    else if (!strcmp(key, "inbox"))
	    {
		gridNodes_[numGridNodes_].inbox=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].inbox)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "path"))
	    {
		gridNodes_[numGridNodes_].path=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].path)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "disk"))
	    {
		gridNodes_[numGridNodes_].freeSpace=strtoll(value, NULL, 10);
	    }
	    else if (!strcmp(key, "extrarsl"))
	    {
		gridNodes_[numGridNodes_].extraRsl=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].extraRsl)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "extrajsscontact"))
	    {
		gridNodes_[numGridNodes_].extraJssContact=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].extraJssContact)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "jobtimeout"))
	    {
		gridNodes_[numGridNodes_].jobTimeout=atof(value);
	    }
	    else if (!strcmp(key, "ftptimeout"))
	    {
		gridNodes_[numGridNodes_].ftpTimeout=atof(value);
	    }
	    else if (!strcmp(key, "copytimeout"))
	    {
		gridNodes_[numGridNodes_].copyTimeout=atof(value);
	    }
	    else if (!strcmp(key, "gpfs"))
	    {
		gridNodes_[numGridNodes_].gpfs = atoi(value);
	    }
	    else if (!strncmp(key, "data", 4)) {
		int disknum = 0;
		logMessage(DEBUG, "reading disk quota '%s'", key);

		/* Get which disk number */
		if (strlen(key)>6) {
		    logMessage(WARN, "Unrecognised attribute '%s' for node %s",
			       key, tmp);
		}
		if (isdigit(key[4])) {
		    disknum = key[4] - '0';
		    if (isdigit(key[5])) {
			disknum = disknum*10 + (key[5] - '0');
		    }
		}
		if (disknum >= gridNodes_[numGridNodes_].numDisks) {
		    int i;
		    gridNodes_[numGridNodes_].diskQuota = globus_libc_realloc(
			gridNodes_[numGridNodes_].diskQuota,
			(disknum+1)*sizeof(long long));

		    /* zero the newly allocated quotas */
		    for (i = gridNodes_[numGridNodes_].numDisks; i < disknum+1; i++) {
			gridNodes_[numGridNodes_].diskQuota[i] = 0LL;
		    }

		    /* set numDisks */
		    gridNodes_[numGridNodes_].numDisks = disknum + 1;
		}

		gridNodes_[numGridNodes_].diskQuota[disknum] = strtoll(value, NULL, 10);
		logMessage(DEBUG, "quota value '%qd'", gridNodes_[numGridNodes_].diskQuota[disknum]);
	    }
	    else
	    {
		if (strcmp(key, "multidisk"))
		{
		    logMessage(3, "Unrecognised attribute '%s' for node %s", key, tmp);
		}
	    }
	    
	    key=getNextConfigKeyValue("nodelist", &value);
	}

	/* Check all the mandatory parameters existed */
	if ((gridNodes_[numGridNodes_].site==NULL)||(gridNodes_[numGridNodes_].path==NULL)
			||(gridNodes_[numGridNodes_].diskQuota[0]==NULL)
	    ||(gridNodes_[numGridNodes_].storageElementType == INVALID_SE_TYPE))
	{
	    logMessage(5, "Node %s is missing a required attribute", tmp);
	    return 0;
	}

	tmp=value;
	numGridNodes_++;
    }

    /*
     * Read node choosing parameters
     */
    locationWeight_ = getConfigIntValue("miscconf", "location_weight", 1);
    if (locationWeight_<0)
    {
	locationWeight_=1;
    }

    spaceWeight_ = getConfigIntValue("miscconf", "space_weight", 1);
    if (spaceWeight_<0)
    {
	spaceWeight_=1;
    }

    /* Now read the node preferences file. This basically lists the grid 
     * nodes in the order that this particular host likes to use them - 
     * normally closest ones first */
    if (safe_asprintf(&filenameBuffer, "%s/nodeprefs.conf", installPath_)<0) 
    {
	errorExit("Out of memory in getNodeInfo");
    }
    prefList_ = readNodeList(filenameBuffer);
    if (prefList_ == NULL)
    {
	logMessage(5, "Unable to read nodeprefs.conf");
	globus_libc_free(filenameBuffer);
	return 0;
    }
    globus_libc_free(filenameBuffer);

    /*updateNodePreferenceList();*/

    /* Try transferring dead node list from primary */
    deadList_ = readRemoteNodeList("deadnodes.conf");
    if (deadList_ == NULL)
    {
	logMessage(5, "Error reading dead node list");
	return 0;
    }

    /* Now try disabled node list */
    disabledList_ = readRemoteNodeList("disablednodes.conf");
    if (disabledList_ == NULL)
    {
	logMessage(5, "Error reading disabled node list");
	return 0;
    }

    /* Now try retiring node list */
    retiringList_ = readRemoteNodeList("retiringnodes.conf");
    if (retiringList_ == NULL)
    {
	logMessage(5, "Error reading retiring node list");
	return 0;
    }

    /* Warn user if user/host certificate is going to expire soon */
    checkCertificateExpiry();

    return 1;
}

/***********************************************************************
*   int loadLocalConfigFile(char *filename, char *cfgname)
*
* 	Loads a config file 
*
*   Parameters:                                                    [I/O]
*
*     filename  name of the file 								    I
*     cfgname   name to load it as                                  I
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int loadLocalConfigFile(char *filename, char *cfgname)
{
    char *path;
    int result;

    logMessage(DEBUG, "loadLocalConfigFile(%s,%s)", filename, cfgname);
    

    /* try to copy config file */
    if (safe_asprintf(&path, "%s/%s", installPath_, filename)
	< 0)
    {
	errorExit("Out of memory in loadLocalConfigFile");
    }
   

    /* try to load it */
    result = loadConfigFile(path, cfgname);

    globus_libc_free(path);

    return result;
}

/***********************************************************************
*   int initialiseFromMainNode()
*
*   Loads config files necessary from local source.
*
*   Parameters:                                                    [I/O]
*
*     (none)
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int initialiseLocally()
{
    logMessage(1, "initialiseLocally()");

    if (!loadLocalConfigFile("mainnodelist.conf", "nodelist"))
    {
	return 0;
    }

    return 1;
}

/***********************************************************************
*  int getNodeInfoFromLocalFile(const char *dir)
* TODO EG This is here to get the tests to run.
*
*   Main node startup function. Reads main node list. Works out local hostname 
* and path to QCDgrid software.
*
*   Parameters:                                                    [I/O]
*
*     dir  The directory where the .conf files are				      I
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int getNodeInfoFromLocalFile(const char *dir)
{
    char *tmp;
    char *filenameBuffer;

    logMessage(DEBUG, "getNodeInfo(%s)", dir);

    /* Work out temp dir first */
    determineTmpDir();

    installPath_ = safe_strdup(dir);

    /* Get our hostname */
    globus_libc_gethostname(thisHostname_, 256);

    /* Read primary node name from config file */
    if (safe_asprintf(&filenameBuffer, "%s/nodes.conf", getQcdgridPath())<0) 
    {
	errorExit("Out of memory in getNodeInfo");
    }
    if (!loadConfigFile(filenameBuffer, "mainnodeinfo"))
    {
	globus_libc_free(filenameBuffer);
	logMessage(5, "Cannot parse nodes.conf");
	return 0;
    }
    globus_libc_free(filenameBuffer);

    tmp = getFirstConfigValue("mainnodeinfo", "node");
    if (!tmp)
    {
	logMessage(5, "No node specified in nodes.conf");
	return 0;
    }
    primaryNode_ = safe_strdup(tmp);
    tmp = getFirstConfigValue("mainnodeinfo", "path");
    if (!tmp)
    {
	logMessage(5, "No path specified in nodes.conf");
	return 0;
    }
    primaryNodePath_ = safe_strdup(tmp);

    /* Add Metadata Catalogue server location */
    tmp = getFirstConfigValue( "mainnodeinfo", "mdc" );
    if (tmp)
    {
        mdcLocation_ = safe_strdup(tmp);
    }
    else
    {	
        logMessage(5, "No metadata catalogue server specified. Use mdc=server:port in nodes.conf" );
        return 0;
    }

    /* Try copying main config file from primary node */
    mainNode_=primaryNode_;

    if(!initialiseLocally())
    {
	    logMessage(5, "Primary node is down and secondary cannot be used for "
		       "this operation");
	    return 0;
	 }

    /* process main node list */
    tmp=getFirstConfigValue("nodelist", "node");
    while (tmp)
    {
	char *key, *value;
		gridNodes_=globus_libc_realloc(gridNodes_, (numGridNodes_+1)*sizeof(storageElement_t)); 
	if (!gridNodes_) 
	{
	    errorExit("Out of memory in getNodeInfo");
	}

	/* Initialise node structure */
	gridNodes_[numGridNodes_].name=safe_strdup(tmp);
	if (!gridNodes_[numGridNodes_].name)
	{
	    errorExit("Out of memory in getNodeInfo");
	}
	gridNodes_[numGridNodes_].site=NULL;
	gridNodes_[numGridNodes_].freeSpace=0;
	gridNodes_[numGridNodes_].inbox=NULL;
	gridNodes_[numGridNodes_].path=NULL;
	gridNodes_[numGridNodes_].extraRsl=NULL;
	gridNodes_[numGridNodes_].extraJssContact=NULL;
	gridNodes_[numGridNodes_].jobTimeout=-1.0;
	gridNodes_[numGridNodes_].ftpTimeout=-1.0;
	gridNodes_[numGridNodes_].copyTimeout=-1.0;
	gridNodes_[numGridNodes_].gpfs = 0;
	gridNodes_[numGridNodes_].storageElementType = GLOBUS;
	gridNodes_[numGridNodes_].properties = newKeyAndValueHashTable();

	/* Iterate over all node's other parameters in config file */
	key=getNextConfigKeyValue("nodelist", &value);

	/* Loop until end of file or next node is reached */
	while ((key!=NULL)&&(strcmp(key, "node")))
	{
	    addKeyAndValueToHashTable(gridNodes_[numGridNodes_].properties, key, value);
	    if (!strcmp(key, "site"))
	    {
		gridNodes_[numGridNodes_].site=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].site)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "type")) {
				if (!strcmp(value, "globus")) {
					gridNodes_[numGridNodes_].storageElementType = GLOBUS;
					initSEtoGlobus(&gridNodes_[numGridNodes_]);
				} else if (!strcmp(value, "srm")) {
					gridNodes_[numGridNodes_].storageElementType = SRM;
					initSEtoSRM(&gridNodes_[numGridNodes_]);
				} else {
					logMessage(ERROR, "Unrecognised storage type: %s", value);
				}
			}
	    else if (!strcmp(key, "inbox"))
	    {
		gridNodes_[numGridNodes_].inbox=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].inbox)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "path"))
	    {
		gridNodes_[numGridNodes_].path=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].path)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "disk"))
	    {
		gridNodes_[numGridNodes_].freeSpace=strtoll(value, NULL, 10);
	    }
	    else if (!strcmp(key, "extrarsl"))
	    {
		gridNodes_[numGridNodes_].extraRsl=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].extraRsl)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "extrajsscontact"))
	    {
		gridNodes_[numGridNodes_].extraJssContact=safe_strdup(value);
		if (!gridNodes_[numGridNodes_].extraJssContact)
		{
		    errorExit("Out of memory in getNodeInfo");
		}
	    }
	    else if (!strcmp(key, "jobtimeout"))
	    {
		gridNodes_[numGridNodes_].jobTimeout=atof(value);
	    }
	    else if (!strcmp(key, "ftptimeout"))
	    {
		gridNodes_[numGridNodes_].ftpTimeout=atof(value);
	    }
	    else if (!strcmp(key, "copytimeout"))
	    {
		gridNodes_[numGridNodes_].copyTimeout=atof(value);
	    }
	    else if (!strcmp(key, "gpfs"))
	    {
		gridNodes_[numGridNodes_].gpfs = atoi(value);
	    }
	    else
	    {
		if (strcmp(key, "multidisk"))
		{
		    logMessage(3, "Unrecognised attribute '%s' for node %s", key, tmp);
		}
	    }
	    
	    key=getNextConfigKeyValue("nodelist", &value);
	}

	/* Check all the mandatory parameters existed */
	if ((gridNodes_[numGridNodes_].site==NULL)||(gridNodes_[numGridNodes_].path==NULL))
	{
	    logMessage(5, "Node %s is missing a required attribute", tmp);
	    return 0;
	}

	tmp=value;
	numGridNodes_++;
    }

    return 1;
}


/***********************************************************************
*   void destroyNodeInfo()
*
*   Frees all the storage used by the node info in this module
*
*   Parameters:                                                    [I/O]
*
*     None
*   
*   Returns: (void)
***********************************************************************/
void destroyNodeInfo()
{
    int i;

    logMessage(1, "destroyNodeInfo()");

    /*
     * Free miscellaneous small strings
     */
    if (tmpDir_)
    {
	globus_libc_free(tmpDir_);
    }
    if (installPath_)
    {
	globus_libc_free(installPath_);
    }
    if (primaryNode_)
    {
	globus_libc_free(primaryNode_);
    }
    if (primaryNodePath_)
    {
	globus_libc_free(primaryNodePath_);
    }
    if (mdcLocation_)
    {
	globus_libc_free(mdcLocation_);
    }

    /*
     * Free node lists
     */
    destroyNodeList(prefList_);
    destroyNodeList(deadList_);
    destroyNodeList(disabledList_);
    destroyNodeList(retiringList_);
  
    /* 
     * Free main node info structure
     */
    for (i = 0; i < numGridNodes_; i++)
    {
	globus_libc_free(gridNodes_[i].name);
	globus_libc_free(gridNodes_[i].site);
	globus_libc_free(gridNodes_[i].path);
	if (gridNodes_[i].extraRsl)
	{
	    globus_libc_free(gridNodes_[i].extraRsl);
	}
	if (gridNodes_[i].extraJssContact)
	{
	    globus_libc_free(gridNodes_[i].extraJssContact);
	}
    }
    globus_libc_free(gridNodes_);

    /*
     * Free config files
     */
    freeConfigFile("mainnodeinfo");
    freeConfigFile("miscconf");
    freeConfigFile("nodelist");
}
