/***********************************************************************
*
*   Filename:   verify.c
*
*   Authors:    James Perry, Radoslaw Ostrowski  (jamesp, radek)   EPCC.
*
*   Purpose:    Code to verify that QCDgrid replica catalogue matches
*               actual files on grid
*
*   Contents:   Verification functions
*
*   Used in:    Background process and command line verification
*               utility
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

#include "misc.h"
#include "job.h"
#include "replica.h"
#include "node.h"
#include "verify.h"
#include "hashtable.h"

/*
 * Counters which are used to print some statistics at the end of the
 * check.
 */
int inconsistencies_ = 0;
int unrepairedInconsistencies_ = 0;

/*
 * We allow certain inconsistencies to exist and don't bother warning
 * the user about them. This is because files in the process of being
 * replicated will not be registered in the catalogue yet. So this
 * points to a list of files which are known not to be in the
 * catalogue, and they are ignored by the verification
 */
char **allowedInconsistencies_ = NULL;

/***********************************************************************
*   char getUserResponse(char *prompt)
*    
*   Displays a prompt and gets the user's input in response to that
*   prompt. Used for displaying menus giving the user choices about how
*   to repair any inconsistencies in the grid.
*    
*   Parameters:                                [I/O]
*
*     prompt   the prompt string to display     I
*    
*   Returns: the first character of the user's input
***********************************************************************/
static char getUserResponse(char *prompt)
{
    char input[20];

    globus_libc_printf(prompt);
    fgets(input, 20, stdin);
    return input[0];
}

/*
 * This variable holds the number of storage disks on the node currently
 * being processed
 */
static int numDisksOnCurrentNode_;

/*
 * This structure encapsulates useful information about copies of a
 * file on multiple storage disk systems
 */
typedef struct multiDiskCopiesInfo_s
{
    int numCopies;

    /* array with disk name for each copy */
    char **diskNames;

    /* array with length for each copy */
    long long *lengths;

} multiDiskCopiesInfo_t;

/***********************************************************************
*   int promptForMultipleCopiesFix(multiDiskCopiesInfo_t *mdci,
*                                  char *host, char *lfn,
*                                  int interactive)
*    
*   Displays information about the copies of a file on a multidisk
*   system. If running in interactive mode, prompts the user to decide
*   which copy to keep, deletes the others and registers correctly in
*   the replica catalogue
*    
*   Parameters:                                           [I/O]
*
*     mdci         contains details of copies of file      I
*     host         hostname of machine being processed     I
*     lfn          logical name of file being processed    I
*     interactive  set if running in interactive mode      I
*    
*   Returns: 1 if the issue has been resolved,
*            0 if nothing has been done,
*           -1 if an error occurred
***********************************************************************/
static int promptForMultipleCopiesFix(multiDiskCopiesInfo_t *mdci,
				      char *host, char *lfn,
				      int interactive,
				      struct storageElement *se)
{
    int i, response, toKeep;
    char *filename;
    long long longest = 0;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];

    inconsistencies_++;
    globus_libc_printf("\n[%s] There are %d copies of file %s on host %s\n", textTime(),
		       mdci->numCopies, lfn, host);
    for (i = 0; i < mdci->numCopies; i++)
    {
	globus_libc_printf("  %d. in %s, length %d\n", i+1, mdci->diskNames[i], (int)mdci->lengths[i]);
	if (mdci->lengths[i] > longest)
	{
	    longest = mdci->lengths[i];
	}
    }

    if (interactive)
    {
	response = getUserResponse("Which copy do you want to keep? (default=all) ");

	if ((response >= '1') && ((response - '0') <= mdci->numCopies))
	{
	    toKeep = response - '1';

	    if (mdci->lengths[toKeep] < longest)
	    {
		globus_libc_printf("\nWarning, you have not selected the largest file\n");
		response = getUserResponse("Are you sure you want to proceed? (Y/N) ");
		if ((response != 'y') && (response != 'Y'))
		{
		    return 0;
		}
	    }
	    
	    for (i = 0; i < mdci->numCopies; i++)
	    {
		if (toKeep == i)
		{
		    /* this is the one we're keeping */
		    /* this may fail if file is already registered, but that doesn't matter */
		    registerFileWithRc(host, lfn);
		    setDiskInfo(host, lfn, mdci->diskNames[i]);
		}
		else
		{
		    /* remove this one */
		    if (safe_asprintf(&filename, "%s/%s/%s", getNodePath(host), mdci->diskNames[i],
				      lfn) < 0)
		    {
			errorExit("Out of memory in catalogue verification");
		    }
		    globus_libc_printf("Removing copy on disk %s\n", mdci->diskNames[i]);
		    if (se->digs_rm(errbuf, host, filename) != DIGS_SUCCESS) {
			logMessage(5, "Error removing copy on disk %s: %s",
				   mdci->diskNames[i], errbuf);
			globus_libc_free(filename);
			return 0;
		    }
		    globus_libc_free(filename);
		}
	    }
	    return 1;
	}
    }

    return 0;
}

/***********************************************************************
*   multiDiskCopiesInfo_t *getMultiDiskCopiesInfo(char *host, char *lfn)
*    
*   Gets information about all the copies of a certain file on a
*   multidisk system
*    
*   Parameters:                                       [I/O]
*
*     host  the name of the machine being processed    I
*     lfn   the logical name of the file               I
*    
*   Returns: structure containing copy information, NULL on error
***********************************************************************/
static multiDiskCopiesInfo_t *getMultiDiskCopiesInfo(char *host, char *lfn,
						     struct storageElement *se)
{
    multiDiskCopiesInfo_t *info;
    char *diskName, *fileName;
    int i;
    long long len;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];

    /* allocate and initialise structure */
    info = globus_libc_malloc(sizeof(multiDiskCopiesInfo_t));
    if (info == NULL)
    {
	errorExit("Out of memory in getMultiDiskCopiesInfo");
    }

    info->numCopies = 0;
    info->diskNames = NULL;
    info->lengths = NULL;

    for (i = 0; i < numDisksOnCurrentNode_; i++)
    {
	if (i == 0)
	{
	    if (safe_asprintf(&diskName, "data") < 0)
	    {
		errorExit("Out of memory in getMultiDiskCopiesInfo");
	    }
	}
	else
	{
	    if (safe_asprintf(&diskName, "data%d", i) < 0)
	    {
		errorExit("Out of memory in getMultiDiskCopiesInfo");
	    }
	}
	
	if (safe_asprintf(&fileName, "%s/%s/%s", getNodePath(host),
			  diskName, lfn) < 0)
	{
	    errorExit("Out of memory in getMultiDiskCopiesInfo");
	}


	if (se->digs_getLength(errbuf, fileName, host, &len) == DIGS_SUCCESS)
	{
	    /* found a copy of the file */
	    info->numCopies++;
	    info->diskNames = globus_libc_realloc(info->diskNames,
						  info->numCopies*sizeof(char*));
	    info->lengths = globus_libc_realloc(info->lengths,
						info->numCopies*sizeof(long long));
	    if ((!info->diskNames) || (!info->lengths))
	    {
		errorExit("Out of memory in getMultiDiskCopiesInfo");
	    }
	    
	    info->diskNames[info->numCopies-1] = safe_strdup(diskName);
	    if (!info->diskNames[info->numCopies-1])
	    {
		errorExit("Out of memory in getMultiDiskCopiesInfo");
	    }
	    info->lengths[info->numCopies-1] = len;
	}
	
	globus_libc_free(fileName);
	globus_libc_free(diskName);
    }


    return info;
}

/***********************************************************************
*   void destroyMultiDiskCopiesInfo(multiDiskCopiesInfo_t *info)
*    
*   Frees the memory used by a structure containing multidisk copy info
*    
*   Parameters:                                       [I/O]
*
*     info    structure containing copy information    I
*    
*   Returns: (void)
***********************************************************************/
static void destroyMultiDiskCopiesInfo(multiDiskCopiesInfo_t *info)
{
    int i;

    for (i = 0; i < info->numCopies; i++)
    {
	globus_libc_free(info->diskNames[i]);
    }
    if (info->diskNames)
    {
	globus_libc_free(info->diskNames);
    }
    if (info->lengths)
    {
	globus_libc_free(info->lengths);
    }
    globus_libc_free(info);
}

/*
 * Structure used as callback parameter for extraRcEntryCallback
 */
typedef struct rc_entry_callback_param_s
{
    char *hostname;
    int interactive;
} rc_entry_callback_param_t;

/***********************************************************************
*   void extraRcEntryCallback(char *filename, void *param)
*    
*   Callback for iterating over hash table. Called for every extra
*   replica catalogue entry without a corresponding file.
*    
*   Parameters:                                       [I/O]
*
*     filename  the logical name of the file           I
*     param     structure containing hostname and
*               interactive flag                       I
*    
*   Returns: (void)
***********************************************************************/
static void extraRcEntryCallback(char *filename, void *param)
{
    char *host;
    int interactive;
    rc_entry_callback_param_t *p;

    p = (rc_entry_callback_param_t *)param;
    host = p->hostname;
    interactive = p->interactive;
    
    inconsistencies_++;
    globus_libc_printf("\n[%s] File %s doesn't exist on host %s\n", textTime(), 
		       filename, host);

    if (interactive == 1)
    {
	switch(getUserResponse("Do you want to:\n"
			       "  (1) Delete the replica catalogue entry\n"
			       "  (2) Do nothing (default)\n? "))
	{
	case '1':
	    globus_libc_printf("Deleting RC entry...\n\n");
	    if (!removeFileFromLocation(host, filename))
	    {
		globus_libc_fprintf(stderr, "Error unregistering file.\n");
		unrepairedInconsistencies_++;
	    }
	    break;
	default:
	    globus_libc_printf("Doing nothing...\n\n");
	    unrepairedInconsistencies_++;
	    break;
	}
    }
    // delete without asking
    else if (interactive == -1)
    {
	globus_libc_printf("Deleting RC entry...\n\n");
	if (!removeFileFromLocation(host, filename))
	{
	    globus_libc_fprintf(stderr, "Error unregistering file.\n");
	    unrepairedInconsistencies_++;
	}
    }

}

/***********************************************************************
*   int handleMultipleCopies(char *filename, char *host,
*                            qcdgrid_hash_table_t *ignore,
*                            int interactive)
*    
*   Checks whether there are multiple copies of a file on a host. If
*   so, prompts the user to fix this (in interactive mode)
*    
*   Parameters:                                                  [I/O]
*
*     filename  the logical name of the file                      I
*     host      host on which file was found                      I
*     ignore    file is added to this hash table if a problem is
*               found and fixed. May be NULL if not required      I
*     interactive
*               whether to prompt the user for solution           I
*    
*   Returns: 1 if multiple copies found, 0 if not, -1 on error
***********************************************************************/
static int handleMultipleCopies(char *filename, char *host,
				qcdgrid_hash_table_t *ignore, int interactive,
				struct storageElement *se)
{
    multiDiskCopiesInfo_t *mdci;
    
    if (interactive)
    {
      mdci = getMultiDiskCopiesInfo(host, filename, se);
	if (!mdci) return -1;
	
	if (mdci->numCopies > 1)
	{
	    if (promptForMultipleCopiesFix(mdci, host, filename, interactive, se))
	    {
		if (ignore)
		{
		    addToHashTable(ignore, filename);
		}
	    }
	    
	    destroyMultiDiskCopiesInfo(mdci);
	    return 1;
	}
	destroyMultiDiskCopiesInfo(mdci);
    }
    else
    {
	logMessage(3, "Possible problem with disk name of %s", filename);
    }
    return 0;
}

/***********************************************************************
*   int wrongDisk(char *filename, char *host, char *currentMdDiskName,
*                 int interactive)
*    
*   Called when a file is found on the wrong disk of a node
*    
*   Parameters:                                               [I/O]
*
*     filename  the logical name of the file                   I
*     host      the host name on which file was found          I
*     currentMdDiskName
*               name of disk file was found on                 I
*     interactive
*               whether to prompt user for fixes               I
*
*   Returns: 1 on success, 0 on error
***********************************************************************/
static int wrongDisk(char *filename, char *host, char *currentMdDiskName,
		     int interactive, struct storageElement *se)
{
    int result;

    if (interactive)
    {
	/*
	 * There might actually be multiple copies of the file here.
	 * Check for this and fix it
	 */
        result = handleMultipleCopies(filename, host, NULL, interactive, se);
	if (result < 0) return 0;
	if (!result)
	{
	    /* There is only one copy, but it has the wrong name set */
	    globus_libc_fprintf( stderr,
				 "\n[%s] File %s on host %s has incorrect disk name "
				 "(should be %s)\n", textTime(), filename, host,
				 currentMdDiskName);
	    if (interactive)
	    {
		switch (getUserResponse("Do you want to:\n"
					"  (1) Correct the disk name\n"
					"  (2) Do nothing (default)\n? "))
		{
		case '1':
		    globus_libc_printf("Correcting disk name...\n");
		    if (!setDiskInfo(host, filename, currentMdDiskName))
		    {
			globus_libc_fprintf(stderr, "Error setting disk name\n");
			unrepairedInconsistencies_++;
		    }
		    break;
		default:
		    unrepairedInconsistencies_++;
		    break;
		}
	    }
	}
    }
    else
    {
	logMessage(3, "Possible problem with disk name of %s", filename);
    }

    return 1;
}



void checkAndHandleAttributes(char *filename, char *host, int interactive,
			      qcdgrid_hash_table_t *group_d,
			      qcdgrid_hash_table_t *permissions_d,
			      qcdgrid_hash_table_t *md5sum_d,
			      qcdgrid_hash_table_t *size_d,
			      qcdgrid_hash_table_t *submitter_d,
			      struct storageElement *se)
{   
  char *submitterUnknown = "<unknown>";
  int failuresCount;
  int failuresLimit = 10;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  
  /* Check if all the attributes are set in the RLS */   
//  logMessage(1, "lookupValueInHashTable(submitter_d, filename)");
  if(lookupValueInHashTable(submitter_d, filename) == NULL)
  {
    inconsistencies_++;
    globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute submitter\n", filename, host);
    if (interactive >= 1)
    {
      switch (getUserResponse("Do you want to:\n"
			      "  (1) Set up '<unknown>' submitter\n"
			      "  (2) Do nothing (default)\n? "))
      {
        case '1':
	  if(!registerAttrWithRc(filename, "submitter", submitterUnknown)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	  }
	  break;
	default:
	  globus_libc_printf("Doing nothing...\n\n");
	  unrepairedInconsistencies_++;
	  break;
      }
    }

// -1 = force default values
    else if (interactive == -1)
    {
// RADEK fix without asking
	globus_libc_fprintf(stderr, "Registering attribute submitter for file %s at node %s\n", filename, host);
	if(!registerAttrWithRc(filename, "submitter", submitterUnknown)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	}

    }
  }

//  logMessage(1, "lookupValueInHashTable(group_d, filename)");
  if(lookupValueInHashTable(group_d, filename) == NULL)
  {
    char *defaultGroup = "ukq";
    
    inconsistencies_++;
    globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute group\n", filename, host);
	
    if (interactive >= 1)
    {
      switch (getUserResponse("Do you want to:\n"
			      "  (1) Set up 'ukq' group\n"
			      "  (2) Do nothing (default)\n? "))
      {
        case '1':
	  if(!registerAttrWithRc(filename, "group", defaultGroup)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	  }
	  break;
        default:
	  globus_libc_printf("Doing nothing...\n\n");
	  unrepairedInconsistencies_++;
	  break;
      }
    }
    else if (interactive == -1)
    {
	globus_libc_fprintf(stderr, "Registering attribute group for file %s at node %s\n", filename, host);
	if(!registerAttrWithRc(filename, "group", defaultGroup)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	}

    }
  }


  if(lookupValueInHashTable(permissions_d, filename) == NULL)
  {
    char *defaultPermissions = "private";
    char *alternativePermissions = "public";
    
    inconsistencies_++;
    globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute permissions\n", filename, host);

    if (interactive >= 1)
    {
      switch (getUserResponse("Do you want to:\n"
			      "  (1) Set up 'private' permissions\n"
			      "  (2) Set up 'public' permissions\n"
			      "  (3) Do nothing (default)\n? "))
      {
        case '1':
	  if(!registerAttrWithRc(filename, "permissions", defaultPermissions)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	  }
	  break;
	case '2':
	  if(!registerAttrWithRc(filename, "permissions", alternativePermissions)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	  }
	  break;
	default:
	  globus_libc_printf("Doing nothing...\n\n");
	  unrepairedInconsistencies_++;
	  break;
      }
    }
    else if (interactive == -1)
    {
	globus_libc_fprintf(stderr, "Registering attribute permissions as default for file %s at node %s\n", filename, host);
	if(!registerAttrWithRc(filename, "permissions", defaultPermissions)){
	    globus_libc_fprintf(stderr, "Error registering with RLS\n");;
	}
    }

  }

  if(lookupValueInHashTable(size_d, filename) == NULL || strcmp(lookupValueInHashTable(size_d, filename), "-1") == 0)
  {
      long long calcSize;
      char *sizeString;
      char *fullPath = constructFilename(host, filename);

      globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute size\n", filename, host);

      if(lookupValueInHashTable(md5sum_d, filename) != NULL)
      {
	  globus_libc_fprintf(stderr, "File %s at node %s DOES have replica entry for attribute md5sum\n", filename, host);

	  if (interactive >= 1)
	  {
	      switch (getUserResponse("Do you want to:\n"
				      "  (1) Set up 'size' attribute\n"
				      "  (2) Do nothing (default)\n? "))
	      {
	      case '1':
		  if(verifyMD5SumWithRLS(host, filename, fullPath) == 1)
		  {
		      

		      /* calculate size on the file */
		      // try up to 10 times in case of temporary problems 
		      for(failuresCount=0; failuresCount<failuresLimit; failuresCount++)
		      {
			  if (se->digs_getLength(errbuf, fullPath, host, &calcSize) == DIGS_SUCCESS) break;
			  globus_libc_printf("Calculating size failed %d times. Trying again...\n", failuresCount);
			  if (failuresCount >= failuresLimit-1)
			  {
			      globus_libc_printf("Too many errors!. Skipping this file\n");
			      return;
			  }
		      }
		      
		      /* and create attr as a string */  
		      if (safe_asprintf(&sizeString, "%d", calcSize) < 0)
		      {
			  globus_libc_printf("Out of memory in check-one-file");
		      }
		      
		      if(!registerAttrWithRc(filename, "size", sizeString)){
			  globus_libc_fprintf(stderr, "Error registering with RLS\n");
		      }
		      globus_libc_free(sizeString);
		  } 
		  else 
		  {
		      globus_libc_fprintf(stderr, "File is corrupted, not storing 'size' attribute\n");
		      unrepairedInconsistencies_++;
		  }
		  break;
	      default:
		  globus_libc_printf("Doing nothing...\n\n");
		  unrepairedInconsistencies_++;
		  break;
	      }
	  }
	  else if (interactive == -1)
	  {
	      if(verifyMD5SumWithRLS(host, filename, fullPath) == 1)
	      {
                // calculate size on the file 
		  
		  // try up to 10 times in case of temporary problems 
		  for(failuresCount=0; failuresCount<failuresLimit; failuresCount++)
		  {
		      if (se->digs_getLength(errbuf, fullPath, host, &calcSize) == DIGS_SUCCESS) break;
		      globus_libc_printf("Calculating size failed %d times. Trying again...\n", failuresCount);
		      if (failuresCount >= failuresLimit-1)
			  {
			      globus_libc_printf("Too many errors!. Skipping this file\n");
			      return;
			  }
		  }
		  
 
		  // and create attr as a string   
		  if (safe_asprintf(&sizeString, "%d", calcSize) < 0)
		  {
		      globus_libc_printf("Out of memory in check-one-file");
		  }
	      
		  if(!registerAttrWithRc(filename, "size", sizeString)){
		      globus_libc_fprintf(stderr, "Error registering with RLS\n");
		  }
		  globus_libc_free(sizeString);
	      }
	      else 
	      {
		  globus_libc_fprintf(stderr, "File is corrupted, not storing 'size' attribute\n");
		  unrepairedInconsistencies_++;
	      }
	  }

      }
      else
      {
	  //dont verify
	  globus_libc_fprintf(stderr, "File %s at node %s does NOT have replica entry for attribute md5sum\n", filename, host);

	  if (interactive >= 1)
	  {
	      switch (getUserResponse("Do you want to:\n"
				      "  (1) Set up 'size' attribute\n"
				      "  (2) Do nothing (default)\n? "))
	      {
	      case '1':
		  /* calculate size on the file */
		  // try up to 10 times in case of temporary problems 
	      	  for(failuresCount=0; failuresCount<failuresLimit; failuresCount++)
		  {
		      if (se->digs_getLength(errbuf, fullPath, host, &calcSize) == DIGS_SUCCESS) break;
		      globus_libc_printf("Calculating size failed %d times. Trying again...\n", failuresCount);
		      if (failuresCount >= failuresLimit-1)
			  {
			      globus_libc_printf("Too many errors!. Skipping this file\n");
			      return;
			  } 
		  }
		  
		  /* and create attr as a string */  
		  if (safe_asprintf(&sizeString, "%d", calcSize) < 0)
		  {
		      globus_libc_printf("Out of memory in check-one-file");
		  }
  
		  if(!registerAttrWithRc(filename, "size", sizeString)){
		      globus_libc_fprintf(stderr, "Error registering with RLS\n");
		  }
		  globus_libc_free(sizeString);
		  
		  break;
	      default:
		  globus_libc_printf("Doing nothing...\n\n");
		  unrepairedInconsistencies_++;
		  break;
	      }
	  }
	  else if (interactive == -1)
	  {
	      //  register anyway
	      // calculate size on the file 
	      for(failuresCount=0; failuresCount<failuresLimit; failuresCount++)
	      {
		  if (se->digs_getLength(errbuf, fullPath, host, &calcSize) == DIGS_SUCCESS) break;
		  globus_libc_printf("Calculating size failed %d times. Trying again...\n", failuresCount);
		  if (failuresCount >= failuresLimit-1)
			  {
			      globus_libc_printf("Too many errors!. Skipping this file\n");
			      return;
			  }
	      }

		  // and create attr as a string   
		  if (safe_asprintf(&sizeString, "%d", calcSize) < 0)
		  {
		      globus_libc_printf("Out of memory in check-one-file");
		  }
	      
		  if(!registerAttrWithRc(filename, "size", sizeString)){
		      globus_libc_fprintf(stderr, "Error registering with RLS\n");
		  }
		  globus_libc_free(sizeString);

	  }
      }
      globus_libc_free(fullPath);
  }



  if(lookupValueInHashTable(md5sum_d, filename) == NULL)          
  {
    globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute md5sum\n", filename, host);

    inconsistencies_++;

    char *filePath;
    char *newChecksum;
  
	  if (interactive >= 1)
	  {
	      switch (getUserResponse("Do you want to:\n"
				      "  (1) Set up 'md5sum' attribute\n"
				      "  (2) Do nothing (default)\n? "))
	      {
	      case '1':
		  /* Perform remote checksum */
		  filePath = constructFilename(host, filename);
		  
		  // if job failed and failed less then 10 times then print error message and try again	

		  for(failuresCount=0; failuresCount<failuresLimit; failuresCount++)
		  {    
		    if (se->digs_getChecksum(errbuf, filePath, host, &newChecksum, DIGS_MD5_CHECKSUM) ==
			DIGS_SUCCESS) {
		      globus_libc_fprintf(stderr, "New checksum = %s\n", newChecksum); 
		      break;
		    }
		    else
		      {
			globus_libc_fprintf(stderr, "Remote checksum job failed for %d time.\n", failuresCount+1);
		      }
		  }
		  if(failuresCount >= failuresLimit)
		  {
		      globus_libc_printf("Too many errors!. Skipping this file\n");
		      return;
		  }

	  
		  if(!registerAttrWithRc(filename, "md5sum", newChecksum)){
		      globus_libc_fprintf(stderr, "Error registering md5sum with RLS\n");;
		  }
		  
		  globus_libc_free(filePath);
		  se->digs_free_string(&newChecksum);
		  
		  break;
	      default:
		  globus_libc_printf("Doing nothing...\n\n");
		  unrepairedInconsistencies_++;
		  break;
	      }
	  }
	  else if (interactive == -1)
	  {
	      // Perform remote checksum 
	      filePath = constructFilename(host, filename);

	      for(failuresCount=0; failuresCount<failuresLimit; failuresCount++)
	      {    
		if (se->digs_getChecksum(errbuf, filePath, host, &newChecksum, DIGS_MD5_CHECKSUM) ==
		    DIGS_SUCCESS) {
		  globus_libc_fprintf(stderr, "New checksum = %s\n", newChecksum); 
		  break;
		}
		else {
		  globus_libc_fprintf(stderr, "Remote checksum job failed for %d time.\n", failuresCount+1);
		}

	      }
	      if(failuresCount >= failuresLimit)
	      {
		  globus_libc_printf("Too many errors!. Skipping this file\n");
		  return;
	      }

      
	      if(!registerAttrWithRc(filename, "md5sum", newChecksum)){
		  globus_libc_fprintf(stderr, "Error registering md5sum with RLS\n");;
	      }
	      
	      globus_libc_free(filePath);
	      se->digs_free_string(&newChecksum);
	  }
  }

}
  
  
  
  
  

/***********************************************************************
*   int checkOneFile(char *filename, char *host,
*                    qcdgrid_hash_table_t *ignore,
*	  	     qcdgrid_hash_table_t *ht,
*		     qcdgrid_hash_table_t *rcFiles,
*		     int currentMdDiskNum, int numRcDisks,
*		     char *currentMdDiskName,
*		     qcdgrid_hash_table_t **fileDisksList,
*		     int interactive
*		     qcdgrid_hash_table_t *group_d,
*		     qcdgrid_hash_table_t *permissions_d,
*		     qcdgrid_hash_table_t *md5sum_d,
*		     qcdgrid_hash_table_t *size_d,
*		     qcdgrid_hash_table_t *submitter_d)
*    
*   Checks one file found on a node against the replica catalogue
*    
*   Parameters:                                                [I/O]
*
*     filename  logical name of file found on host              I
*     host      host where file was found                       I
*     ignore    hash table of files already dealt with          I
*     ht        hash table used to check for duplicate files    I
*     rcFiles   hash table containing files registered at node  I
*     currentMdDiskNum
*               disk number on which file was found             I
*     numRcDisks
*               number of disks on this node                    I
*     currentMdDiskName
*               disk name on which file was found               I
*     fileDisksList
*               array of hash tables containing files on disks  I
*     interactive
*               whether to prompt user for fixes                I
*     group_d
*               list of all lfns and correponding mappings 
*               for a group attribute                           I
*     permissions_d
*               for permissions attribute                       I
*     md5sum_d
*               for md5sum attribute                            I
*     size_d
*               for size attribute                              I
*     submitter_d
*               for submitter attribute                         I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int checkOneFile(char *filename, char *host,
			qcdgrid_hash_table_t *ignore,
			qcdgrid_hash_table_t *ht,
			qcdgrid_hash_table_t *rcFiles,
			int currentMdDiskNum, int numRcDisks,
			char *currentMdDiskName,
			qcdgrid_hash_table_t **fileDisksList,
			int interactive,
			qcdgrid_hash_table_t *group_d,  /* pointers to attribute dictionaries */
			qcdgrid_hash_table_t *permissions_d,
			qcdgrid_hash_table_t *md5sum_d,
			qcdgrid_hash_table_t *size_d,
			qcdgrid_hash_table_t *submitter_d,
			struct storageElement *se)
{
    int multipleCopies;
    int j;
    int allowed;
    int found;
    int result;
    char *filePath;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];

    /* Ignore files in the NEW directory - they shouldn't have catalogue
     * entries
     */
    if (strncmp(filename, "NEW/", 4))   /* remember returns 0 on match */
    {
	/*
	 * On multidisk machines, we also check whether there are multiple
	 * copies on different storage disks. The hash table holds all the
	 * LFNs we've already found - if the name we're processing is in
	 * the hash table, there must be multiple copies of the file
	 */

	if (lookupHashTable(ignore, filename))
	{
	    /*
	     * This filename has already been dealt with - ignore it
	     */
	    return 1;
	}
	
	if (lookupHashTable(ht, filename))
	{
	    multipleCopies=1;
	}
	else
	{
	    multipleCopies = 0;
	    addToHashTable(ht, filename);
	}
	
	/*
	 * Print a warning if multiple copies are found. Ideally, would like
	 * to display a list of copies, their sizes, dates, md5 sums, and
	 * give the user the option to keep one copy and automatically remove
	 * all the others
	 */
	if (multipleCopies)
	{
	    logMessage(3, "Warning, multiple copies of file %s on host %s",
		       filename, host);
	}
	
	found = lookupHashTableAndRemove(rcFiles, filename);

	if (found)
	{
	    /*
	     * multidisk: found the file, now check it's in the
	     * directory it's supposed to be in
	     */

	    if ((currentMdDiskNum >= numRcDisks) ||
		(!lookupHashTable(fileDisksList[currentMdDiskNum], filename)))
	    {
		inconsistencies_++;

		if (!wrongDisk(filename, host, currentMdDiskName, interactive, se))
		{
		    return 0;
		}
	    } /* end code for file on wrong disk */


	    /* Perform a check if all of the attributes exist for a given filename*/
	    checkAndHandleAttributes(filename, host, interactive,
				     group_d, permissions_d,
				     md5sum_d, size_d, submitter_d, se);


	}
	else /* file on disk not found in catalogue */
	{
	    /*
	     * Check if this is one of the allowed inconsistencies
	     */
	    allowed=0;
	    if (allowedInconsistencies_)
	    {
		j=0;
		while (allowedInconsistencies_[j])
		{
		    if (!strcmp(allowedInconsistencies_[j], filename))
		    {
			allowed=1;
			break;
		    }
		    j++;
		}
	    }
	    
	    if (!allowed)
	    {
		/* We reached the end of the RC list without finding a match.
		 * That means there's a file with no RC entry
		 */
		inconsistencies_++;
		
		if (multipleCopies)
		{
		    /* 
		     * This file has no replica catalogue entry, but there's another copy
		     * of it on another disk that does. Need to work out which one is the
		     * correct copy
		     */
		  if (handleMultipleCopies(filename, host, ignore, interactive, se) < 0)
		    {
			return 0;
		    }
		}
		else /* not multiple (known) copies */
		{
		    /*
		     * A file on a multidisk system has no replica catalogue entry.
		     * Check here for duplicate copies of it on the different disks.
		     * If no duplicates are found, fall through to single disk handler
		     */
		  result = handleMultipleCopies(filename, host, ignore, interactive, se);
		    if (result < 0) return 0;
		    if (result)
		    {
			return 1;
		    }
		    
		    globus_libc_printf("\n[%s] File %s on host %s has no replica catalogue entry\n",
				       textTime(), filename, host);
		    
		    if (interactive >= 1)
		    {
			switch (getUserResponse("Do you want to:\n"
						"  (1) Create an entry for the file\n"
						"  (2) Delete the file\n"
						"  (3) Do nothing (default)\n? "))
			{
			case '1':
			    globus_libc_printf("Adding a replica catalogue entry...\n\n");
			    if (!registerFileWithRc(host, filename))
			    {
				globus_libc_fprintf(stderr, "Error registering file.\n");
				unrepairedInconsistencies_++;
			    }
			    
			    /*
			     * Add attribute giving directory if multidisk
			     */
			    if (!setDiskInfo(host, filename, currentMdDiskName))
			    {
				globus_libc_fprintf(stderr, "Error setting disk name\n");
			    }

			    /* Add the remaining attributes */
			    checkAndHandleAttributes(filename, host, interactive,
						     group_d, permissions_d,
						     md5sum_d, size_d, submitter_d, se);
			    break;
			case '2':
			    globus_libc_printf("Deleting the file...\n\n");
			    filePath = constructFilename(host, filename);
			    if (!filePath) {
			      globus_libc_fprintf(stderr, "Error constructing file path\n");
			      unrepairedInconsistencies_++;
			      break;
			    }
			    if (se->digs_rm(errbuf, host, filePath) != DIGS_SUCCESS) {
			      globus_libc_fprintf(stderr, "Error deleting file: %s.\n", errbuf);
			      unrepairedInconsistencies_++;
			    }
			    globus_libc_free(filePath);
			    break;
			default:
			    globus_libc_printf("Doing nothing...\n\n");
			    unrepairedInconsistencies_++;
			    break;
			}
		    }
		    else
		    {
		      /*
		       * See if this is a node that can do scan-based adding, add a new entry
		       * for the file if so
		       */
		      if (nodeSupportsAddScan(host)) {
			
			/*
			 * Only do it if the lfn is not registered yet
			 */
			if (!fileInCollection(filename)) {
			  char *pfn;
			  char *groupAttr;
			  char *md5Attr;
			  char *ownerAttr;
			  char *permsAttr;
			  char *permStr;
			  long long fileSize;
			  char sizebuf[50];

			  /*
			   * FIXME: won't work for multi-disk nodes, but we can't use
			   * constructFilename as the disk attribute won't be there. Not a
			   * problem currently as only OMERO supports this and is always single
			   * disk.
			   */
			  if (safe_asprintf(&pfn, "%s/data/%s", getNodePath(host), filename) < 0) {
			    logMessage(ERROR, "Out of memory");
			    unrepairedInconsistencies_++;
			    return 0;
			  }
			  
			  /*
			   * Get attributes from the node
			   */
			  if (se->digs_getGroup(errbuf, pfn, host, &groupAttr) != DIGS_SUCCESS) {
			    logMessage(ERROR, "Error getting group for %s", filename);
			    globus_libc_free(pfn);
			    unrepairedInconsistencies_++;
			    return 0;
			  }
			  if (se->digs_getOwner(errbuf, pfn, host, &ownerAttr) != DIGS_SUCCESS) {
			    logMessage(ERROR, "Error getting owner for %s", filename);
			    globus_libc_free(groupAttr);
			    globus_libc_free(pfn);
			    unrepairedInconsistencies_++;
			    return 0;
			  }
			  if (se->digs_getChecksum(errbuf, pfn, host, &md5Attr, DIGS_MD5_CHECKSUM) != DIGS_SUCCESS) {
			    logMessage(ERROR, "Error getting checksum for %s", filename);
			    globus_libc_free(pfn);
			    globus_libc_free(groupAttr);
			    globus_libc_free(ownerAttr);
			    unrepairedInconsistencies_++;
			    return 0;
			  }
			  if (se->digs_getPermissions(errbuf, pfn, host, &permsAttr) != DIGS_SUCCESS) {
			    logMessage(ERROR, "Error getting group for %s", filename);
			    globus_libc_free(pfn);
			    globus_libc_free(groupAttr);
			    globus_libc_free(ownerAttr);
			    globus_libc_free(md5Attr);
			    unrepairedInconsistencies_++;
			    return 0;
			  }
			  if (se->digs_getLength(errbuf, pfn, host, &fileSize) != DIGS_SUCCESS) {
			    logMessage(ERROR, "Error getting group for %s", filename);
			    globus_libc_free(groupAttr);
			    globus_libc_free(ownerAttr);
			    globus_libc_free(md5Attr);
			    globus_libc_free(permsAttr);
			    globus_libc_free(pfn);
			    unrepairedInconsistencies_++;
			    return 0;
			  }
			  globus_libc_free(pfn);
			  
			  /*
			   * Register file
			   */
			  if (!registerFileWithRc(host, filename)) {
			    logMessage(ERROR, "Error registering file %s at node %s", filename, host);
			    globus_libc_free(groupAttr);
			    globus_libc_free(ownerAttr);
			    globus_libc_free(md5Attr);
			    globus_libc_free(permsAttr);
			    unrepairedInconsistencies_++;
			    return 0;
			  }

			  /*
			   * Set disk info
			   */
			  setDiskInfo(host, filename, "data");
			  
			  /*
			   * Set other attributes
			   */
			  registerAttrWithRc(filename, "group", groupAttr);
			  registerAttrWithRc(filename, "md5sum", md5Attr);
			  registerAttrWithRc(filename, "submitter", ownerAttr);
			  sprintf(sizebuf, "%qd", fileSize);
			  registerAttrWithRc(filename, "size", sizebuf);
			  if ((permsAttr[3] - '0') & 4) {
			    permStr = "public";
			  }
			  else {
			    permStr = "private";
			  }
			  registerAttrWithRc(filename, "permissions", permStr);

			  globus_libc_free(groupAttr);
			  globus_libc_free(ownerAttr);
			  globus_libc_free(md5Attr);
			  globus_libc_free(permsAttr);
			}
		      }
		    }
#if 0
		    else /* don't ask just do it */
		    {
			globus_libc_printf("Adding a replica catalogue entry...\n\n");
			if (!registerFileWithRc(host, filename))
			{
			    globus_libc_fprintf(stderr, "Error registering file.\n");
			    unrepairedInconsistencies_++;
			}
			
			/*
			 * Add attribute giving directory if multidisk
			 */
			if (!setDiskInfo(host, filename, currentMdDiskName))
			{
			    globus_libc_fprintf(stderr, "Error setting disk name\n");
			}
			
			/* Add the remaining attributes */
			checkAndHandleAttributes(filename, host, interactive,
						 group_d, permissions_d,
						 md5sum_d, size_d, submitter_d, se);
		    }
#endif		    

		} /* end not multiple copies code */
	    } /* end not allowed inconsistency code */
	} /* end file not found code */
    } /* ignore NEW */

    return 1;
}

/***********************************************************************
*   void verifyReplicaCatalogue(char *host, int interactive)
*    
*   Verifies the replica catalogue entries pertaining to the specified
*   host
*    
*   Algorithm: Reads all the filenames registered at the host from the
*   replica catalogue into rcContents. Reads all the filenames
*   actually in the host's storage directory by remotely executing
*   scannode on it, storing the output in nodeContents. Then, for each 
*   filename in nodeContents, looks for the same filename in rcContents.
*   If it finds it, marks the filename in rcContents as "found".
*   If it doesn't find it, the file is missing from the replica
*   catalogue. In interactive mode, the user is given the options of 
*   adding the file to the RC, deleting the file or doing nothing.
*     After checking through all the node contents, the rcContents
*   array is scanned for any entries that are not marked as "found".
*   These represent files that are registered on the RC but missing from
*   the node itself. In interactive mode, the user is given the option 
*   to delete the replica catalogue entry.
*
*   Parameters:                                                  [I/O]
* 
*     host         The FQDN of the host being checked             I
*     interactive  Non-zero value causes the function to prompt   I
*                  the user, giving them choices of options to
*                  fix any inconsistencies
*    
*   Returns: (void)
***********************************************************************/
static void verifyReplicaCatalogue(char *host, int interactive)
{
    char **rcContents;
    int i;

    /* used for extracting all attributes from RLS */
    qcdgrid_hash_table_t *md5sum_d;
    qcdgrid_hash_table_t *submitter_d;
    qcdgrid_hash_table_t *group_d;
    qcdgrid_hash_table_t *size_d;
    qcdgrid_hash_table_t *permissions_d;

    /*
     * This hash table will be initialised with all the filenames read from the
     * replica catalogue. Then whenever we want to look for a file that's
     * present on the node, we look it up in here (and remove it)
     */
    qcdgrid_hash_table_t *rcFiles;
    rc_entry_callback_param_t cbParam;

    /*
     * The 'ht' hash table is used to store all the filenames we've processed
     * so far on a multidisk system, so that duplicates can be easily identified.
     * The 'ignore' hash table stores filenames which should be ignored from now
     * on. This can happen when multiple copies of a file appear on multidisk
     * systems
     */
    qcdgrid_hash_table_t *ht, *ignore;

    qcdgrid_hash_table_t **fileDisksList;
    int numRcDisks;

    struct storageElement *se;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    char **list;
    int listLength;
    int pathlen;
    int disknum = 0;
    char diskname[10];
    char *lfn;

    logMessage(3, "Processing host %s", host);

    /* get storage element struct for node */
    se = getNode(host);
    if (!se) {
      logMessage(ERROR, "Error getting SE struct for %s", host);
      return;
    }

    /* Get files listed at node in RC */
    rcContents=listLocationFiles(host);
    if (!rcContents)
    {
	logMessage(3, "No files currently registered at %s", host);

	/* Create dummy empty list */
	/* FIXME: listLocationFiles probably should do this itself */
	rcContents=globus_libc_malloc(sizeof(char*));
	if (!rcContents) errorExit("Out of memory in verifyReplicaCatalogue");
	rcContents[0]=NULL;
    }

    /* Query RLS for all the disk attributes on multidisk nodes */
    fileDisksList = getAllFileDisks(host);
    if (fileDisksList == NULL)
    {
	freeLocationFileList(rcContents);
	logMessage(3, "Error getting disk info for host %s", host);
	return;
    }


    /* count the number of disk hash tables so we don't index off the end */
    numRcDisks = 0;
    while (fileDisksList[numRcDisks])
    {
	numRcDisks++;
    }

    /* Hash all the RC files */
    rcFiles = newHashTable();
    i = 0;
    while (rcContents[i])
    {
	addToHashTable(rcFiles, rcContents[i]);
	i++;
    }

    freeLocationFileList(rcContents);

    /* Get files actually stored at node */
    result = se->digs_scanNode(errbuf, host, &list, &listLength, 0);
    if (result != DIGS_SUCCESS) {
      logMessage(ERROR, "Error scanning node %s: %s (%s)", host,
		 digsErrorToString(result), errbuf);
      destroyFileDisksList(fileDisksList);
      destroyHashTable(rcFiles);
      return;
    }

    /* get length in characters of initial path to remove */
    pathlen = strlen(getNodePath(host));
    if (getNodePath(host)[pathlen-1] != '/') pathlen++;

    /* First count the number of storage dirs */
    numDisksOnCurrentNode_ = 0;
    for (i = 0; i < listLength; i++) {
      if (strlen(list[i]) < (pathlen+6)) {
	logMessage(3, "Skipping bad line '%s'", list[i]);
	continue;
      }

      if ((list[i][pathlen] != 'd') ||
	  (list[i][pathlen+1] != 'a') ||
	  (list[i][pathlen+2] != 't') ||
	  (list[i][pathlen+3] != 'a')) {
	logMessage(3, "Skipping bad line '%s'", list[i]);
	continue;
      }
      disknum = 0;
      if (isdigit(list[i][pathlen+4])) {
	disknum = list[i][pathlen+4] - '0';
	if (isdigit(list[i][pathlen+5])) {
	  disknum = disknum*10 + (list[i][pathlen+5] - '0');
	}
      }
      if ((disknum+1) > numDisksOnCurrentNode_) {
	numDisksOnCurrentNode_ = disknum + 1;
      }
    }


    /* create a hash table to speed up checking for duplicates */
    ht = newHashTable();	
    
    /* hash table for lfns to ignore because they've already been processed */
    ignore = newHashTable();

    logMessage(3, "This node has %d disks", numDisksOnCurrentNode_);

    /* Obtain attributes for all lfns in RLS */
    group_d = getAllAttributesValues("group");
    permissions_d = getAllAttributesValues("permissions");
    md5sum_d = getAllAttributesValues("md5sum");
    size_d = getAllAttributesValues("size");
    submitter_d = getAllAttributesValues("submitter");


    /* Iterate over all lines of node contents */
    for (i = 0; i < listLength; i++) {

      /* check filename is right sort of path */
      if (strlen(list[i]) < (pathlen + 6)) {
	globus_libc_fprintf(stderr, "Invalid line '%s' returned\n", list[i]);
	/* skip it */
	continue;
      }

      if ((list[i][pathlen] != 'd') ||
	  (list[i][pathlen+1] != 'a') ||
	  (list[i][pathlen+2] != 't') ||
	  (list[i][pathlen+3] != 'a')) {
	globus_libc_fprintf(stderr, "Invalid line '%s' returned\n", list[i]);
	/* skip it */
	continue;
      }

      /* get data directory name and start of lfn */
      lfn = &list[i][pathlen+5];
      if (list[i][pathlen+4] == '/') {
	sprintf(diskname, "data");
      }
      else {
	disknum = list[i][pathlen+4] - '0';
	lfn++;
	if (isdigit(list[i][pathlen+5])) {
	  disknum = disknum*10 + (list[i][pathlen+5] - '0');
	  lfn++;
	}
	sprintf(diskname, "data%d", disknum);
      }

      /* ignore contents of NEW directory */
      if (!strncmp(lfn, "NEW/", 4)) {
	globus_libc_fprintf(stderr, "Skipping %s\n", lfn);
	continue;
      }

      /* run checks on this file */
      if (!checkOneFile(lfn, host, ignore, ht, rcFiles,
			disknum, numRcDisks,
			diskname, fileDisksList,
			interactive, 
			group_d, permissions_d, md5sum_d,
			size_d, submitter_d, se)) {
	  forEachHashTableEntry(rcFiles, extraRcEntryCallback, (void*)&cbParam);

	  se->digs_free_string_array(&list, &listLength);
	  
	  destroyFileDisksList(fileDisksList);

	  destroyHashTable(ht);
	  destroyHashTable(ignore);
	  destroyHashTable(rcFiles);

	  destroyKeyAndValueHashTable(group_d);
	  destroyKeyAndValueHashTable(submitter_d);
	  destroyKeyAndValueHashTable(md5sum_d);
	  destroyKeyAndValueHashTable(size_d);
	  destroyKeyAndValueHashTable(permissions_d);

	  return;
      }
    }

    /* Now check the RC entries to see if there's any 'left over' */
    cbParam.hostname = host;
    cbParam.interactive = interactive;

    forEachHashTableEntry(rcFiles, extraRcEntryCallback, (void*)&cbParam);

    se->digs_free_string_array(&list, &listLength);

    destroyFileDisksList(fileDisksList);

    destroyHashTable(ht);
    destroyHashTable(ignore);
    destroyHashTable(rcFiles);

    destroyKeyAndValueHashTable(group_d);
    destroyKeyAndValueHashTable(submitter_d);
    destroyKeyAndValueHashTable(md5sum_d);
    destroyKeyAndValueHashTable(size_d);
    destroyKeyAndValueHashTable(permissions_d);

}

/***********************************************************************
*   int runGridVerification(char *node, int interactive)
*    
*   Does a complete verification of the grid. Checks that the replica
*   catalogue entries match what is really in the nodes' storage
*   directories.
*    
*   Parameters:                                                [I/O]
*
*     node         which node to run verification on - NULL     I
*                  means all of them
*     interactive  whether to prompt the user and allow them    I
*                  to fix any problems found
*    
*   Returns: 1 if inconsistencies were found, 0 if not.
***********************************************************************/
int runGridVerification(char *node, int interactive)
{
    int nn;
    int i;
    char *host;

    inconsistencies_ = 0;
    unrepairedInconsistencies_ = 0;

    if (node)
    {
	if (!isNodeDisabled(node))
	{
	    verifyReplicaCatalogue(node, interactive);
	}
    }
    else
    {
	/* Loop over every node in the grid */
	nn = getNumNodes();
	for (i = 0; i < nn; i++)
	{
	    host = getNodeName(i);
	    
	    if (!isNodeDisabled(host))
	    {
		verifyReplicaCatalogue(host, interactive);
	    }
	}
    }

    if (inconsistencies_)
    {
	return 1;
    }
    else
    {
	return 0;
    }
}

/* how far we got in checksumming through the list of logical files*/
int checksumLfnListPos_ = 0;

/***********************************************************************
*   int runChecksums()
*    
*   Runs checksums on all copies of the files on the grid and compare
*   them with RLS entries
*    
*   Parameters:                                                [I/O]
*
*     None
*    
*   Returns: 0 if inconsistencies were not found or positive int
*            which is a number of inconsistencies found
***********************************************************************/
int runChecksums(int maxChecksums)
{
    /* The list of files on the grid */
    static char **lfnList = NULL;
    static int numLfns = 0;
    static int lfnSpace = 0;

    int i;
    int result;

    char *lfn;
    char *firstLoc;
    char *otherLoc;
    char *checksDisabled;

    logMessage(1, "runChecksums(%d)", maxChecksums);

    if (numLfns <= checksumLfnListPos_)
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

	lfn = getFirstFile();
	while (lfn)
	{
	    /* Allocate more space if necessary */
	    if (numLfns >= lfnSpace)
	    {
		lfnSpace += 1000;
		lfnList = globus_libc_realloc(lfnList,
					      lfnSpace*sizeof(char*));
		if (!lfnList)
		{
		    errorExit("Out of memory in runChecksums");
		}
	    }
	    lfnList[numLfns] = lfn;
	    numLfns++;
	    
	    lfn = getNextFile();
	}

	if (checksumLfnListPos_ >= numLfns)
	{
	    /* reset list pos if we went off the end */
	    checksumLfnListPos_ = 0;
	}
    }

    /* Now do our quota of checksums for this iteration */
    inconsistencies_ = 0;

    for (i = 0; i < maxChecksums; i++)
    {
	/* Check we haven't checked all yet */
	if (checksumLfnListPos_ >= numLfns)
	{
	    break;
	}

	lfn = lfnList[checksumLfnListPos_];

	logMessage(3, "Checksumming logical file %s", lfn);

	firstLoc = getFirstFileLocation(lfn);
	while ((firstLoc) && ((isNodeDisabled(firstLoc)) || (isNodeDead(firstLoc)))) {
	    firstLoc = getNextFileLocation(lfn);
	}

	if (firstLoc)
	{
	    checksDisabled = getNodeProperty(firstLoc, "disablechecks");
	    if ((checksDisabled == NULL) || (strcmp(checksDisabled, "1")))
	    {
	      /* check first available file */
	      char *pfn1;
	      pfn1 = constructFilename(firstLoc, lfn);
	      
	      // RADEK location might be NULL, if it is, print warning and skip it. ?? maybe dissable the node first
	      
	      if(!verifyGroupAndPermissionsWithRLS(firstLoc, lfn, pfn1))
	      {
		  logMessage(5, "Error occured in verifyGroupAndPermissionsWithRLS", lfn, firstLoc, pfn1);
	      }
	      
	      /* check with RLS what checksum we are expecting */
	      result = verifyMD5SumWithRLS(firstLoc, lfn, pfn1);
	      if(!result)
	      {
		  logMessage(5, "Copy of %s on %s doesn't match with RLS!!\n[%s]", lfn, firstLoc, pfn1);
		  inconsistencies_++;
		  
		  /* don't stop the grid, or remove the file*/ 
		  //removeFileFromLocation(firstLoc, lfn);
		  //deleteRemoteFileFullPath(firstLoc, pfn1);
		  
		  /* disable this node only */
		  logMessage(5, "Disabling %s node", firstLoc);
		  addToDisabledList(firstLoc);
		  
	      }
	      if (result > 0)
	      {
		  updateLastChecked(lfn, firstLoc);
	      }
	      globus_libc_free(pfn1);
	    }
	    if (checksDisabled) globus_libc_free(checksDisabled);

	    otherLoc = getNextFileLocation();

	    while (otherLoc)
	    {
		if ((!isNodeDead(otherLoc)) && (!isNodeDisabled(otherLoc)))
		{
		    checksDisabled = getNodeProperty(otherLoc, "disablechecks");
		    if ((checksDisabled == NULL) || (strcmp(checksDisabled, "1")))
		    {
		      /* check all other available files */
		      char *pfn2;

		      pfn2 = constructFilename(otherLoc, lfn);
		    
		      if(!verifyGroupAndPermissionsWithRLS(otherLoc, lfn, pfn2))
		      {
			logMessage(5, "Error occured in verifyGroupAndPermissionsWithRLS", lfn, otherLoc, pfn2);
		      }

		      result = verifyMD5SumWithRLS(otherLoc, lfn, pfn2);
		      if(!result)
		      {
			logMessage(5, "Copy of %s on %s doesn't match with RLS!!\n[%s]", lfn, otherLoc, pfn2);
			inconsistencies_++;
		 
			logMessage(5, "Disabling %s node", otherLoc);
			addToDisabledList(otherLoc);
			
		      }		
		      if (result > 0)
		      {
			updateLastChecked(lfn, otherLoc);
		      }
		      globus_libc_free(pfn2);
		    }
		    if (checksDisabled) globus_libc_free(checksDisabled);
		}
		otherLoc = getNextFileLocation();
	    }
	}

	checksumLfnListPos_++;
    }
    
    return inconsistencies_;
}
