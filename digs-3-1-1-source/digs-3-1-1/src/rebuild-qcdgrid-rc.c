/***********************************************************************
*
*   Filename:   rebuild-qcdgrid-rc.c
*
*   Authors:    James Perry, Radoslaw Ostrowski (jamesp, radek)   EPCC.
*
*   Purpose:    Rebuilds the QCDgrid replica catalogue using information
*               from the storage directories of the actual nodes
*               themselves
*
*   Contents:   Main function, routines specific to this command
*
*   Used in:    Administration of QCDgrid
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

#include <globus_common.h>

#include "misc.h"
#include "job.h"
#include "node.h"
#include "replica.h"

/***********************************************************************
*   void rebuildLocationCatalogue(char *host)
*    
*   Constructs the replica catalogue for the specified host based on the
*   files in its storage directory
*
*   Parameters:                                  [I/O]
*
*     host  The FQDN of the host being processed  I
*    
*   Returns: (void)
***********************************************************************/
//static qcdgrid_hash_table_t *
void rebuildLocationCatalogue(char *host, int size, int md5sum)
{
    char *defaultGroup = "ukq";
    char *defaultPermissions = "private";
    char *submitterUnknown = "<unknown>";
    qcdgrid_hash_table_t *md5sum_d;
    qcdgrid_hash_table_t *submitter_d;
    qcdgrid_hash_table_t *group_d;
    qcdgrid_hash_table_t *size_d;
    qcdgrid_hash_table_t *permissions_d;

    char *tmpSize = NULL;
    char *tmpSubmitter = NULL;
    char *tmpMd5sum = NULL;
    char *tmpGroup = NULL;
    char *tmpPermissions = NULL;

    struct storageElement *se, *se2;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    char **list;
    int listLength;

    int i;
    int pathlen;
    char diskname[10];
    int disknum = -1;
    char *lfn;
    long long int length;
    char lenbuf[50];
    char *tmpMd5LFN;

    globus_libc_printf("Building catalogue for node %s\n", host);

    se = getNode(host);
    if (!se)
    {
	globus_libc_fprintf(stderr, "Error getting node structure\n");
	return;
    }

    /* Obtain attributes for all lfns in RLS */
    globus_libc_printf("Obtaining all attributes\n"); 
    group_d = getAllAttributesValues("group");
    permissions_d = getAllAttributesValues("permissions");
    md5sum_d = getAllAttributesValues("md5sum");
    size_d = getAllAttributesValues("size");
    submitter_d = getAllAttributesValues("submitter");


    globus_libc_printf("Deleting old catalogue\n");

    deleteEntireLocation(host);

    /* Get list of files actually stored at node */
    result = se->digs_scanNode(errbuf, host, &list, &listLength, 0);
    if (result != DIGS_SUCCESS)
    {
	globus_libc_fprintf(stderr, "Error scanning for file list from %s: %s (%s)\n",
			    host, digsErrorToString(result), errbuf);
	return;
    }

    /* get length in characters of initial path to remove */
    pathlen = strlen(getNodePath(host));
    if (getNodePath(host)[pathlen-1] != '/') pathlen++;

    sprintf(diskname, "data");

    /*
     * The first register operation sometimes seems to fail for no obvious
     * reason (maybe if the previous RLS delete went wrong and left the
     * catalogue in a strange state), so do a sacrificial "dummy" write here
     * to take the failure
     */
    registerFileWithRc("dummy", "digs-rebuild-rc-dummy");

    /* Loop over all the filename returned */
    for (i = 0; i < listLength; i++)
    {
      if (!strcmp(&list[i][strlen(list[i])-14], "-QCDGRIDLOCKED")) continue;
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
	if (disknum != list[i][pathlen+4] - '0')
        {
	    printf("data%d", list[i][pathlen+4] - '0');
        }
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

      /* try to actually register file */
      if (!registerFileWithRc(host, lfn)) {
	globus_libc_fprintf(stderr, "Error registering file %s at node %s\n", lfn, host);
	continue;
      }

      /*
       * Add an attribute here
       * specifying which disk
       */
      if (!setDiskInfo(host, lfn, diskname)) {
	globus_libc_fprintf(stderr, "Error adding disk name %s for file %s at node %s\n",
			    diskname, lfn, host);
      }

      /* Check if all the attributes are set in the RLS */
      tmpSubmitter = lookupValueInHashTable(submitter_d, lfn);
      if(tmpSubmitter == NULL) {
	/* attribute doesn't exist, so set to unknown */
	globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute submitter\n", lfn, host);
	if(!registerAttrWithRc(lfn, "submitter", submitterUnknown)){
	  globus_libc_fprintf(stderr, "Error registering submitter with RLS\n");;
	}
      }
      else {
	globus_libc_free(tmpSubmitter);
      }

      tmpGroup = lookupValueInHashTable(group_d, lfn);
      if(tmpGroup == NULL) {
	/* attribute doesn't exist, so create it */
	globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute group\n", lfn, host);
	if(!registerAttrWithRc(lfn, "group", defaultGroup)) {
	  globus_libc_fprintf(stderr, "Error registering group with RLS\n");
	}
      }
      else {
	globus_libc_free(tmpGroup);
      }

      tmpPermissions = lookupValueInHashTable(permissions_d, lfn);
      if(tmpPermissions == NULL) {
	/* attribute doesn't exist, so create it */
	globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry for attribute permissions\n", lfn, host);
	if(!registerAttrWithRc(lfn, "permissions", defaultPermissions)){
	  globus_libc_fprintf(stderr, "Error registering permissions with RLS\n");;
	}
      }
      else {
	globus_libc_free(tmpPermissions);
      }
      
      tmpMd5sum = lookupValueInHashTable(md5sum_d, lfn);
      if((tmpMd5sum == NULL || strcmp(tmpMd5sum, "-1") == 0) && md5sum) {
	/* attribute doesn't exist or is incorrect, so create it*/
	globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry (or has incorrect) for attribute md5sum\n", lfn, host);

	/*
	 * check if this is a Globus node - if not, try to find a Globus node to get the checksum from
	 */
	if (se->storageElementType != GLOBUS) {
	  char *md5pfn;
	  char *md5host = getFirstFileLocation(lfn);
	  se2 = NULL;
	  while (md5host) {
	    se2 = getNode(md5host);
	    if (se2->storageElementType == GLOBUS) {
	      /* found a Globus node */
	      md5pfn = constructFilename(md5host, lfn);
	      break;
	    }
	    md5host = getNextFileLocation();
	  }
	  if (md5host == NULL) {
	    /* didn't find a Globus node hosting the file so just use the first one anyway */
	    se2 = se;
	    md5host = host;
	    md5pfn = safe_strdup(list[i]);
	  }

	  /* get the checksum */
	  result = se2->digs_getChecksum(errbuf, md5pfn, md5host, &tmpMd5LFN, DIGS_MD5_CHECKSUM);
	  if (result == DIGS_SUCCESS) {
	    if (!registerAttrWithRc(lfn, "md5sum", tmpMd5LFN)) {
	      globus_libc_fprintf(stderr, "Error registering md5sum with RLS\n");
	    }
	    se2->digs_free_string(&tmpMd5LFN);

	    updateLastChecked(lfn, md5host);
	  }
	  else {
	    globus_libc_fprintf(stderr, "Error getting checksum for file %s: %s (%s)\n",
				lfn, digsErrorToString(result), errbuf);
	  }
	  globus_libc_free(md5pfn);
	}
	else {
	  /* it's a Globus node, just use it */
	  result = se->digs_getChecksum(errbuf, list[i], host, &tmpMd5LFN, DIGS_MD5_CHECKSUM);
	  if (result == DIGS_SUCCESS) {
	    if(!registerAttrWithRc(lfn, "md5sum", tmpMd5LFN)){
	      globus_libc_fprintf(stderr, "Error registering md5sum with RLS\n");;
	    }
	    se->digs_free_string(&tmpMd5LFN);
	    
	    updateLastChecked(lfn, host);
	  }
	  else {
	    globus_libc_fprintf(stderr, "Error getting checksum for file %s: %s (%s)\n",
				lfn, digsErrorToString(result), errbuf);
	  }
	}
      }
      else {
	globus_libc_free(tmpMd5sum);
      }

      tmpSize = lookupValueInHashTable(size_d, lfn);
      if((tmpSize == NULL || strcmp(tmpSize, "-1") == 0) && size) {
	/* attribute doesn't exist or is incorrect, so create it*/
	globus_libc_fprintf(stderr, "File %s at node %s doesn't have replica entry (or has incorrect) for attribute size\n", lfn, host);
	  
	result = se->digs_getLength(errbuf, list[i], host, &length);
	if (result == DIGS_SUCCESS) {
	  sprintf(lenbuf, "%qd", length);
	  if(!registerAttrWithRc(lfn, "size", lenbuf)){
	    globus_libc_fprintf(stderr, "Error registering size with RLS\n");;
	  }
	}
	else {
	  globus_libc_fprintf(stderr, "Error getting length for file %s: %s (%s)\n",
			      lfn, digsErrorToString(result), errbuf);
	}
      }
      else {
	globus_libc_free(tmpSize);
      }
    }
    
    destroyKeyAndValueHashTable(group_d);
    destroyKeyAndValueHashTable(submitter_d);
    destroyKeyAndValueHashTable(md5sum_d);
    destroyKeyAndValueHashTable(size_d);
    destroyKeyAndValueHashTable(permissions_d);

    se->digs_free_string_array(&list, &listLength);
}


#if 0
/***********************************************************************
*   static void handleMD5SumAndSizeCallback(char *key, char *value, void *param)
*    
*   Callback for iterating over hash table. Called for every lfn in the RLS 
*   entry without a corresponding checksum.
*    
*   Parameters:                                       [I/O]
*
*     key       the logical name of the file           I
*     value     the value of the checksum
*     param     structure containing hostname and
*               interactive flag                       I
*    
*   Returns: (void)
***********************************************************************/
static void handleMD5SumAndSizeCallback(char *key, char *value, void *param)
{
  char *tmpNode;
  char *tmpFilename;
  char *tmpValue;
  long long calcSize;
  char *sizeString;
  qcdgrid_hash_table_t *size_d;

  size_d = (qcdgrid_hash_table_t *)param;  

  globus_libc_printf( "trying to register md5sum lfn = %s;", key);
  globus_libc_printf(" md5sum = %s\n", value);
      
  if(! registerAttrWithRc(key, "md5sum", value))
  {
    globus_libc_fprintf(stderr, "Error registering with RLS\n");
  }
  tmpValue = lookupValueInHashTable(size_d, key);

  if(tmpValue == NULL)
  {
    globus_libc_printf("size was missing too for LFN = %s\n", key);
    tmpNode = getFirstFileLocation(key);
    tmpFilename = constructFilename(tmpNode, key);
    calcSize = getRemoteFileLengthFullPath(tmpNode, tmpFilename);

    globus_libc_free(tmpFilename);
    globus_libc_free(tmpNode);

    globus_libc_printf("obtained size = %d\n", (int)calcSize);
    
    if (safe_asprintf(&sizeString, "%d", calcSize) < 0)
    {
      globus_libc_printf("Out of memory in rebuild-qcdgrid-rc");
    }

    globus_libc_printf("trying to register size lfn = %s; size = %s\n", key, sizeString);
    if(!registerAttrWithRc(key, "size", sizeString))
    {
      globus_libc_fprintf(stderr, "Error registering with RLS\n");
    }
    globus_libc_free(sizeString);
  }
  else
  {
    globus_libc_free(tmpValue);
  }

}
#endif

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of QCDgrid replica catalogue rebuilding command
*
*   Parameters:                                                  [I/O]
*
*     Optional hostname to rebuild catalogue for one host only    I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    int nn;
    int i;
    char *host;

    int md5sum = 0;
    int size = 0;

//    qcdgrid_hash_table_t *dict;
//    qcdgrid_hash_table_t *size_d;

//    dict = newKeyAndValueHashTable();

    if (argc > 4)
    {
	globus_libc_printf("Usage: digs-rebuild-rc [--no-md5sum] [--no-size] [<hostname>]\n");
	return 0;
    }

    if (!qcdgridInit(0))
    {
	return 1;
    }

//skips 1 argument
    for(i=1; i<argc; i++)
    {
	if(!strcmp(argv[i], "--no-md5sum"))
	{
	    md5sum = 1;
	}
	if(!strcmp(argv[i], "--no-size"))
	{
	    size = 1;
	}
    }

    int isHostSpecified = argc - 1 - size - md5sum;
    //argc - 1 - size - md5sum == 0  -> no hostname
    //                         == 1 -> argv[argc-1] hostname

// there is a host specified
    if (isHostSpecified)
    {
	host = argv[argc-1];

	if (nodeIndexFromName(host) < 0)
	{
	    globus_libc_fprintf(stderr, "%s is not a grid node\n", host);
	    return 1;
	}
	
//RADEK check md5sum and size as default
// change from 1 to 0 and back:  1-0=1 1-1=0 
	rebuildLocationCatalogue(host, 1-size, 1-md5sum);
//	destroyKeyAndValueHashTable(dict);
    }
else
//there is no host specified
    {

	/* Loop over every node in the grid */
	nn = getNumNodes();
	for (i = 0; i < nn; i++)
	{
	    host = getNodeName(i);
	    
                            //RADEK check md5sum and size as default
	    rebuildLocationCatalogue(host, 1-size, 1-md5sum);
	}

	/* 
	 *  Deal with dict (lfn, checksum) for misssing attributes
	 *  for each element 'lfn' key in dict
	 *  register checksum in RLS and check if size attribute exists
	 */
	
// dict cannot be NULL as it has entries
//	if(dict != NULL)
//	{
	    
//	    globus_libc_printf("There are missing checksums/sizes\n");
	    
//	    size_d = getAllAttributesValues("size");    
	    // replaces the iterator with a function which is called 
	    // from forEachHashTableKeyAndValue
	    
//	    forEachHashTableKeyAndValue(dict, handleMD5SumAndSizeCallback, (void*)&size_d);
	    
//	    destroyKeyAndValueHashTable(dict);
//	    destroyKeyAndValueHashTable(size_d);
//	}       

    }

    return 0;
}
