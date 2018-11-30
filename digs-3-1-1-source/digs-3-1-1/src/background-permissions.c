/***********************************************************************
*
*   Filename:   background-permissions.c
*
*   Authors:    Radoslaw Ostrowski              (radek)   EPCC.
*
*   Purpose:    Code specific for changing permissions of files on the QCDgrid
*
*   Contents:   Functions related to changing permissions of files on grid
*
*   Used in:    Background process on main node of QCDgrid
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2007 The University of Edinburgh
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
#include <sys/stat.h>
#include <unistd.h>

#include "node.h"
#include "misc.h" 
#include "background-permissions.h"
#include "replica.h"

/*
 * Add operations which have been requested but have not yet been
 * done due to a host being unreachable are stored in an array of this
 * structure. Then each time a host comes back from the dead (list),
 * see if there were any pending deletes for that host and execute them.
 *
 * The list also needs to be saved to disk when it changes in case the
 * control thread goes down.
 */
typedef struct pendingPermissions_s
{
    char *lfn;
    char *group;
    char *permissions;
    char *recursive;

} pendingPermissions_t;

int numPendingPermissions_;
pendingPermissions_t *pendingPermissions_;

/***********************************************************************
*   void savePendingPermissionsList()
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
void savePendingPermissionsList()
{
    char *filenameBuffer;
    FILE *f;
    int i;

    logMessage(1, "savePendingPermissionList()");

    if (safe_asprintf(&filenameBuffer, "%s/pendingpermissions", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in savePendingPermissionList");
    }

    f = fopen(filenameBuffer, "w");
    
    for (i = 0; i < numPendingPermissions_; i++)
    {
      // [0 | 1] ukq ukqcd/DWF/ensemble3.1100 public
	globus_libc_fprintf(f, "%s %s %s %s\n", 
			    pendingPermissions_[i].recursive,
			    pendingPermissions_[i].group,
			    pendingPermissions_[i].lfn,
			    pendingPermissions_[i].permissions);
    }
    fclose(f);
    globus_libc_free(filenameBuffer);
}

/***********************************************************************
*   int loadPendingPermissionList()
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
int loadPendingPermissionsList()
{
    char *filenameBuffer;
    char *lineBuffer = NULL;
    int lineBufferSize=0;
    FILE *f;

    int i;
    int numParams = 1;
    char ** params = NULL;
    char *nextSpc;
    char *paramStart;

    logMessage(1, "loadPendingPermissionsList()");

    numPendingPermissions_ = 0;
    pendingPermissions_ = NULL;

    if (safe_asprintf(&filenameBuffer, "%s/pendingpermissions", getQcdgridPath()) < 0)
    {
	errorExit("Out of memory in loadPendingPermissionsList");
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

	if(numParams != 4)
	{
	    logMessage(5, "Warning: malformed pending permissions list:\n%s", lineBuffer);
	    break;
	}

	params = globus_libc_malloc(sizeof(char*) * numParams);
	if (!params)
	{
	    errorExit("Out of memory in parseMessage loadPendingPermissionList");
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
		errorExit("Out of memory in parseMessage loadPendingPermissionsList");
	    }

	    i++;
	}

	pendingPermissions_ = globus_libc_realloc(pendingPermissions_, (numPendingPermissions_+1)
					      *sizeof(pendingPermissions_t));
	if (!pendingPermissions_)
	{
	    fclose(f);
	    globus_libc_free(lineBuffer);

	    /*	    for (i=0; i < numParams; i++)
	    {
	      globus_libc_free(params[i]);
	    }
	    */
	    globus_libc_free(params);

	    return 0;
	}

	pendingPermissions_[numPendingPermissions_].recursive = safe_strdup(params[0]);
	pendingPermissions_[numPendingPermissions_].group = safe_strdup(params[1]);
	pendingPermissions_[numPendingPermissions_].lfn = safe_strdup(params[2]);
	pendingPermissions_[numPendingPermissions_].permissions = safe_strdup(params[3]);

	numPendingPermissions_++;        

	safe_getline(&lineBuffer, &lineBufferSize, f);

	 // reset numParams before reading another line
	 numParams = 1;
    }

    globus_libc_free(lineBuffer);
    /*
    for (i=0; i < numParams; i++)
    {
      globus_libc_free(params[i]);
    }
    */
    globus_libc_free(params);
    fclose(f);

    return 1;
}

/***********************************************************************
*   void addPendingPermissions(int numParams, char ** params)
*    
*   Permissions an change permission operation to the list of pending changes.
*    
*   Parameters:                                        [I/O]
*
*     numParams  Number of parameters                   I
*     params     An array of parameters                 I  
*
*   Returns: (void)
************************************************************************/

void addPendingPermissions(int numParams, char ** params)
{
    int i;

    for(i=0; i<numParams; i++)
    {
      logMessage(1, "arg[%d] = %s", i, params[i]);
    }
 
    pendingPermissions_ = globus_libc_realloc(pendingPermissions_, 
					  (numPendingPermissions_+1)*sizeof(pendingPermissions_t));
    if (!pendingPermissions_)
    {
	errorExit("Out of memory in addPendingPermissions");
    }

    pendingPermissions_[numPendingPermissions_].recursive = safe_strdup(params[0]);
    pendingPermissions_[numPendingPermissions_].group = safe_strdup(params[1]);
    pendingPermissions_[numPendingPermissions_].lfn =  safe_strdup(params[2]);
    pendingPermissions_[numPendingPermissions_].permissions = safe_strdup(params[3]);


    if ((!pendingPermissions_[numPendingPermissions_].lfn)         ||
	(!pendingPermissions_[numPendingPermissions_].group)       ||
	(!pendingPermissions_[numPendingPermissions_].permissions) ||
	(!pendingPermissions_[numPendingPermissions_].recursive))
    {
	errorExit("Out of memory in addPendingPermissions");
    }

    numPendingPermissions_++;
    savePendingPermissionsList();
}

/***********************************************************************
*   void removePendingPermissions(char *lfn)
*    
*   Removes an change permissions operation from the list of pending changes
*    
*   Parameters:                                        [I/O]
*
*     lfn  The Logical Filename                         I
*
*   Returns: (void)
************************************************************************/
void removePendingPermissions(char *lfn)
{
    int i;

    for (i=0; i<numPendingPermissions_; i++)
    {
        if (!strcmp(pendingPermissions_[i].lfn, lfn))
	{	
	    globus_libc_free(pendingPermissions_[i].recursive);
	    globus_libc_free(pendingPermissions_[i].group);
	    globus_libc_free(pendingPermissions_[i].lfn);
	    globus_libc_free(pendingPermissions_[i].permissions);

	    numPendingPermissions_--;
	    for (; i < numPendingPermissions_; i++)
	    {
		pendingPermissions_[i] = pendingPermissions_[i + 1];
	    }

	    pendingPermissions_=globus_libc_realloc(pendingPermissions_, (numPendingPermissions_+1)
						*(sizeof(pendingPermissions_t)));

	    if (!pendingPermissions_)
	    {
		errorExit("Out of memory in removePendingPermissions");
	    }

	    savePendingPermissionsList();
	    return;
	}
    }
}

/***********************************************************************
*   int isInPenndingPermissionsList(char *lfn)
*
*   Checks if a LFN is in the pendingpermissions list (in memory)
*   
*   Parameters:                                           [I/O]
*
*     lfn     to check in the pendingadds list              I
*
*   Returns: 
*     
*     position in the array or -1 if not found
***********************************************************************/
int isInPenndingPermissionsList(char *lfn)
{
  int i;
  
  for(i=0; i<numPendingPermissions_; i++)
  {
    if (!strcmp(pendingPermissions_[i].lfn, lfn))
    {      
      return i;
    }
  }
  return -1;
}

/***********************************************************************
*  void getPendingPermission(char *lfn, char **recursive, char **group, char **permissions)
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
void getPendingPermission(char *lfn, char **recursive, char **group, char **permissions)
{

  int i;
  i = isInPenndingPermissionsList(lfn);

  *recursive   = pendingPermissions_[i].recursive; 
  *group       = pendingPermissions_[i].group; 
  *permissions = pendingPermissions_[i].permissions;
}



void changeAllReplicasPermissions(char *lfn, char *permissions)
{
  char *node;
  char *fullPath;

  node = getFirstFileLocation(lfn);
  while(node)
  {
    fullPath = constructFilename(node, lfn);
    logMessage(1, "changeAllReplicasPermissions: lfn = %s; fullpath = %s; node = %s", lfn, fullPath, node); 

    chmodRemotely(permissions, fullPath, node);

    globus_libc_free(fullPath);
    node = getNextFileLocation();
  }
}

/***********************************************************************
*   int handlePermissionsChangesCallback(char *lfn, void *param)
*
*   Changes permissions on one file in a directory
*    
*   Parameters:                                                    [I/O]
*
*     lfn    logical filename to change permissions on              I
*     param  new permissions for file                               I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int handlePermissionsChangesCallback(char *lfn, void *param)
{
    char *perms = (char *)param;
    if (!registerAttrWithRc(lfn, "permissions", perms))
    {
	logMessage(5, "Error changing permissions attribute in RLS for LFN = %s", lfn);
	return 0;
    }
    changeAllReplicasPermissions(lfn, perms);

    return 1;
}


/***********************************************************************
*   int handlePermissionsChanges()
*
*   Function for registering new permission attributes with RLS
*   and changing the actual permissions on the File Systems
*    
*   Parameters:                                                    [I/O]
*
*     node FQDN of machine to add file to                           I
*     lfn  Logical grid filename to add                             I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/


// registers new permissions with FC
// calls function to locate all the copies and change their permissions
int handlePermissionsChanges()
{
  int i;
  //  char **lfns;

  // go from the back so it won't be influenced by adding new entries or removing those that worked on
  // or break everytime there is a change and recall again.  


  for (i=numPendingPermissions_ - 1; i>=0; i--)
  {


// TODO
 
     //check if user is allowed to change it!!!


    /* not recursive */
    if(strcmp("false", pendingPermissions_[i].recursive) == 0)
    {
      logMessage(1, "recursive = %s", pendingPermissions_[i].recursive);


      /* change permission attribute for given lfn */
      if(!registerAttrWithRc(pendingPermissions_[i].lfn, "permissions", pendingPermissions_[i].permissions))
      {
	logMessage(5, "Error changing permissions attribute in RLS for LFN = %s", pendingPermissions_[i].lfn);
	return 0;
      }

      /* change the file permissions also on remote locations */
      changeAllReplicasPermissions(pendingPermissions_[i].lfn, 
				   pendingPermissions_[i].permissions);


    } /* recursive */
    else if(strcmp("true", pendingPermissions_[i].recursive) == 0)
    {
      logMessage(1, "recursive = %s", pendingPermissions_[i].recursive);

      if (!forEachFileInDirectory(pendingPermissions_[i].lfn, handlePermissionsChangesCallback,
				  pendingPermissions_[i].permissions))
      {
	  return 0;
      }

    } /* wrong syntax */
    else
    {
	logMessage(5, "Error: received unknown parameter: %s; expected 'true' or 'false", pendingPermissions_[i].recursive);
	return 0;      
    }
    removePendingPermissions(pendingPermissions_[i].lfn);
  }
  return 1;
}

