/***********************************************************************
*
*   Filename:   digs-list.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*               Daragh Byrne           (daragh)   EPCC.
*               Radoslaw Ostrowski     (radek)    EPCC.
*
*   Purpose:    Command to list files on the QCDgrid
*
*   Contents:   Main function
*
*   Used in:    Day to day use of QCDgrid
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

#include "qcdgrid-client.h"
#include "replica.h"
#include "node.h"

void printUsage()
{
  /* show usage */
  globus_libc_printf("Usage: digs-list [arguments] [<wildcard>]\n");
  globus_libc_printf("  -n: show number of copies of each file\n");
  globus_libc_printf("  -w: print whereabouts (locations) of where the file is stored\n");
  globus_libc_printf("  -g: print group the file belongs to\n");
  globus_libc_printf("  -p: print permissions of the file\n");
  globus_libc_printf("  -c: print stored MD5 checksum\n");
  globus_libc_printf("  -s: print stored size\n");
  globus_libc_printf("  -o: print original submitter\n");
  globus_libc_printf("  -l: use all of the above lowercase arguments\n");
  globus_libc_printf("  -h or --help: show this usage information\n");
  globus_libc_printf("\n");  
  globus_libc_printf("Usage: digs-list [argument] <value> [...]\n");
  globus_libc_printf("  -W <node>: print all files stored on this node\n");
  globus_libc_printf("  -G <group>: print all files belonging to this group\n");
  globus_libc_printf("  -P public|private <group>: print all files having those permissions and owned by this group\n");
  globus_libc_printf("  -O <\"DN of a submitter\">: print all files submitted by this original submitter\n");
       
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of list command
*    
*   Parameters:
*
*     Optional parameter '-n' give number of copies of each file
*     Optional parameter '-l' gives locations of the file.
*     Optional filename or pattern narrows listing
*
*   Returns: 0 on success, 1 on error
*            Prints a list of files to standard output
***********************************************************************/

int main(int argc, char *argv[])
{
    int i, j;
    int showNumCopies = 0;
    int showLocations = 0;
    int showGroup = 0;
    int showChecksum = 0;
    int showSize = 0;
    int showSubmitter = 0;
    int showPermissions = 0;

    int showAll = 0;
    int showAllBySubmitter = 0;
    char *submitter = NULL;
    int showAllByGroup = 0;
    char *group = NULL;
    int showAllByPermissions = 0;
    char *permissions = NULL;
    int showAllByNode = 0;
    char *node = NULL;

    char *tmpGroup, *tmpPermissions, *tmpSubmitter;

    qcdgrid_hash_table_t *md5sum_d = NULL;
    qcdgrid_hash_table_t *size_d = NULL;
    qcdgrid_hash_table_t *submitter_d = NULL;
    qcdgrid_hash_table_t *group_d = NULL;
    qcdgrid_hash_table_t *permissions_d = NULL;
    qcdgrid_hash_table_t *node_d = NULL;

    int isWildcard = 1;
    char *wildcard="*";
    char *ssLFN="";

    logicalFileInfo_t *list;
    int numFiles;

    /*time_t start, end;*/

    //globus_libc_printf("Number of args: %d\n", argc);
    //for (i = 1; i < argc; i++){ 
    //globus_libc_printf("argv[%d] = %s\n", i, argv[i]);
    //}

    /* Parse command line arguments */
    for (i = 1; i < argc; i++)
    {      
      	if (strcmp(argv[i], "-l") == 0)
	{
	    showNumCopies = 1;
	    showLocations = 1;
	    showGroup = 1;
	    showChecksum = 1;
	    showSize = 1;
	    showSubmitter = 1;
	    showPermissions = 1;
	}
	else if (strcmp(argv[i], "-n") == 0)
	{
	    showNumCopies = 1;
	}
        else if( strcmp( argv[i], "-w" ) == 0 )
        {
            showLocations = 1;
        }
        else if( strcmp( argv[i], "-g" ) == 0 )
        {
            showGroup = 1;
        }
        else if( strcmp( argv[i], "-c" ) == 0 )
        {
            showChecksum = 1;
        }
        else if( strcmp( argv[i], "-s" ) == 0 )
        {
            showSize = 1;
        }
        else if( strcmp( argv[i], "-p" ) == 0 )
        {
            showPermissions = 1;
        }
        else if( strcmp( argv[i], "-o" ) == 0 )
        {
            showSubmitter = 1;
        }
        else if( strcmp( argv[i], "-W" ) == 0 )
        {
	  /* there is exactly one more arg */
	  if(i+2 == argc)
	  {
	    showAll = 1;
            showAllByNode = 1;
	    node = globus_libc_malloc((strlen(argv[i+1]) + 5) * sizeof(char));
	    strcpy(node, argv[i+1]);
	    strcat(node, "-dir");
	    break;
	  }
	  else
	  {
	    printUsage();
	    return 0;
	  }
        }
        else if( strcmp( argv[i], "-O" ) == 0 )
        {
	  /* there is exactly one more arg */
	  if(i+2 == argc)
	  {
	    showAll = 1;
            showAllBySubmitter = 1;
	    submitter = argv[i+1];
	    break;
	  }
	  else
	  {
	    printUsage();
	    return 0;
	  }
        }
        else if( strcmp( argv[i], "-G" ) == 0 )
        {
	  /* there is exactly one more arg */
	  if(i+2 == argc)
	  {
	    showAll = 1;
            showAllByGroup = 1;
	    group = argv[i+1];
	    break;
	  }
	  else
	  {
	    printUsage();
	    return 0;
	  }
        }
        else if( strcmp( argv[i], "-P" ) == 0 )
        {
	  /* there are exactly two more args */
	  if(i+3 == argc && 
	     (strcmp(argv[i+1], "private") == 0 || strcmp(argv[i+1], "public") == 0))
	  {
	    showAll = 1;
            showAllByPermissions = 1;

	    permissions = argv[i+1];

	    group = argv[i+2];
	    break;
	  }
	  else
	  {
	    printUsage();
	    return 0;
	  }
        }

	else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
	{
	  printUsage();
	  return 0;
	}
	else
	{
	    /* if we end up here an lfn was specified (as last arg) so no wildcard*/
	    isWildcard = 0;
	    /* get rid of double slashes */	    
            ssLFN = substituteChars(argv[i], "//", "/");
	}
    }

    if (!qcdgridInit(1))
    {
	globus_libc_fprintf(stderr, "Error loading config\n");
	return 1;
    }
    atexit(qcdgridShutdown);

    if(isWildcard)
    {
	list = getFileList(wildcard, &numFiles);
    }
    else
    {
	list = getFileList(ssLFN, &numFiles);
	globus_libc_free(ssLFN);
    }

    if (list == NULL)
    {
	globus_libc_printf("Error listing files\n");
	return 1;
    }
    if (showGroup || showAllByGroup || showAllByPermissions)
    {
      group_d = getAllAttributesValues("group");
    }
    if (showPermissions || showAllByPermissions)
    {
      permissions_d = getAllAttributesValues("permissions");
    }
    if (showChecksum)
    {
      md5sum_d = getAllAttributesValues("md5sum");
    }
    if (showSize)
    {
      size_d = getAllAttributesValues("size");
    }
    if (showSubmitter || showAllBySubmitter)
    {
      submitter_d = getAllAttributesValues("submitter");
    }
    if (showAllByNode)
    {
      node_d = getAllAttributesValues(node);
    }

    for (i = 0; i < numFiles; i++)
    {
        // print LFN first, but only if no lower case command was issued
	if(!showAll)
	{
	  globus_libc_printf("%s ", list[i].lfn);
	}
	if (showGroup)
	{
	  globus_libc_printf("%s ", lookupValueInHashTable(group_d, list[i].lfn));
	}
	if (showPermissions)
	{
	  globus_libc_printf("%s ", lookupValueInHashTable(permissions_d, list[i].lfn));
	}
	if (showSubmitter)
	{
	  globus_libc_printf("'%s' ", lookupValueInHashTable(submitter_d, list[i].lfn));
	}
	if (showSize)
	{
	  globus_libc_printf("%s ", lookupValueInHashTable(size_d, list[i].lfn));
	}
	if (showChecksum)
	{
	  globus_libc_printf("%s ", lookupValueInHashTable(md5sum_d, list[i].lfn));
	}
	if (showNumCopies)
	{
	    globus_libc_printf("(%d) ", list[i].numPfns);
	}
	if (showLocations)
	{
	    globus_libc_printf("[");
	    for (j = 0; j < list[i].numPfns - 1; j++)
	    {
		globus_libc_printf("%s ", getNodeName(list[i].pfns[j]));
	    }
	    globus_libc_printf("%s]", getNodeName(list[i].pfns[j]));
	}
	if(!showAll)
	{
	  globus_libc_printf("\n");
	}

	/* 'show all' commands */
	if (showAllByNode)
	{
	  if (lookupValueInHashTable(node_d, list[i].lfn) != NULL)
	  {
	    globus_libc_printf("%s\n", list[i].lfn);
	  }
	}	
	if (showAllBySubmitter)
	{
	  tmpSubmitter = lookupValueInHashTable(submitter_d, list[i].lfn);

	  if(tmpSubmitter != NULL)
	  {
	    if (strcmp(submitter, tmpSubmitter) == 0)
	      {
		globus_libc_printf("%s\n", list[i].lfn);
	      }
	    globus_libc_free(tmpSubmitter);
	  }
	}	
	if (showAllByGroup)
	{
	  tmpGroup = lookupValueInHashTable(group_d, list[i].lfn);

	  if(tmpGroup != NULL)
	  {
	    if (strcmp(group, tmpGroup) == 0)
	    {
	      globus_libc_printf("%s\n", list[i].lfn);
	    }
	    globus_libc_free(tmpGroup);
	  }
	}	
	if (showAllByPermissions)
	{
	    tmpGroup = lookupValueInHashTable(group_d, list[i].lfn);
	    tmpPermissions = lookupValueInHashTable(permissions_d, list[i].lfn);
	    
	    if(tmpGroup != NULL && tmpPermissions != NULL)
	    {
	        if(!strcmp(group, tmpGroup) && !strcmp(permissions, tmpPermissions))
		{
		    globus_libc_printf("%s\n", list[i].lfn);
		}
		globus_libc_free(tmpPermissions);
		globus_libc_free(tmpGroup);
	    }

	}	
    }


    /* now clean up */
    freeFileList(list, numFiles);
    if (showGroup || showAllByGroup || showAllByPermissions)
    {
      destroyKeyAndValueHashTable(group_d);
    }
    if (showPermissions || showAllByPermissions)
    {
      destroyKeyAndValueHashTable(permissions_d);
    }
    if (showSubmitter || showAllBySubmitter)
    {
      destroyKeyAndValueHashTable(submitter_d);
    }
    if (showSize)
    {
      destroyKeyAndValueHashTable(size_d);
    }
    if (showChecksum)
    {
      destroyKeyAndValueHashTable(md5sum_d);
    }
    if (showAllByNode)
    {
      destroyKeyAndValueHashTable(node_d);
      globus_libc_free(node);
    }

    return 0;
}

