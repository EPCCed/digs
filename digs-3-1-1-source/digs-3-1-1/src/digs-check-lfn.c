/***********************************************************************
*
*   Filename:   digs-check-lfn.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to check the
*               consistency of a logical file on a DiGS grid.
*
*   Contents:   Main function
*
*   Used in:    System administration of QCDgrid
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2008 The University of Edinburgh
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

#include <globus_common.h>

#include "qcdgrid-client.h"

#include "replica.h"
#include "node.h"
#include "job.h"
#include "misc.h"

/*
 * This structure holds the information we need about one physical
 * copy of a file
 */
typedef struct
{
    char *node;
    int nodeUnavailable;
    char md5sum[33];
    long long size;
} lfnCopyInfo_t;

/***********************************************************************
*   int getRemoteChecksum(char *lfn, char *pfn, lfnCopyInfo_t *info)
*    
*   Performs an MD5 checksum on a remote copy of a file. Stores the
*   checksum in the lfnCopyInfo_t struct
*    
*   Parameters:                                [I/O]
*
*     lfn    logical filename                   I
*     pfn    node where file is stored          I
*     info   receives the MD5 sum                 O
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int getRemoteChecksum(char *lfn, char *pfn, lfnCopyInfo_t *info)
{
    char *cksum;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;
    struct storageElement *se;
    char *path;

    path = constructFilename(pfn, lfn);
    if (path == NULL)
    {
	globus_libc_fprintf(stderr, "Construct filename failed in getRemoteChecksum\n");
	return 0;
    }

    se = getNode(pfn);
    if (!se)
    {
	globus_libc_fprintf(stderr, "getNode failed for %s in getRemoteChecksum\n", pfn);
	globus_libc_free(path);
	return 0;
    }

    result = se->digs_getChecksum(errbuf, path, pfn, &cksum, DIGS_MD5_CHECKSUM);
    globus_libc_free(path);
    if (result != DIGS_SUCCESS)
    {
	globus_libc_fprintf(stderr, "Getting remote checksum from %s failed\n", pfn);
	return 0;
    }

    strcpy(info->md5sum, cksum);
    se->digs_free_string(&cksum);
    return 1;
}

#define CORRECT_TIMEOUT 120.0

/***********************************************************************
*   int correctFileCopy(char *fromNode, char *toNode, char *lfn)
*    
*   Corrects the copy of file lfn stored on toNode, by copying the one
*   from fromNode over it.
*    
*   Parameters:                                    [I/O]
*
*     fromNode  node containing good copy of file   I
*     toNode    node containing copy to replace     I
*     lfn       logical filename                    I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int correctFileCopy(char *fromNode, char *toNode, char *lfn)
{
    char *srcname, *destname, *tmpname;
    
    globus_libc_printf("Correcting copy on %s (from %s)...\n", toNode,
		       fromNode);

    srcname = constructFilename(fromNode, lfn);
    destname = constructFilename(toNode, lfn);
    if ((!srcname) || (!destname))
    {
	globus_libc_fprintf(stderr, "Error constructing filenames\n");
	return 0;
    }

    tmpname = getTemporaryFile();
    if (!tmpname)
    {
	globus_libc_free(srcname);
	globus_libc_free(destname);
	globus_libc_fprintf(stderr, "Error getting temporary file\n");
	return 0;
    }

    if (!copyToLocal(fromNode, srcname, tmpname))
    {
	globus_libc_fprintf(stderr, "Error copying %s from %s\n", lfn, fromNode);
	globus_libc_free(srcname);
	globus_libc_free(destname);
	unlink(tmpname);
	globus_libc_free(tmpname);
	return 0;
    }

    if (!copyFromLocal(tmpname, toNode, destname))
    {
	globus_libc_fprintf(stderr, "Error copying %s to %s\n", lfn, toNode);
	globus_libc_free(srcname);
	globus_libc_free(destname);
	unlink(tmpname);
	globus_libc_free(tmpname);
	return 0;
    }

    globus_libc_free(srcname);
    globus_libc_free(destname);
    unlink(tmpname);
    globus_libc_free(tmpname);
    return 1;
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of the digs-check-lfn executable. Sets everything
*   up, then runs the check
*    
*   Parameters:                                [I/O]
*
*    argv[1]  LFN of file to check              I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
/*
 * Algorithm:
 *
 * First we gather up all the information we have on the logical file -
 * the checksum and size attributes from RLS, and the actual checksum
 * and size of each copy. We also make a note here of any nodes that
 * are disabled or not responding.
 *
 * Next we try to detect the 3 common cases:
 *
 * 1. All the checksums and sizes match, nothing needs to be done
 *
 * 2. All of the copies match the RLS metadata except one. That one
 *    copy is probably corrupted, give the user the option to correct
 *    it
 *
 * 3. All of the copies are the same but the RLS metadata is wrong.
 *    Give the user the option to correct the metadata
 *
 * Any other result means that there are multiple mismatches,
 * hopefully very unusual. In this case give the user all the
 * information we have and allow them to decide which copy is
 * "correct".
 */
int main(int argc, char *argv[])
{
    logicalFileInfo_t *info;
    int numFiles = 0;
    int i, j;
    char *pfn, *lfn;
    char *rlssizestr, *rlscksum;
    long long rlssize;
    lfnCopyInfo_t *copies;
    int unavailableNodes = 0;
    int mismatches;
    int firstMismatch, firstMatch;
    char c;
    int index[10];
    char sizebuf[50];

    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;
    struct storageElement *se;
    char *path;

    if (argc != 2)
    {
	globus_libc_printf("Usage: digs-check-lfn <logical filename>\n");
	return 0;
    }
  
    if (!qcdgridInit(0))
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	return 1;
    }
    atexit(qcdgridShutdown);

    lfn = argv[1];

    /* get file's basic info */
    info = getFileList(lfn, &numFiles);
    if (!info)
    {
	globus_libc_fprintf(stderr, "Error getting file info from RLS\n");
	return 1;
    }
    if (info[0].numPfns == 0)
    {
	globus_libc_fprintf(stderr, "No locations registered for file\n");
	return 1;
    }
    globus_libc_printf("%d copies of file\n", info[0].numPfns);

    /* get expected checksum and size from RLS */
    rlssizestr = getRLSAttribute(lfn, "size");
    if (rlssizestr == NULL)
    {
	/* use invalid size value if no attribute */
	rlssizestr = safe_strdup("-1");
    }

    rlscksum = getRLSAttribute(lfn, "md5sum");
    if (rlscksum == NULL)
    {
	/* use very unlikely checksum value if no attribute */
	rlscksum = safe_strdup("00000000000000000000000000000000");
    }

    rlssize = strtoll(rlssizestr, NULL, 10);
    globus_libc_free(rlssizestr);
    globus_libc_printf("RLS says file size is %qd, checksum is %s\n", rlssize, rlscksum);

    /* allocate space for copies information */
    copies = globus_libc_malloc(info[0].numPfns * sizeof(lfnCopyInfo_t));
    if (!copies)
    {
	globus_libc_printf("Out of memory\n");
	return 1;
    }

    /* loop over each copy of file */
    for (i = 0; i < info[0].numPfns; i++)
    {
	pfn = getNodeName(info[0].pfns[i]);
	if (!pfn)
	{
	    globus_libc_printf("Error getting node name for file location\n");
	    /* keep going, might just be one corrupted entry */
	    continue;
	}
	globus_libc_printf("File has location at %s\n", pfn);

	copies[i].node = pfn;
	copies[i].nodeUnavailable = 0;
	copies[i].size = 0LL;
	strcpy(copies[i].md5sum, "");

	/* get size and checksum on this node */
	se = getNode(pfn);
	if (!se)
	{
	    globus_libc_printf("Error getting node structure for %s, skipping\n", pfn);
	    copies[i].nodeUnavailable = 1;
	    copies[i].size = 0LL;
	    unavailableNodes++;
	    continue;	    
	}

	path = constructFilename(pfn, lfn);
	result = se->digs_getLength(errbuf, path, pfn, &copies[i].size);
	if (result != DIGS_SUCCESS)
	{
	    globus_libc_printf("Failed to get file size from node %s, skipping\n", pfn);
	    globus_libc_free(path);
	    copies[i].nodeUnavailable = 1;
	    copies[i].size = 0LL;
	    unavailableNodes++;
	    continue;
	}
	globus_libc_free(path);
	if (!getRemoteChecksum(lfn, pfn, &copies[i]))
	{
	    globus_libc_printf("Failed to get checksum from node %s, skipping\n", pfn);
	    copies[i].nodeUnavailable = 1;
	    copies[i].size = 0LL;
	    unavailableNodes++;
	    continue;
	}

	globus_libc_printf("Size on this node: %qd, checksum: %s\n", copies[i].size, copies[i].md5sum);
    }

    /* got info from all the nodes, now process it */
    /* check for all nodes being down */
    if (unavailableNodes == info[0].numPfns)
    {
	globus_libc_printf("\nAll nodes are disabled or not responding, please try again later.\n");
	globus_libc_free(copies);
	globus_libc_free(rlscksum);
	return 0;
    }

    if (unavailableNodes)
    {
	globus_libc_printf("\nWarning: one or more nodes hosting copies of this file was down.\n");
	globus_libc_printf("This information may be incomplete.\n\n");
    }

    /* check for everything matching */
    mismatches = 0;
    firstMismatch = -1;
    firstMatch = -1;
    for (i = 0; i < info[0].numPfns; i++)
    {
	/* skip dead nodes */
	if (copies[i].nodeUnavailable) continue;

	/* check for mismatch */
	if ((copies[i].size != rlssize) || (strcmp(copies[i].md5sum, rlscksum)))
	{
	    mismatches++;
	    if (firstMismatch < 0)
	    {
		firstMismatch = i;
	    }
	}
	else
	{
	    if (firstMatch < 0)
	    {
		firstMatch = i;
	    }
	}
    }

    if (mismatches == 0)
    {
	/* they all matched */
	globus_libc_printf("\nAll copies match\n");
	globus_libc_free(copies);
	globus_libc_free(rlscksum);
	return 0;
    }

    if ((mismatches == 1) && ((info[0].numPfns - unavailableNodes) > 1))
    {
	/* one copy mismatched the RLS attributes */
	globus_libc_printf("\nCopy on %s doesn't match RLS. Fix this copy? (Y/N)\n", copies[firstMismatch].node);
	c = fgetc(stdin);
	if ((c == 'y') || (c == 'Y'))
	{
	    /*
	     * Node that failed is firstMismatch. A good copy is in firstMatch
	     */
	    if (firstMatch < 0)
	    {
		globus_libc_fprintf(stderr, "No accessible good copy to fix from\n");
	    }
	    else
	    {
		correctFileCopy(copies[firstMatch].node, copies[firstMismatch].node, lfn);
	    }
	}
	globus_libc_free(copies);
	globus_libc_free(rlscksum);
	return 0;
    }

    if (mismatches == (info[0].numPfns - unavailableNodes))
    {
	int firstWorkingNode, allTheSame;
	/* they all mismatched the RLS attributes! */
	/*
	 * Check if the file checksums are all the same.
	 */
	firstWorkingNode = -1;
	allTheSame = 1;
	for (i = 0; i < info[0].numPfns; i++)
	{
	    if (copies[i].nodeUnavailable) continue;

	    if (firstWorkingNode < 0)
	    {
		firstWorkingNode = i;
	    }
	    else
	    {
		if (strcmp(copies[i].md5sum, copies[firstWorkingNode].md5sum))
		{
		    allTheSame = 0;
		}
	    }
	}

	/*
	 * If they are all the same, prompt user to fix RLS attributes and return
	 */
	if (allTheSame)
	{
	    globus_libc_printf("\nRLS attributes don't match actual files. Correct the attributes? (Y/N)\n");
	    c = fgetc(stdin);
	    if ((c == 'y') || (c == 'Y'))
	    {
		sprintf(sizebuf, "%qd", copies[firstWorkingNode].size);
		if (!setRLSAttribute(lfn, "size", sizebuf))
		{
		    globus_libc_fprintf(stderr, "Error setting size attribute\n");
		    return 1;
		}
		if (!setRLSAttribute(lfn, "md5sum", copies[firstWorkingNode].md5sum))
		{
		    globus_libc_fprintf(stderr, "Error setting md5sum attribute\n");
		    return 1;
		}
	    }
	    globus_libc_free(copies);
	    globus_libc_free(rlscksum);
	    return 0;
	}
    }

    /*
     * Dealt with all the common cases. Now let user choose one copy as the right one, copy it
     * over the incorrect ones and the RLS attributes
     */
    globus_libc_printf("\nThere are multiple versions of this file. If you know which copy is correct, enter its number.\n");
    globus_libc_printf("If not, press enter to exit.\n");

    j = 0;
    for (i = 0; i < info[0].numPfns; i++)
    {
	if (copies[i].nodeUnavailable) continue;

	globus_libc_printf("%d. %s: size %qd, checksum %s\n", j+1, copies[i].node, copies[i].size, copies[i].md5sum);
	index[j] = i;
	j++;
    }

    c = fgetc(stdin);
    if ((c >= '1') && (c < ('1'+j)))
    {
	/* firstMatch is our copy to fix from */
	firstMatch = index[c - '1'];

	for (i = 0; i < info[0].numPfns; i++)
	{
	    if ((!copies[i].nodeUnavailable) && (strcmp(copies[i].md5sum, copies[firstMatch].md5sum)))
	    {
		/* doesn't match the chosen copy, fix it */
		if (!correctFileCopy(copies[firstMatch].node, copies[i].node, lfn))
		{
		    globus_libc_fprintf(stderr, "Copy from %s to %s failed, aborting\n", copies[firstMatch].node,
					copies[i].node);
		    globus_libc_free(copies);
		    globus_libc_free(rlscksum);
		    return 1;
		}
	    }
	}

	/* fix the RLS attributes too */
	sprintf(sizebuf, "%qd", copies[firstMatch].size);
	if (!setRLSAttribute(lfn, "size", sizebuf))
	{
	    globus_libc_fprintf(stderr, "Error setting size attribute\n");
	    return 1;
	}
	if (!setRLSAttribute(lfn, "md5sum", copies[firstMatch].md5sum))
	{
	    globus_libc_fprintf(stderr, "Error setting md5sum attribute\n");
	    return 1;
	}
    }

    globus_libc_free(copies);
    globus_libc_free(rlscksum);

    return 0;
}
