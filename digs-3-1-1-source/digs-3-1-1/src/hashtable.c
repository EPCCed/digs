/***********************************************************************
*
*   Filename:   hashtable.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implements a simple hash table
*
*   Contents:   Hash table manipulation functions
*
*   Used in:    Replica catalogue contents manipulation
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
#include "hashtable.h"

/***********************************************************************
*   static unsigned char hashString(char *str)
* 
*   Generate a simple 8-bit hash code from a string
*    
*   Parameters:                                             [I/O]
*
*       str     string to hash                               I
*    
*   Returns: the hash code
***********************************************************************/
unsigned int hashString(char *str)
{
    unsigned int hash;
    int i;

    hash=0;
    i=0;
    while (str[i])
    {
	hash ^= (unsigned char)str[i];
	hash = (hash << 3) | (hash >> 5);
	i++;
    }

    return hash;
}

/***********************************************************************
*   qcdgrid_hash_table_t *newHashTable()
* 
*   Creates an empty hash table
*    
*   Parameters:                                             [I/O]
*
*     (none)
*    
*   Returns: pointer to new hash table
***********************************************************************/
qcdgrid_hash_table_t *newHashTable()
{
    qcdgrid_hash_table_t *ht;
    int i;

    ht = globus_libc_malloc(sizeof(qcdgrid_hash_table_t));
    if (!ht)
    {
	errorExit("Out of memory in newHashTable");
    }

    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
	ht->h[i].numEntries = 0;
	ht->h[i].numAlloced = 0;
	ht->h[i].entries = NULL;
	ht->h[i].values = NULL;
    }

    logMessage(1, "newHashTable: %p", ht);

    return ht;
}


/***********************************************************************
*   int addToHashTable(qcdgrid_hash_table_t *ht, char *str)
* 
*   Adds a string to a hash table
*    
*   Parameters:                                             [I/O]
*
*       ht      hash table to add to                         I
*       str     string to add                                I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int addToHashTable(qcdgrid_hash_table_t *ht, char *str)
{
    unsigned int hash;

    hash = hashString(str);

    hash &= (NUM_HASH_BUCKETS-1);

    ht->h[hash].numEntries++;
    if (ht->h[hash].numEntries > ht->h[hash].numAlloced)
    {
	ht->h[hash].numAlloced += 20;
	ht->h[hash].entries = globus_libc_realloc(ht->h[hash].entries,
						  ht->h[hash].numAlloced *
						  sizeof(char*));
	if (!ht->h[hash].entries)
	{
	    errorExit("Out of memory in addToHashTable");
	}
    }

    ht->h[hash].entries[ht->h[hash].numEntries-1] = safe_strdup(str);
    if (!ht->h[hash].entries[ht->h[hash].numEntries-1])
    {
	errorExit("Out of memory in addToHashTable");
    }

    return 1;
}

/***********************************************************************
*   int lookupHashTable(qcdgrid_hash_table_t *ht, char *str)
* 
*   Looks up a string in the hash table
*    
*   Parameters:                                             [I/O]
*
*       ht      hash table to look in                        I
*       str     string to look for                           I
*    
*   Returns: 1 if string was found, 0 if not
***********************************************************************/
int lookupHashTable(qcdgrid_hash_table_t *ht, char *str)
{
    unsigned int hash;
    int i;

    hash = hashString(str);
    hash &= (NUM_HASH_BUCKETS-1);
    for (i = 0; i < ht->h[hash].numEntries; i++)
    {
	if (!strcmp(ht->h[hash].entries[i], str))
	{
	    return 1;
	}
    }
    return 0;
}

/***********************************************************************
*   int lookupHashTableAndRemove(qcdgrid_hash_table_t *ht, char *str)
* 
*   Looks up a string in the hash table. If it was there, remove it
*    
*   Parameters:                                             [I/O]
*
*       ht      hash table to look in                        I
*       str     string to look for                           I
*    
*   Returns: 1 if string was found, 0 if not
***********************************************************************/
int lookupHashTableAndRemove(qcdgrid_hash_table_t *ht, char *str)
{
    unsigned int hash;
    int i;

    hash = hashString(str);
    hash &= (NUM_HASH_BUCKETS-1);
    for (i = 0; i < ht->h[hash].numEntries; i++)
    {
	if (!strcmp(ht->h[hash].entries[i], str))
	{
	    globus_libc_free(ht->h[hash].entries[i]);
	    //globus_libc_free(ht->h[hash].values[i]);

	    ht->h[hash].numEntries--;
	    for (; i < ht->h[hash].numEntries; i++)
	    {
		ht->h[hash].entries[i] = ht->h[hash].entries[i+1];
		//ht->h[hash].values[i] = ht->h[hash].values[i+1];
	    }

	    return 1;
	}
    }
    return 0;
}

/***********************************************************************
*   void forEachHashTableEntry(qcdgrid_hash_table_t *ht,
*                              void (*callback)(char*, void*),
*                              void *cbParam)
* 
*   Calls a function for each string in a hash table
*    
*   Parameters:                                             [I/O]
*
*       ht        hash table to look in                      I
*       callback  the function to call                       I
*       cbParam   custom parameter to pass to callback       I
*    
*   Returns: (void)
***********************************************************************/
void forEachHashTableEntry(qcdgrid_hash_table_t *ht,
			   void (*callback)(char*, void*),
			   void *cbParam)
{
    int i, j;
    
    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
	for (j = 0; j < ht->h[i].numEntries; j++)
	{
	    callback(ht->h[i].entries[j], cbParam);
	}
    }
}

/***********************************************************************
*   void destroyHashTable(qcdgrid_hash_table_t *ht)
* 
*   Frees all the memory used by a hash table
*    
*   Parameters:                                             [I/O]
*
*       ht      hash table to destroy                        I
*    
*   Returns: (void)
***********************************************************************/
void destroyHashTable(qcdgrid_hash_table_t *ht)
{
    int i, j;

    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
	for (j = 0; j < ht->h[i].numEntries; j++)
	{
	    globus_libc_free(ht->h[i].entries[j]);
	}

	if (ht->h[i].entries)
	{
	    globus_libc_free(ht->h[i].entries);
	}
    }

    globus_libc_free(ht);

    logMessage(1, "destroyHashTable: %p", ht);
}


/*********************************************************************
                   Dictionary version of hashable
*********************************************************************/

/***********************************************************************
*   qcdgrid_hash_table_t *newKeyAndValueHashTable()
* 
*   Creates an empty key and value hash table
*    
*   Parameters:                                             [I/O]
*
*     (none)
*    
*   Returns: pointer to new hash table
***********************************************************************/
qcdgrid_hash_table_t *newKeyAndValueHashTable()
{
    qcdgrid_hash_table_t *ht;
    int i;

    ht = globus_libc_malloc(sizeof(qcdgrid_hash_table_t));
    if (!ht)
    {
	errorExit("Out of memory in newHashTable");
    }

    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
	ht->h[i].numEntries = 0;
	ht->h[i].numAlloced = 0;
	ht->h[i].entries = NULL;
	ht->h[i].values = NULL;
    }

    logMessage(1, "newKeyAndValueHashTable: %p", ht);

    return ht;
}


/***********************************************************************
*   int addKeyAndValueToHashTable(qcdgrid_hash_table_t *ht, 
*                                 char *hashableKey, char *value)
* 
*   Adds a value to a hash table for a particular key
*    
*   Parameters:                                             [I/O]
*
*       ht            hash table to add to                   I
*       hashableKey   key to be hashed                       I
*       value         value of key to be added               I
*
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int addKeyAndValueToHashTable(qcdgrid_hash_table_t *ht, char *hashableKey, char *value)
{
    unsigned int hash;

    hash = hashString(hashableKey);

    hash &= (NUM_HASH_BUCKETS-1);

    ht->h[hash].numEntries++;
    if (ht->h[hash].numEntries > ht->h[hash].numAlloced)
    {
	ht->h[hash].numAlloced += 20;
	ht->h[hash].entries = globus_libc_realloc(ht->h[hash].entries,
						  ht->h[hash].numAlloced *
						  sizeof(char*));

	ht->h[hash].values = globus_libc_realloc(ht->h[hash].values,
						  ht->h[hash].numAlloced *
						  sizeof(char*));

	if ((!ht->h[hash].entries) || (!ht->h[hash].values))
	{
	    errorExit("Out of memory in addToHashTable");
	}
    }

    ht->h[hash].entries[ht->h[hash].numEntries-1] = safe_strdup(hashableKey);
    ht->h[hash].values[ht->h[hash].numEntries-1] = safe_strdup(value);

    if ((!ht->h[hash].entries[ht->h[hash].numEntries-1]) || (!ht->h[hash].values[ht->h[hash].numEntries-1]))
    {
	errorExit("Out of memory in addToHashTable");
    }

    return 1;
}


/***********************************************************************
*   void forEachHashTableKeyAndValue(qcdgrid_hash_table_t *ht,
*                                    void (*callback)(char*, char*, void*),
*                                    void *cbParam)
* 
*   Calls a function for each key and corresponding value in a hash table
*    
*   Parameters:                                             [I/O]
*
*       ht        hash table to look in                      I
*       callback  the function to call                       I
*       cbParam   custom parameter to pass to callback       I
*    
*   Returns: (void)
***********************************************************************/
void forEachHashTableKeyAndValue(qcdgrid_hash_table_t *ht,
			         void (*callback)(char*, char*, void*),
			         void *cbParam)
{
    int i, j;
    
    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
	for (j = 0; j < ht->h[i].numEntries; j++)
	{
	    callback(ht->h[i].entries[j], ht->h[i].values[j], cbParam);
	}
    }
}


/***********************************************************************
*   char * lookupValueInHashTable(qcdgrid_hash_table_t *ht, char *key)
* 
*   Looks up a string in the key and value hash table
*    
*   Parameters:                                             [I/O]
*
*       ht      hash table to look in                        I
*       key     string to look for                           I
*    
*   Returns: value of the key if string was found, NULL if not
***********************************************************************/
char * lookupValueInHashTable(qcdgrid_hash_table_t *ht, char *key)
{
    unsigned int hash;
    int i;

    hash = hashString(key);
    hash &= (NUM_HASH_BUCKETS-1);
    for (i = 0; i < ht->h[hash].numEntries; i++)
    {
	if (!strcmp(ht->h[hash].entries[i], key))
	{
	  return safe_strdup(ht->h[hash].values[i]);
	}
    }
    return NULL;
}


/***********************************************************************
*   void destroyKeyAndValueHashTable(qcdgrid_hash_table_t *ht)
* 
*   Frees all the memory used by a key and value hash table
*    
*   Parameters:                                             [I/O]
*
*       ht      hash table to destroy                        I
*    
*   Returns: (void)
***********************************************************************/
void destroyKeyAndValueHashTable(qcdgrid_hash_table_t *ht)
{
    int i, j;

    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
	for (j = 0; j < ht->h[i].numEntries; j++)
	{
	    globus_libc_free(ht->h[i].entries[j]);
	    globus_libc_free(ht->h[i].values[j]);
	}

	if (ht->h[i].entries)
	{
	    globus_libc_free(ht->h[i].entries);
	}
	if (ht->h[i].values)
	{
	    globus_libc_free(ht->h[i].values);
	}
    }

    globus_libc_free(ht);

    logMessage(1, "destroyKeyAndValueHashTable: %p", ht);
}
