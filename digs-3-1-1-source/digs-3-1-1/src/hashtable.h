/***********************************************************************
*
*   Filename:   hashtable.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Simple hash table implementation
*
*   Contents:   Function prototypes for this module
*             
*   Used in:    Called by RLS related modules
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
#ifndef HASHTABLE_H
#define HASHTABLE_H

/*
 * Hash table functions and structures. The hash table is used by the
 * RLS module to efficiently determine a list of all files on the grid
 * Important: this must always be a power of 2!
 */
#define NUM_HASH_BUCKETS  4096

typedef struct qcdgrid_hash_bucket_s
{
    int numEntries;
    int numAlloced;
    char **entries;
    char **values;
} qcdgrid_hash_bucket_t;

typedef struct qcdgrid_hash_table_s
{
    qcdgrid_hash_bucket_t h[NUM_HASH_BUCKETS];
} qcdgrid_hash_table_t;

qcdgrid_hash_table_t *newHashTable();
int addToHashTable(qcdgrid_hash_table_t *ht, char *str);
int lookupHashTable(qcdgrid_hash_table_t *ht, char *str);
int lookupHashTableAndRemove(qcdgrid_hash_table_t *ht, char *str);
void destroyHashTable(qcdgrid_hash_table_t *ht);
void forEachHashTableEntry(qcdgrid_hash_table_t *ht,
			   void (*callback)(char*, void*),
			   void *cbParam);
unsigned int hashString(char *str);


/*********************************************************************
                   Dictionary version of hashable
*********************************************************************/
qcdgrid_hash_table_t *newKeyAndValueHashTable();
int addKeyAndValueToHashTable(qcdgrid_hash_table_t *ht, 
			      char *hashableKey, char *value);
char *lookupValueInHashTable(qcdgrid_hash_table_t *ht, char *key);
void forEachHashTableKeyAndValue(qcdgrid_hash_table_t *ht,
			         void (*callback)(char*, char*, void*),
			         void *cbParam);
void destroyKeyAndValueHashTable(qcdgrid_hash_table_t *ht);

#endif
