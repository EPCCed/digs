/***********************************************************************
*
*   Filename:   replica.c
*
*   Authors:    James Perry, Radoslaw Ostrowski  (jamesp, radek)   EPCC.
*
*   Purpose:    Interfaces with the Globus replica catalogue
*
*   Contents:   Wrapper functions around Globus RC functionality
*
*   Used in:    Called by other modules to access the replica catalogue
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

#include <globus_rls_client.h>
#include <globus_list.h>

#include "config.h"
#include "misc.h"
#include "node.h"
#include "replica.h"
#include "hashtable.h"
#include "background-new.h"

/*
 * Some of the function names in here are not very consistent with
 * RLS terminology; this is because they exactly mirror functions in
 * the old Globus replica catalogue based implementation (see
 * replica.c)
 */

/*
 * Handle to the replica location service
 */
static globus_rls_handle_t *rlsHandle_;

/* whether RLS is currently opened or not */
static int rlsOpened_ = 0;

/***********************************************************************
*   void closeReplicaCatalogue()
*    
*   Closes the Globus replica location service handle.
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
void closeReplicaCatalogue()
{
    logMessage(1, "closeReplicaCatalogue()");

    if (rlsOpened_)
    {
	globus_rls_client_close(rlsHandle_);
	rlsOpened_ = 0;
    }
}

/***********************************************************************
*   int openReplicaCatalogue(char *hostname)
*    
*   Opens the Globus replica location service. Called at startup, and
*   the same handle is used to access the replica catalogue from then on
*    
*   Parameters:                                                    [I/O]
*
*     hostname  FQDN of the machine hosting the RLS                 I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int openReplicaCatalogue(char *hostname)
{
    globus_result_t result;
    char *buffer;
    unsigned short port;

    logMessage(1, "openReplicaCatalogue(%s)", hostname);

    if (rlsOpened_)
    {
	/* it's already open */
	return 1;
    }
    
    /*
     * The same config file entry is used for the RLS port number as
     * for the old replica catalogue port number
     */
    port = getConfigIntValue("miscconf", "rc_port", 39281);

    /*
     * construct RLS URL
     */
    if (safe_asprintf(&buffer, "rls://%s:%d/", hostname, port) < 0) 
    {
	errorExit("Out of memory in openReplicaCatalogue");
    }

    result = globus_rls_client_connect(buffer, &rlsHandle_);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(5, "Cannot open RLS connection to %s", buffer);
	globus_libc_free(buffer);
	return 0;
    }

    globus_libc_free(buffer);
    rlsOpened_ = 1;
    return 1;
}

/***********************************************************************
*   int reopenReplicaCatalogue()
*    
*   Attempts to reopen the replica location service handle, in case of
*   access failure
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int reopenReplicaCatalogue()
{
    logMessage(1, "reopenReplicaCatalogue()");
    closeReplicaCatalogue();
    return openReplicaCatalogue(getMainNodeName());
}

/***********************************************************************
*   unsigned int rlsHashString(char *str)
*    
*   Hashes a string to produce a 32-bit hash code
*    
*   Parameters:                                                    [I/O]
*
*     str   points to the string to hash                            I
*    
*   Returns: a 32-bit hash code
***********************************************************************/
static unsigned int rlsHashString(char *str)
{
    int i;
    unsigned int hash = 0;

    i = 0;
    while (str[i])
    {
	hash ^= str[i];
	hash = (hash << 3) | (hash >> 29);
	i++;
    }
    return hash;
}

/***********************************************************************
*   logicalFileInfo_t *getFileList(char *wildcard, int *numfiles)
*    
*   Obtains a list of all the files in the catalogue matching the
*   specified wildcard string
*    
*   Parameters:                                                    [I/O]
*
*     wildcard  wildcard to use in query                            I
*     numfiles  receives number of files in list                      O
*    
*   Returns: pointer to array of file structures, NULL on error
***********************************************************************/
/* must be a power of two */
#define LFN_HASH_SIZE 4096

logicalFileInfo_t *getFileList(char *wildcard, int *numfiles)
{
    logicalFileInfo_t *list;
    int *hashTable;
    int lfi;

    int listAlloced, listUsed;
    globus_result_t res;
    int offset = 0;
    globus_list_t *glist, *p;
    globus_rls_string2_t *str2;
    int i;
    int hc;

    logMessage(1, "getFileList(%s)", wildcard);

    /* allocate space for 1000 files in list initially */
    *numfiles = 0;
    listAlloced = 1000;
    listUsed = 0;
    list = globus_libc_malloc(1000 * sizeof(logicalFileInfo_t));
    if (!list)
    {
	errorExit("Out of memory in getFileList");
    }
    hashTable = globus_libc_malloc(LFN_HASH_SIZE * sizeof(int));
    if (!hashTable)
    {
	errorExit("Out of memory in getFileList");
    }
    for (i = 0; i < LFN_HASH_SIZE; i++)
    {
	hashTable[i] = -1;
    }

    /* get the file list from Globus */
    res = globus_rls_client_lrc_get_pfn_wc(rlsHandle_, wildcard,
					   rls_pattern_unix, &offset,
					   0, &glist);
    if (res != GLOBUS_SUCCESS)
    {
	int rc;

	globus_libc_free(list);
	globus_libc_free(hashTable);

	res = globus_rls_client_error_info(res, &rc, NULL, 0, GLOBUS_TRUE);
	if (rc == GLOBUS_RLS_LFN_NEXIST)
	{
	    logMessage(5, "%s: no matching files found", wildcard);
	}
	else
	{
	    printError(res, "globus_rls_client_lrc_get_pfn_wc");
	}
	return NULL;
    }

    /* process list from Globus */
    p = glist;
    while (p)
    {
	/* get string mapping (s1 = lfn, s2 = pfn) */
	str2=(globus_rls_string2_t *) globus_list_first(p);

	/* hash the lfn */
	hc = rlsHashString(str2->s1);
	
	/* look for lfn in current list */
	lfi = hashTable[hc & (LFN_HASH_SIZE - 1)];
	while (lfi >= 0)
	{
	    if (!strcmp(list[lfi].lfn, str2->s1))
	    {
		if (list[lfi].numPfns < MAX_PFNS)
		{
		    list[lfi].pfns[list[lfi].numPfns] = nodeIndexFromName(str2->s2);
		    list[lfi].numPfns++;
		}
		break;
	    }
	    lfi = list[lfi].next;
	}

	if (lfi < 0)
	{
	    /* didn't find it - need to add new lfn to list */
	    if (listUsed >= listAlloced)
	    {
		/* allocate more entries if necessary */
		listAlloced *= 2;
		list = globus_libc_realloc(list, listAlloced * sizeof(logicalFileInfo_t));
		if (!list)
		{
		    errorExit("Out of memory in getFileList");
		}
	    }
	    list[listUsed].lfn = safe_strdup(str2->s1);
	    if (!list[listUsed].lfn)
	    {
		errorExit("Out of memory in getFileList");
	    }
	    list[listUsed].hash = hc;
	    list[listUsed].numPfns = 1;
	    list[listUsed].pfns[0] = nodeIndexFromName(str2->s2);

	    list[listUsed].next = hashTable[hc & (LFN_HASH_SIZE - 1)];
	    hashTable[hc & (LFN_HASH_SIZE - 1)] = listUsed;

	    listUsed++;
	}

	/* next list item */
	p=globus_list_rest(p);
    }

    globus_rls_client_free_list(glist);
    globus_libc_free(hashTable);

    *numfiles = listUsed;
    return list;
}

/***********************************************************************
*   void freeFileList(logicalFileInfo_t *list, int numfiles)
*    
*   Frees a list of file info structures
*    
*   Parameters:                                                    [I/O]
*
*     list      array to free                                       I
*     numfiles  number of entries in array                          I
*    
*   Returns: (void)
***********************************************************************/
void freeFileList(logicalFileInfo_t *list, int numfiles)
{
    int i;

    logMessage(1, "freeFileList()");

    for (i = 0; i < numfiles; i++)
    {
	globus_libc_free(list[i].lfn);
    }
    globus_libc_free(list);
}

/***********************************************************************
*   int deleteEntireLocation(char *location)
*    
*   Removes a whole location and all its replica information from the 
*   catalogue. Not to be taken lightly - only call when node is being 
*   permanently removed from the grid
*    
*   Parameters:                                                    [I/O]
*
*     location  FQDN of the machine being removed                   I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int deleteEntireLocation(char *location)
{
    globus_list_t *filesAtLocation;
    globus_list_t *failureList;
    globus_result_t result;

    logMessage(1, "deleteEntireLocation(%s)", location);

    /*
     * Get a list of all the files at that location
     */
    result = globus_rls_client_lrc_get_lfn(rlsHandle_, location, NULL,
					   0, &filesAtLocation);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(3, "Cannot get list of files at %s", location);
	return 0;
    }

    if (filesAtLocation)
    {
	/*
	 * Now delete them all
	 */
	result = globus_rls_client_lrc_delete_bulk(rlsHandle_,
					       filesAtLocation,
					       &failureList);
	if (result != GLOBUS_SUCCESS)
	{
	    logMessage(3, "Bulk delete of location %s failed",
		       location);
	    globus_rls_client_free_list(filesAtLocation);
	    return 0;
	}

	globus_rls_client_free_list(filesAtLocation);
	if (failureList)
	{
	    globus_rls_client_free_list(failureList);
	}
    }
    return 1;
}

/***********************************************************************
*   int fileInCollection(char *lfn)
*    
*   Checks if file is already registered in the catalogue
*    
*   Parameters:                                                    [I/O]
*
*     lfn  Logical grid filename of file to check                   I
*    
*   Returns: 1 if file is in collection, 0 if not
***********************************************************************/
int fileInCollection(char *lfn)
{
    globus_list_t *locations;
    globus_result_t result;

    logMessage(1, "fileInCollection(%s)", lfn);

    result = globus_rls_client_lrc_get_pfn(rlsHandle_, lfn, NULL, 0,
					   &locations);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(3, "globus_rls_client_lrc_get_pfn failed in "
		   "fileInCollection(%s)", lfn);
	return 0;
    }

    if (!locations)
    {
	return 0;
    }

    globus_rls_client_free_list(locations);

    return 1;
}

/***********************************************************************
*   int fileAtLocation(char *node, char *lfn)
*    
*   Checks if file already exists at a location
*    
*   Parameters:                                                    [I/O]
*
*     lfn  Logical grid filename of file to check                   I
*    
*   Returns: 1 if file is in location, 0 if not
***********************************************************************/
int fileAtLocation(char *node, char *lfn)
{
    if (globus_rls_client_lrc_mapping_exists(rlsHandle_, lfn, node)
	== GLOBUS_SUCCESS)
    {
	return 1;
    }

    return 0;
}

/***********************************************************************
*   void freeLocationFileList(char **list)
*    
*   Frees the file list returned by listLocationFiles
*    
*   Parameters:                                                    [I/O]
*
*     list  Pointer to the list to free                             I
*    
*   Returns: (void)
***********************************************************************/
void freeLocationFileList(char **list)
{
    int i;

    if (list)
    {
	i = 0;
	while (list[i])
	{
	    globus_libc_free(list[i]);
	    i++;
	}
	globus_libc_free(list);
    }
}

/***********************************************************************
*   void destroyFileDisksList(qcdgrid_hash_table_t **hts)
*    
*   Frees the file disks list returned by getAllFileDisks
*    
*   Parameters:                                                    [I/O]
*
*     hts  Pointer to the list to free                              I
*    
*   Returns: (void)
***********************************************************************/
void destroyFileDisksList(qcdgrid_hash_table_t **hts)
{
    int i;

    i = 0;
    while (hts[i])
    {
	destroyHashTable(hts[i]);
	i++;
    }
    globus_libc_free(hts);
}

/***********************************************************************
*   qcdgrid_hash_table_t **getAllFileDisks(char *location)
*    
*   This function obtains the disk attributes from all files on a node
*   with one single RLS query. It returns an array of pointers to hash
*   tables (terminated by a NULL). Each hash table contains all the
*   filenames that reside on one disk (i.e. the first hash table
*   contains all the files in 'data', the second all the files in
*   'data1', the third all the files in 'data2', etc.)
*    
*   Parameters:                                                    [I/O]
*
*     location  FQDN of location to query                           I
*    
*   Returns: Pointer to array of hash tables as described above, or NULL
*            on error
***********************************************************************/
qcdgrid_hash_table_t **getAllFileDisks(char *location)
{
    qcdgrid_hash_table_t **hts;
    int numDisks;
    int numHtsAlloced;
    int i;
    int dn;

    int numFiles;

    globus_result_t result;
    globus_list_t *list, *p;
    globus_rls_attribute_object_t *attrObj;
    char *attrName;

    logMessage(1, "getAllFileDisks(%s)", location);

    /* allocate space for 8 disk hash tables to begin with */
    numHtsAlloced = 8;
    hts = globus_libc_malloc(8 * sizeof(qcdgrid_hash_table_t*));

    if (hts == NULL)
    {
	errorExit("Out of memory in getAllFileDisks");
    }

    for (i = 0; i < numHtsAlloced; i++)
    {
	hts[i] = NULL;
    }

    /* we always have at least one disk: 'data' */
    numDisks = 1;
    hts[0] = newHashTable();

    if (safe_asprintf(&attrName, "%s-dir", location) < 0)
    {
	errorExit("Out of memory in getAllFileDisks");
    }

    /* Query the catalogue for all attributes */
    result = globus_rls_client_lrc_attr_search(rlsHandle_, attrName,
					       globus_rls_obj_lrc_lfn,
					       globus_rls_attr_op_all,
					       NULL, NULL, NULL, 0,
					       &list);
    globus_libc_free(attrName);

    if (result == GLOBUS_SUCCESS)
    {
	numFiles = 0;
	
	p = list;
	while (p)
	{
	    attrObj = (globus_rls_attribute_object_t *)globus_list_first(p);
	    p = globus_list_rest(p);

	    /* get disk number for this attribute in dn */
	    dn = attrObj->attr.val.s[4];
	    if ((dn < '1') && (dn != 0))
	    {
		logMessage(3, "Unexpected attribute value %s", attrObj->attr.val.s);
		destroyFileDisksList(hts);
		globus_rls_client_free_list(list);
		return NULL;
	    }

	    if (dn >= '1')
	    {
		dn -= '0';
	    }

	    /* now see if we need to create a new hash table */
	    if (dn >= (numDisks - 1))
	    {
		while (dn >= (numHtsAlloced - 1))
		{
		    hts = globus_libc_realloc(hts, (numHtsAlloced+8)*
					      sizeof(qcdgrid_hash_table_t*));
		    if (!hts)
		    {
			errorExit("Out of memory in getAllFileDisks");
		    }
		    for (i = numHtsAlloced; i < (numHtsAlloced+8); i++)
		    {
			hts[i] = NULL;
		    }
		    numHtsAlloced += 8;
		}

		for (i = numDisks; i < (dn+1); i++)
		{
		    hts[i] = newHashTable();
		}
		numDisks = dn + 1;
	    }

	    /* add to hash table */
	    addToHashTable(hts[dn], attrObj->key);
	    numFiles ++;
	}
	globus_rls_client_free_list(list);
    }

    /*
     * Remember that if there's only one disk, not all files are guaranteed to
     * have a disk attribute. Some of them might (in which case we may have a
     * partial list from the code above) or maybe none of them do (in which case
     * the attribute search failed). In either case, build a new list here.
     */
    if ((numDisks == 1) || (result != GLOBUS_SUCCESS))
    {
	char **fileList;

	logMessage(1, "getAllFileDisks: single disk version");

	logMessage(1, "before destroying HT");
	/* destroy any partial list and start again */
	destroyHashTable(hts[0]);
	logMessage(1, "after destroying HT");

	hts[0] = newHashTable();
	logMessage(1, "after creating HT");


	/* list all the files stored here into the hash table */
	fileList = listLocationFiles(location);
	if (fileList != NULL)
	{
	    i = 0;
	    while (fileList[i])
	    {
		addToHashTable(hts[0], fileList[i]);
		i++;
	    }

	    freeLocationFileList(fileList);
	}
    }

    return hts;
}				  

/***********************************************************************
*   char **listLocationFiles(char *location)
*    
*   Returns a list of all the files at the given location. Used to
*   verify that the replica catalogue matches the actual files on a
*   node's disk
*    
*   Parameters:                                                    [I/O]
*
*     location  FQDN of location to list                            I
*    
*   Returns: Pointer to a list of filenames registered at the location,
*            or NULL on error
***********************************************************************/
char **listLocationFiles(char *location)
{
    globus_list_t *files;
    globus_list_t *p;
    globus_result_t result;
    int i;
    int count;
    char **list;
    globus_rls_string2_t *str2;

    logMessage(1, "listLocationFiles(%s)", location);

    /*
     * List the files
     */
    result = globus_rls_client_lrc_get_lfn(rlsHandle_, location, NULL,
					   0, &files);
    if (result!=GLOBUS_SUCCESS)
    {
	logMessage(3, "get lfn failed in listLocationFiles(%s)", location);
	return NULL;
    }

    /*
     * Count the number of files
     */
    count = 0;
    p = files;
    while (p)
    {
	p = globus_list_rest(p);
	count++;
    }
    
    /*
     * Create a simple char** list
     */
    list = globus_libc_malloc((count+1)*sizeof(char*));
    if (!list)
    {
	errorExit("Out of memory in listLocationFiles");
    }

    p = files;
    for (i = 0; i < count; i++)
    {
	str2 = (globus_rls_string2_t *) globus_list_first(p);
	list[i] = safe_strdup(str2->s1);
	if (!list[i])
	{
	    errorExit("Out of memory in listLocationFiles");
	}
	p = globus_list_rest(p);
    }
    list[i] = NULL;

    globus_rls_client_free_list(files);

    return list;
}

/***********************************************************************
*   int removeFileFromCollection(char *lfn)
*    
*   Removes the logical filename from the collection, for when all copies
*   of the file are deleted
*    
*   Parameters:                                                    [I/O]
*
*     lfn  Logical grid filename to remove                          I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int removeFileFromCollection(char *lfn)
{
    /*
     * It's unnecessary to do this in the replica location
     * service as unlike the old replica catalogue, it has
     * no concept of a collection of files. A file only
     * exists if it's associated with a location
     */
    logMessage(1, "removeFileFromCollection(%s)", lfn);
    return 1;
}

/***********************************************************************
*   int removeFileFromLocation(char *node, char *lfn)
*    
*   Removes a logical filename from a location entry in the catalogue
*    
*   Parameters:                                                    [I/O]
*
*     node FQDN of machine to remove file from                      I
*     lfn  Logical grid filename to remove                          I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int removeFileFromLocation(char *node, char *lfn)
{

    globus_result_t result;

    logMessage(1, "removeFileFromLocation(%s,%s)", node, lfn);

    result = globus_rls_client_lrc_delete(rlsHandle_, lfn, node);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(3, "globus_rls_client_lrc_delete failed in "
		   "removeFileFromLocation(%s,%s)", node, lfn);
	return 0;
    }

    return 1;
}

/***********************************************************************
*   int registerAttrWithRc(char *lfn, char *key, char *value)
*    
*   Registers an attribute to a particular lfn
*    
*   Parameters:                                                    [I/O]
*
*     value of the attribute                                         I
*     key   name of the attribute                                    I
*     lfn   Logical grid filename to add                             I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int registerAttrWithRc(char *lfn, char *key, char *value)
{
    globus_result_t result;
    globus_rls_attribute_t attr;

    logMessage(1, "registerAttrWithRc lfn=%s key=%s value=%s", lfn, key, value);

    /*
     * First create the attribute. There seems no way to tell whether
     * it already exists, so just create it regardless. If it already
     * exists this will fail obviously, but it shouldn't matter
     */
    result = globus_rls_client_lrc_attr_create(rlsHandle_, key,
					       globus_rls_obj_lrc_lfn,
					       globus_rls_attr_type_str);

    /*
     * Fill in the attribute's values
     */
    attr.name = key;
    attr.objtype = globus_rls_obj_lrc_lfn;
    attr.type = globus_rls_attr_type_str;

    /*
     * Try to remove attribute if it already exists
     */

// RADEK it shouldn't be removed, if it's there it should probably stay there
// unless it is permissions attribute 
// (or incerect, then remove by hand)

    /*if (strcmp(key, "permissions") == 0)
      {*/
	result = globus_rls_client_lrc_attr_remove(rlsHandle_, lfn, &attr);
	/*}*/

    attr.val.s = value;

    /*
     * Now try to set it for the particular file specified
     */
    result = globus_rls_client_lrc_attr_add(rlsHandle_, lfn, &attr);

    if (result != GLOBUS_SUCCESS)
    {
      logMessage(5, "ERROR setting the attribute: registerAttrWithRc:\nkey=%s,value=%s", key, value);
      return 0;
    }
    return 1;
}

/***********************************************************************
*   int removeLfnAttribute(char *lfn, char *key)
*    
*   Removes an attribute from a particular lfn
*    
*   Parameters:                                                    [I/O]
*
*     lfn   Logical grid filename                                    I
*     key   name of the attribute                                    I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int removeLfnAttribute(char *lfn, char *key)
{
    globus_result_t result;
    globus_rls_attribute_t attr;
  
    attr.name = key;
    attr.objtype = globus_rls_obj_lrc_lfn;
    attr.type = globus_rls_attr_type_str;

    result = globus_rls_client_lrc_attr_remove(rlsHandle_, lfn, &attr);
    if (result != GLOBUS_SUCCESS)
    {
        logMessage(5, "Error removing attribute %s for lfn %s", key, lfn);
        return 0;
    }
    return 1;
}

/***********************************************************************
*   int registerFileWithRc(char *node, char *lfn)
*    
*   Top level function for registering a new copy of a file
*    
*   Parameters:                                                    [I/O]
*
*     node FQDN of machine to add file to                           I
*     lfn  Logical grid filename to add                             I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int registerFileWithRc(char *node, char *lfn)
{
    globus_result_t result;
    globus_list_t *list;
    int rc;

    logMessage(1, "registerFileWithRc(%s,%s)", node, lfn);

    /*
     * Complication here: different function needs to be called if
     * lfn already exists
     */
    result = globus_rls_client_lrc_get_pfn(rlsHandle_, lfn, NULL, 0,
					   &list);
    if ((result == GLOBUS_SUCCESS) && (list != NULL))
    {
	globus_rls_client_free_list(list);
	result = globus_rls_client_lrc_add(rlsHandle_, lfn, node);

	if (result != GLOBUS_SUCCESS)
	{
	    printError(result, "globus_rls_client_lrc_add");
	    logMessage(3, "File: %s on %s", lfn, node);
	    return 0;
	}
    }
    else
    {
	result = globus_rls_client_lrc_create(rlsHandle_, lfn, node);
	if (result != GLOBUS_SUCCESS)
	{
	    /*
	     * It's possible for the lfn to still exist but with no pfns
	     * registered (for example, a file used to exist but has been
	     * deleted and then re-uploaded), in which case we come in here
	     * instead of to the 'add' branch, but Globus complains when we
	     * try to "create" an already existing lfn. So try to trap this
	     * case and "add" here
	     */
	    result = globus_rls_client_error_info(result, &rc, NULL, 0, GLOBUS_TRUE);
	    if (rc == GLOBUS_RLS_LFN_EXIST)
	    {
		result = globus_rls_client_lrc_add(rlsHandle_, lfn, node);
		if (result != GLOBUS_SUCCESS)
		{
		    printError(result, "globus_rls_client_lrc_add");
		    logMessage(3, "File: %s on %s", lfn, node);
		    return 0;
		}
	    }
	    else
	    {
		printError(result, "globus_rls_client_lrc_create");
		logMessage(3, "File: %s on %s", lfn, node);
		return 0;
	    }
	}
    }
    return 1;
}

/*
 * Using global vars here isn't thread safe. Fixing this would require
 * the caller keeping hold of some sort of context for the iteration.
 * Maybe not worth doing.
 */
static globus_list_t *fileLocationsList_ = NULL;
static globus_list_t *fileLocationsPosition_ = NULL;
static int returnAllLocations_ = 0;

/***********************************************************************
*   char *getNextFileLocation()
*    
*   Returns the next location of a file or NULL if they've been exhausted
*   Warning, not thread safe
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: Pointer to the next location of the file (storage is
*            dynamically allocated and should be freed by the caller)
*            NULL if the locations have been exhausted
***********************************************************************/
char *getNextFileLocation()
{
    char *loc = NULL;
    globus_rls_string2_t *str2;

    do
    {
	if (loc != NULL)
	{
	    globus_libc_free(loc);
	}

	if (fileLocationsPosition_ == NULL)
	{
	    if (fileLocationsList_ != NULL)
	    {
		globus_rls_client_free_list(fileLocationsList_);
		fileLocationsList_ = NULL;
	    }
	    return NULL;
	}

	str2 = (globus_rls_string2_t *)
	    globus_list_first(fileLocationsPosition_);
	loc = safe_strdup(str2->s2);
	if (!loc)
	{
	    errorExit("Out of memory in getNextFileLocation");
	}

	fileLocationsPosition_ = globus_list_rest(fileLocationsPosition_);
    } while (((isNodeDead(loc)) || (isNodeDisabled(loc))) &&
	     (!returnAllLocations_));

    return loc;
}

/***********************************************************************
*   char *getFirstFileLocation(char *lfn)
*    
*   Reads all the locations containing a certain file from the RC and 
*   returns the first one. Skips dead and disabled nodes
*    
*   Parameters:                                                    [I/O]
*
*     lfn  The logical grid filename of the file to find            I
*    
*   Returns: Pointer to the first location of the file (storage is
*            dynamically allocated and should be freed by the caller)
*            NULL if there are no locations
***********************************************************************/
char *getFirstFileLocation(char *lfn)
{
    globus_result_t result;

    if (fileLocationsList_ != NULL)
    {
	globus_rls_client_free_list(fileLocationsList_);
	fileLocationsList_ = NULL;
    }
    fileLocationsPosition_ = NULL;

    result = globus_rls_client_lrc_get_pfn(rlsHandle_, lfn, NULL, 0,
					   &fileLocationsList_);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(3, "globus_rls_client_lrc_get_pfn failed in "
		   "getFirstFileLocation(%s)", lfn);
	return NULL;
    }

    fileLocationsPosition_ = fileLocationsList_;

    returnAllLocations_ = 0;

    return getNextFileLocation();
}

/***********************************************************************
*   char *getFirstFileLocationAll(char *lfn)
*    
*   Reads all the locations containing a certain file from the RC and 
*   returns the first one. Does not skip dead and disabled nodes
*    
*   Parameters:                                                    [I/O]
*
*     lfn  The logical grid filename of the file to find            I
*    
*   Returns: Pointer to the first location of the file (storage is
*            dynamically allocated and should be freed by the caller)
*            NULL if there are no locations
***********************************************************************/
char *getFirstFileLocationAll(char *lfn)
{
    globus_result_t result;

    if (fileLocationsList_ != NULL)
    {
	globus_rls_client_free_list(fileLocationsList_);
	fileLocationsList_ = NULL;
    }
    fileLocationsPosition_ = NULL;

    result = globus_rls_client_lrc_get_pfn(rlsHandle_, lfn, NULL, 0,
					   &fileLocationsList_);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(3, "globus_rls_client_lrc_get_pfn failed in "
		   "getFirstFileLocation(%s)", lfn);
	return NULL;
    }

    fileLocationsPosition_ = fileLocationsList_;

    returnAllLocations_ = 1;

    return getNextFileLocation();
}

/***********************************************************************
*   int getNumCopies(char *lfn, int flags)
*    
*   Counts how many copies of the file are registered in the catalogue
*    
*   Parameters:                                                    [I/O]
*
*     lfn    The logical grid filename of the file to find          I
*     flags  flags describing which copies to count                 I
*    
*   Returns: Number of copies of the file on the grid
***********************************************************************/
int getNumCopies(char *lfn, int flags)
{
    globus_list_t *list;
    globus_list_t *p;
    globus_result_t result;
    int count;
    globus_rls_string2_t *str2;

    logMessage(1, "getNumCopies(%s,%d)", lfn, flags);

    result = globus_rls_client_lrc_get_pfn(rlsHandle_, lfn, NULL, 0,
					   &list);
    if (result != GLOBUS_SUCCESS)
    {
	/* if call failed, try reopening catalogue handle */
	logMessage(3, "getNumCopies failed, reopening catalogue...\n");
	if (!reopenReplicaCatalogue())
	{
	    logMessage(3, "Reopening catalogue failed\n");
	    printError(result, "globus_rls_client_lrc_get_pfn");
	    return 0;
	}

	/* now try call again */
	result = globus_rls_client_lrc_get_pfn(rlsHandle_, lfn, NULL, 0,
					       &list);
	if (result != GLOBUS_SUCCESS)
	{
	    printError(result, "globus_rls_client_lrc_get_pfn");
	    return 0;
	}
    }

    count = 0;
    p = list;
    while (p != NULL)
    {
	str2 = (globus_rls_string2_t *) globus_list_first(p);
	
	if (!isNodeDead(str2->s2))
	{
	    if ((flags & GNC_COUNTRETIRING) || (!isNodeRetiring(str2->s2)))
	    {
		count++;
	    }
	}
	p = globus_list_rest(p);
    }

    globus_rls_client_free_list(list);

    return count;
}

/*
 * These variables hold the list of files used by getFirstFile and
 * getNextFile. This is not thread safe; we assume the replica
 * catalogue will only be accessed by one thread of a process.
 */
static logicalFileInfo_t *fileList_ = NULL;
static int fileListLength_ = 0;
static int fileListPos_ = 0;

/***********************************************************************
*   char *getNextFile()
*    
*   Returns the next file or NULL if they've been exhausted
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: Pointer to the next file (storage is dynamically allocated 
*            and should be freed by the caller)
*            NULL if the files have been exhausted
***********************************************************************/
char *getNextFile()
{
    if (fileListPos_ >= fileListLength_)
    {
	return NULL;
    }
    fileListPos_++;
    return safe_strdup(fileList_[fileListPos_ - 1].lfn);
}

/***********************************************************************
*   char *getFirstFile()
*    
*   Reads all the files from the replica catalogue and returns the first
*   one
*    
*   Parameters:                                                    [I/O]
*
*     None
*    
*   Returns: Pointer to the first file (storage is dynamically allocated 
*            and should be freed by the caller)
*            NULL if there are no files
***********************************************************************/
char *getFirstFile()
{  
    if (fileList_)
    {
	freeFileList(fileList_, fileListLength_);
    }
    fileList_ = getFileList("*", &fileListLength_);
    fileListPos_ = 0;
    
    return getNextFile();
}

/* Need access to the preference order list in node.c, in order to decide
 * which copy is best */
extern nodeList_t *prefList_;

/* Where to start looking in the preference order for locations (basically
 * wherever we left off looking last time) */
static int bestStart_ = 0;

/***********************************************************************
*   char *getNextBestCopyLocation(char *lfn)
*    
*   Gets the next best copy location of the file (this is called when
*   the location suggested by getBestCopyLocation fails to work)
*    
*   Parameters:                                                    [I/O]
*
*     lfn  The logical grid filename of the file to find            I
*    
*   Returns: Pointer to the chosen location of the file (storage should
*            NOT be freed by the caller this time)
*            NULL if the locations have been exhausted
***********************************************************************/
char *getNextBestCopyLocation(char *lfn)
{
    char *loc;
    int bestPosSoFar;
    int bestHost;
    int host;
    int i;

    loc = getFirstFileLocation(lfn);

    bestPosSoFar = 1000000;
    bestHost = -1;

    while (loc) 
    {
	host = nodeIndexFromName(loc);
    
	i = bestStart_;

	while (i < prefList_->count)
	{
	    if ((prefList_->nodes[i] == host) && (i < bestPosSoFar))
	    {
		bestHost = prefList_->nodes[i];
		bestPosSoFar = i;
	    }
	    i++;
	}

	globus_libc_free(loc);
	loc = getNextFileLocation();
    }

    if (bestHost < 0) return NULL;

    bestStart_ = bestPosSoFar + 1;
    
    return getNodeName(bestHost);
}

/***********************************************************************
*   char *getBestCopyLocation(char *lfn)
*    
*   Reads all the locations containing a certain file from the RC and 
*   returns the best one according to the node preference order
*    
*   Parameters:                                                    [I/O]
*
*     lfn  The logical grid filename of the file to find            I
*    
*   Returns: Pointer to the chosen location of the file (storage should
*            NOT be freed by the caller this time)
*            NULL if there are no locations
***********************************************************************/
char *getBestCopyLocation(char *lfn)
{
    bestStart_ = 0;

    return getNextBestCopyLocation(lfn);
}

/***********************************************************************
*   char *getDiskInfo(char *host, char *lfn)
*    
*   Returns the disk (storage directory name in fact) where the file
*   specified is stored on the host
*    
*   Parameters:                                                    [I/O]
*
*     host The FQDN of the host storing the file                    I
*     lfn  The logical grid filename of the file                    I
*    
*   Returns: Pointer to storage directory name. Should be freed by the
*            caller
***********************************************************************/
char *getDiskInfo(char *host, char *lfn)
{
    globus_result_t result;
    char *attrName;
    globus_list_t *list;
    globus_rls_attribute_t *attr;
    char *res;

    logMessage(1, "getDiskInfo(%s,%s)", host, lfn);

    /*
     * Attribute name is '<host>-dir',
     * e.g. 'edqcdgrid.epcc.ed.ac.uk-dir'
     */
    if (safe_asprintf(&attrName, "%s-dir", host) < 0)
    {
	errorExit("Out of memory in getDiskInfo");
    }

    /*
     * Actually query
     */
    result = globus_rls_client_lrc_attr_value_get(rlsHandle_, lfn,
						  attrName,
						  globus_rls_obj_lrc_lfn,
						  &list);
    globus_libc_free(attrName);
    if ((result != GLOBUS_SUCCESS) || (list == NULL))
    {
	return safe_strdup("data");
    }

    /*
     * This should return one value in the list
     */
    attr = (globus_rls_attribute_t *) globus_list_first(list);

    if (attr == NULL)
    {
	globus_rls_client_free_list(list);
	return safe_strdup("data");
    }

    res = safe_strdup(attr->val.s);

    globus_rls_client_free_list(list);
    return res;
}

/***********************************************************************
*   int setDiskInfo(char *host, char *lfn, char *disk)
*    
*   Sets which disk the file is stored on
*    
*   Parameters:                                                    [I/O]
*
*     host The FQDN of the host storing the file                    I
*     lfn  The logical grid filename of the file                    I
*     disk The name of the storage directory                        I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int setDiskInfo(char *host, char *lfn, char *disk)
{
    globus_result_t result;
    char *attrName;
    globus_rls_attribute_t attr;

    logMessage(1, "setDiskInfo(%s,%s,%s)", host, lfn, disk);

    /*
     * Attribute name is '<host>-dir',
     * e.g. 'edqcdgrid.epcc.ed.ac.uk-dir'
     */
    if (safe_asprintf(&attrName, "%s-dir", host) < 0)
    {
	errorExit("Out of memory in setDiskInfo");
    }

    /*
     * First create the attribute. There seems no way to tell whether
     * it already exists, so just create it regardless. If it already
     * exists this will fail obviously, but it shouldn't matter
     */
    result = globus_rls_client_lrc_attr_create(rlsHandle_, attrName,
					       globus_rls_obj_lrc_lfn,
					       globus_rls_attr_type_str);

    /*
     * Fill in the attribute's values
     */
    attr.name = attrName;
    attr.objtype = globus_rls_obj_lrc_lfn;
    attr.type = globus_rls_attr_type_str;

    /*
     * Try to remove attribute if it already exists
     */
    result = globus_rls_client_lrc_attr_remove(rlsHandle_, lfn, &attr);

    attr.val.s = disk;

    /*
     * Now try to set it for the particular file specified
     */
    result = globus_rls_client_lrc_attr_add(rlsHandle_, lfn, &attr);
    globus_libc_free(attrName);
    if (result != GLOBUS_SUCCESS)
    {
	logMessage(3, "globus_rls_client_lrc_attr_add failed for "
		   "setDiskInfo(%s,%s,%s)", host, lfn, disk);
	return 0;
    }
    
    return 1;
}

/***********************************************************************
*   char *constructFilename(char *host, char *lfn)
*    
*   Constructs the full path to a file in storage on a particular host
*    
*   Parameters:                                                    [I/O]
*
*     host The FQDN of the host storing the file                    I
*     lfn  The logical grid filename of the file                    I
*    
*   Returns: Pointer to full path name. Storage is malloced and should
*            be freed by the caller
***********************************************************************/
char *constructFilename(char *host, char *lfn)
{
    char *path;
    char *filename;
    char *disk;

    logMessage(1, "constructFilename(%s,%s)", host, lfn);

    /*
     * Warning: lfn is not necessarily a real logical filename, just a
     * path that's expected to exist in the data directory. It could
     * start with 'NEW/', in which case it needs handled differently
     * on multidisk storage elements
     */
    if (!strncmp(lfn, "NEW/", 4))
    {
	path = getNodePath(host);
	if (path == NULL)
	{
	    return NULL;
	}
	
	if (safe_asprintf(&filename, "%s/data/%s", path, lfn) < 0)
	{
	    errorExit("Out of memory in constructFilename");
	}
    }
    else
    {
	/*
	 * non-NEW version
	 */
	path = getNodePath(host);
	if (path == NULL)
	{
	    return NULL;
	}

	disk = getDiskInfo(host, lfn);
	if (disk == NULL)
	{
	    /* fall back to default here */
	    disk = safe_strdup("data");
	}
	
	if (safe_asprintf(&filename, "%s/%s/%s", path, disk, lfn) < 0)
	{
	    errorExit("Out of memory in constructFilename");
	}
	globus_libc_free(disk);
    }

    return filename;
}


/***********************************************************************
*   int getAttrValueFromRLS(char *lfn, char *name, char **value)
*    
*   Obtain a value of an attribute for a particular lfn 
*    
*   Parameters:                                                    [I/O]
*
*     name The name of the attribute required                       I
*     lfn  The logical grid filename of the file                    I
*     value The place holder for the returned value                 O 
*
*   Returns: void
*   
*   Note: Possible names of attributes are: 
*                  submitter, group, md5sum, size, permissions
***********************************************************************/ 
int getAttrValueFromRLS(char *lfn, char *name, char **value)
{
  logMessage(1, "running getAttrValueFromRLS with lfn=%s and name=%s", lfn, name);

  globus_list_t * attr_list;
  globus_result_t result;
  globus_rls_attribute_t *attr;

  result = globus_rls_client_lrc_attr_value_get(rlsHandle_,
						lfn,
						name,
						globus_rls_obj_lrc_lfn,
						&attr_list);
  
  if (result != GLOBUS_SUCCESS)
  {
    logMessage(5, "Error obtaining attribute value from the RLS. Attribute or LFN might not exist!");
    globus_rls_client_free_list(attr_list);
    return 0;
  }
  

  int size = globus_list_size(attr_list);
  if (size != 1)
  {
    logMessage(5, "Error: expected only one attribute value from the RLS query");
    globus_rls_client_free_list(attr_list);
    return 0;
  }
    
  attr = globus_list_first(attr_list);

  logMessage(1, "value=%s", attr->val);

  if (safe_asprintf(value, "%s", attr->val) < 0)
  {
    logMessage(5, "Out of memory in getAttrValueFromRLS");
    globus_rls_client_free_list(attr_list);
    return 0;
  }

  globus_rls_client_free_list(attr_list);
  return 1;
}


/***********************************************************************
*   qcdgrid_hash_table_t *getAllAttributesValues(char *attrName)
*    
*   Obtain all attribute values for a given attribute name
*    
*   Parameters:                                                    [I/O]
*
*     attrName The name of the attribute                            I
*
*   Returns: hashmap - a list of lfn->attribute value mappings
*            or NULL if error occured
*   
*   Note: Possible names of attributes are: 
*         submitter, group, md5sum, size, permissions or <nodeName>-dir
***********************************************************************/ 
qcdgrid_hash_table_t *getAllAttributesValues(char *attrName)
{
  qcdgrid_hash_table_t *dict;
  globus_result_t result;
  globus_list_t *list, *p;
  globus_rls_attribute_object_t *attrObj;

  int numFiles = -1;

  logMessage(1, "Starting getAllAttributesValues for attrib = %s", attrName);

  dict = newKeyAndValueHashTable();
  //  dict = createDict();

  char *resultStr;
  resultStr = strstr(attrName, "-dir");

#if 0
  if(((strcmp(attrName, "md5sum") != 0 && strcmp(attrName, "submitter") != 0) &&
     (strcmp(attrName, "size") != 0 && strcmp(attrName, "group") != 0)) &&
     ((strcmp(attrName, "permissions") != 0) &&
      ((resultStr == NULL) || strcmp(resultStr, "-dir") != 0)))
  {
    /* wrong attribute name, return NULL */
    logMessage(5, "Attribute '%s' was not recognised. Please use one of the following:\n"
	       "md5sum submitter group size permissions <node>-dir", attrName);
    return NULL;
  }
#endif

  logMessage(1, "Querying RLS for attributes...");
    /* Query the catalogue for all attributes */
    result = globus_rls_client_lrc_attr_search(rlsHandle_, attrName,
					       globus_rls_obj_lrc_lfn,
					       globus_rls_attr_op_all,
					       NULL, NULL, NULL, 0,
					       &list);
   logMessage(1, "Completed");

  if (result == GLOBUS_SUCCESS)
  {
    logMessage(1, "Globus success");
    numFiles = 0;

    p = list;
    while (p)
    {
      attrObj = (globus_rls_attribute_object_t *)globus_list_first(p);
      p = globus_list_rest(p);

      //      logMessage(1, "Adding to dict key=%s; value=%s;", attrObj->key, attrObj->attr.val.s);
      // dict = addToDict(dict, safe_strdup(attrObj->key), safe_strdup(attrObj->attr.val.s));

      if(!addKeyAndValueToHashTable(dict, attrObj->key, attrObj->attr.val.s))
      {
	logMessage(5, "Error adding to hash table for attribute: %s; key: %s; value: %s", attrName, attrObj->key, attrObj->attr.val.s);
	return NULL;
      }
      numFiles ++;
    }
    globus_rls_client_free_list(list);
  }

  logMessage(1, "There are %d results in getAllAttributesValues", numFiles++);

  return dict;
}

/***********************************************************************
*   long long *getFilespaceUsedOnNode(char *node, int *numDisks) 
*    
*   Get the amount of filespace in bytes taken up by each disk on a node.
*    
*   Parameters:                                                    [I/O]
*
*    	node 		The name of node                            	I
* 		numDisks	The number of disks on the node					O
*
*   Returns: An array of longs, representing the total file size stored on 
* 		 each disk: data, data1, data2, etc.
***********************************************************************/ 
long long *getFilespaceUsedOnNode(char *node, int *numDisks) {
	
	char *nodeName= NULL;
	/* An array of longs, representing the total file size stored on 
	 * each disk: data, data1, data2, etc.*/
	long long *filespaceUsedOnDisks = NULL;
	int diskItr;
    nodeName = globus_libc_malloc((strlen(node) + 5) * sizeof(char));
    strcpy(nodeName, node);
    strcat(nodeName, "-dir");
    /* all of the filenames on this node. */
    char **filesOnNode;
    /* An array of pointers to hash tables (terminated by a NULL). Each hash 
     * table contains all the filenames that reside on one disk (i.e. the first 
     * hash table contains all the files in 'data', the second all the files in
    *   'data1', the third all the files in 'data2', etc.)*/
    qcdgrid_hash_table_t **fileDisksList = NULL;
    
    *numDisks = 0;
    
	/* Get files listed at node in RC */
	filesOnNode = listLocationFiles(node);
	if (!filesOnNode) {
		logMessage(WARN, "No files currently registered at %s", node);
		return NULL;
	}
    
    // Get size of every file
    qcdgrid_hash_table_t *size_d = NULL;
    size_d = getAllAttributesValues("size");
    
    /* Query RLS for all the disk attributes on multidisk nodes */
    fileDisksList = getAllFileDisks(node);
    if (fileDisksList == NULL)
    {
	logMessage(WARN, "Error getting disk info for host %s", node);
	return NULL;
    }

    /* count the number of disk hash tables so we don't index off the end */
    int numRcDisks = 0;
    while (fileDisksList[numRcDisks])
    {
	numRcDisks++;
    }
    *numDisks = numRcDisks;
    
	logMessage(DEBUG, "numDisks %d", *numDisks);

    filespaceUsedOnDisks = globus_libc_malloc(numRcDisks * sizeof(long long));
    char *tempSize = NULL;
    // for each disk
    for (diskItr=0; diskItr<numRcDisks; diskItr++){

		logMessage(DEBUG, "diskItr %d", diskItr);
    	filespaceUsedOnDisks[diskItr] = 0;
    	long fileItr = 0;
    	// for every file listed on the node
		while (filesOnNode[fileItr]) {
			/* If the file is on this disk add it's size to the the total 
			 * size on this disk.*/
			if (lookupHashTable(fileDisksList[diskItr], filesOnNode[fileItr])) {
				logMessage(DEBUG, "Filename %s", filesOnNode[fileItr]);
				tempSize = lookupValueInHashTable(size_d, filesOnNode[fileItr]);
				if (tempSize) {
					filespaceUsedOnDisks[diskItr] += strtoll(tempSize, NULL, 10);
					logMessage(DEBUG, "Total file size is %qd",
							filespaceUsedOnDisks[diskItr]);
					globus_libc_free(tempSize);
				}
			}
			fileItr++;
		}
	}
    destroyFileDisksList(fileDisksList);
    destroyKeyAndValueHashTable(size_d);
    return filespaceUsedOnDisks;
}

/***********************************************************************
*   int setRLSAttribute(char *lfn, char *attr, char *val)
*    
*   Sets an attribute's value in RLS
*    
*   Parameters:                                [I/O]
*
*     lfn    logical filename                   I
*     attr   name of attribute to set           I
*     val    value to set it to                 I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int setRLSAttribute(char *lfn, char *attr, char *val)
{
    globus_rls_attribute_t gattr;
    globus_result_t res;
    
    gattr.objtype = globus_rls_obj_lrc_lfn;
    gattr.name = attr;
    gattr.type = globus_rls_attr_type_str;
    gattr.val.s = val;

    res = globus_rls_client_lrc_attr_modify(rlsHandle_, lfn, &gattr);
    if (res != GLOBUS_SUCCESS)
    {
	/*
	 * If it couldn't be modified, it might not have existed for
	 * that file before, so try adding it
	 */
	res = globus_rls_client_lrc_attr_add(rlsHandle_, lfn, &gattr);
    }
    /*
     * Ignore errors here as Globus decides to treat setting an
     * attribute to the same value it has already as an error
     */

    return 1;
}

/***********************************************************************
*   char *getRLSAttribute(char *lfn, char *attr)
*    
*   Retrieves an attribute from RLS
*    
*   Parameters:                                [I/O]
*
*     lfn    logical filename                   I
*     attr   name of attribute to get           I
*    
*   Returns: attribute value (caller should free), or NULL on error
***********************************************************************/
char *getRLSAttribute(char *lfn, char *attr)
{
    globus_list_t *attr_list;
    globus_result_t res;
    int size;
    globus_rls_attribute_t *gattr;
    char *attrval;

    res = globus_rls_client_lrc_attr_value_get(rlsHandle_, lfn, attr,
					       globus_rls_obj_lrc_lfn,
					       &attr_list);
    if (res != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error getting attribute %s for lfn %s", attr, lfn);
	return NULL;
    }

    size = globus_list_size(attr_list);
    if (size != 1)
    {
	logMessage(5, "RLS attr query returned multiple values for %s:%s", lfn, attr);
	globus_rls_client_free_list(attr_list);
	return NULL;
    }

    gattr = globus_list_first(attr_list);

    attrval = safe_strdup(gattr->val.s);
    if (!attrval)
    {
	logMessage(5, "Out of memory in getRLSAttribute");
	globus_rls_client_free_list(attr_list);
	return NULL;
    }

    globus_rls_client_free_list(attr_list);
    return attrval;
}


/***********************************************************************
*   int updateLastChecked(char *lfn, char *host)
*    
*   Updates the "last checked" attribute for the specified copy of the
*   specified file to the current time
*    
*   Parameters:                                   [I/O]
*
*     lfn    logical filename                      I
*     host   hostname containing replica of file   I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int updateLastChecked(char *lfn, char *host)
{
  char *attrName = NULL, *attrVal, *nl;
  time_t t;
  int result;

  /* 
   * work out name for attribute (it's per-replica so have to
   * include the host name)
   */
  if (safe_asprintf(&attrName, "%s-lastchecked", host) < 0) {
    logMessage(5, "Out of memory in updateLastChecked\n");
    return 0;
  }

  /*
   * get the current time, convert to text, remove the newline
   */
  t = time(NULL);
  attrVal = safe_strdup(ctime(&t));
  nl = strchr(attrVal, '\n');
  if (nl) {
    *nl = 0;
  }

  /*
   * actually set the attribute
   */
  result = registerAttrWithRc(lfn, attrName, attrVal);

  globus_libc_free(attrVal);
  globus_libc_free(attrName);

  return 1;
}
