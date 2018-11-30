/***********************************************************************
*
*   Filename:   replica.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Interfaces with the Globus replica catalogue
*
*   Contents:   Function prototypes for this module
*
*   Used in:    Called by other modules to access the replica catalogue
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

#ifndef REPLICA_H
#define REPLICA_H

#include "misc.h"
#include "hashtable.h"
//#include "dictionary.h"

/*
 * Maximum number of physical filenames that a logicalFileInfo_t struct
 * can hold. 20 should be easily enough
 */
#define MAX_PFNS 20

/*
 * Information on a logical file, from RLS
 */
typedef struct logicalFileInfo_s
{
    /* number of locations for this file */
    int numPfns;

    /* logical filename */
    char *lfn;

    /* hash of lfn */
    int hash;

    /* array of indices into main node list (file's locations) */
    char pfns[MAX_PFNS];

    /* link to next struct for hash table */
    int next;
} logicalFileInfo_t;

/*
 * Called at startup by the main initialisation function. Given the hostname
 * and distinguished name of the replica catalogue, opens it and stores the
 * handle for later use by the other functions in the module.
 */
int openReplicaCatalogue(char *hostname);

/*
 * Closes the handle to the replica catalogue
 */
void closeReplicaCatalogue();

logicalFileInfo_t *getFileList(char *wildcard, int *numfiles);

/*
 * Removes an entire location from the RC including all the filenames
 * stored therein
 */
int deleteEntireLocation(char *location);

/*
 * The main replica catalogue update function. Given a storage element name 
 * and a logical filename, registers the file in the location. Takes care of
 * creating the location and/or file entries if they don't already exist.
 *
 * Returns 1 on success, 0 on failure
 */
int registerFileWithRc(char *node, char *lfn);

/*
 * Removes a mapping from a logical filename to a certain location.
 *
 * Returns 1 on success, 0 on failure
 */
int removeFileFromLocation(char *node, char *lfn);

/*
 * Removes a filename from the QCDgrid collection. Called when a file is
 * deleted from the grid altogether
 */
int removeFileFromCollection(char *lfn);

/*
 * Returns a list of the filenames registered at the given location
 */
char **listLocationFiles(char *location);

/*
 * Frees the file list returned by listLocationFiles
 */
void freeLocationFileList(char **list);

/*
 * Queries the RC to check if the file is registered in the collection. If 
 * so, it means the file is on the grid.
 *
 * Returns 1 if file is on grid, 0 if not
 *
 */
int fileInCollection(char *lfn);

/*
 * Queries the RC to check if the file is registered at the location.
 *
 * Returns 1 if the file is at the location, 0 if not
 */
int fileAtLocation(char *node, char *lfn);

/*
 * Returns the first registered location of a logical file, or NULL if it's
 * not on the grid at all. The pointer returned points to dynamically 
 * allocated memory and should be freed by the caller to avoid memory 
 * leakage.
 *
 * The "All" version does not skip dead and disabled nodes.
 */
char *getFirstFileLocation(char *lfn);
char *getFirstFileLocationAll(char *lfn);

/*
 * When called after the above function, returns the next location of the 
 * file (or NULL if all locations have already been returned). Can be used 
 * with the previous function to iterate over all the locations of a file
 */
char *getNextFileLocation();

/*
 * Returns the location of the most suitable copy of the file to transfer,
 * based on the local machine's node preference list
 */
char *getBestCopyLocation(char *lfn);

/*
 * Can be called if the host suggested by the above function is down. 
 * Returns the next most suitable host
 */
char *getNextBestCopyLocation(char *lfn);

/* flags for getNumCopies */
enum { GNC_COUNTRETIRING = 1 };

/*
 * Returns the number of copies of the file on the grid. Used by the control
 * thread to check that there are enough replicas (and if space is running
 * low, also that there aren't too many)
 */
int getNumCopies(char *lfn, int flags);

/*
 * These two functions are used to iterate over all the files on the 
 * QCDgrid. When all the files have been exhausted, the next call to 
 * get_next_file will return NULL to indicate the end.
 */
char *getFirstFile();
char *getNextFile();

/*
 * Obtains the actual path to a data file, based on its host and logical
 * filename
 */
char *constructFilename(char *host, char *lfn);

/*
 * Returns which disk the file is stored on at a particular host
 */
char *getDiskInfo(char *host, char *lfn);

/*
 * Sets which disk the file is stored on at a particular host
 */
int setDiskInfo(char *host, char *lfn, char *disk);

/*
 * Returns a dictionary of key-value pairs, where key is an lfn and value is a 
 * value of corresponding attribute stored in RLS (e.g. md5sum, submitter etc.)
 */
qcdgrid_hash_table_t *getAllAttributesValues(char *attrName);


/***********************************************************************
*   long long *getFilespaceUsedOnNode(char *node, int *numDisks) 
*    
*   Get the amount of filespace taken up by each disk on a node.
*    
*   Parameters:                                                    [I/O]
*
*    	node 		The name of node                            	I
* 		numDisks	The number of disks on the node					O
*
*   Returns: An array of longs, representing the total file size stored on 
* 		 each disk: data, data1, data2, etc.
***********************************************************************/ 
long long *getFilespaceUsedOnNode(char *node, int *numDisks) ;

/*
 * Obtains the disk attributes from all files on a node.
 *
 */
qcdgrid_hash_table_t **getAllFileDisks(char *location);

/*
 * Obtains the value of a single RLS attribute for a single LFN
 */
char *getRLSAttribute(char *lfn, char *attr);

/*
 * Sets the value of an RLS attribute for a single LFN
 */
int setRLSAttribute(char *lfn, char *attr, char *val);

int getAttrValueFromRLS(char *lfn, char *name, char **value);

void freeFileList(logicalFileInfo_t *list, int numfiles);

int registerAttrWithRc(char *lfn, char *key, char *value);

void destroyFileDisksList(qcdgrid_hash_table_t **hts);

int removeLfnAttribute(char *lfn, char *key);

int updateLastChecked(char *lfn, char *host);

#endif
