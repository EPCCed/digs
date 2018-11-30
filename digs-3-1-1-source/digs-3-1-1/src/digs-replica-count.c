/***********************************************************************
*
*   Filename:   digs-replica-count.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to set/query the
*               replica count of files on a DiGS system
*
*   Contents:   Main function
*
*   Used in:    Day to day usage of DiGS
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2007-2008 The University of Edinburgh
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

#include "qcdgrid-client.h"
#include "misc.h"
#include "replica.h"
#include "hashtable.h"
#include "config.h"

/* used to pass needed information into callbacks */
typedef struct {
  qcdgrid_hash_table_t *ht;
  int rc;
} checkCallbackParam_t;

/*
 * Callback function, called for each file in the logical directory
 * while printing the replication count
 */
static int printCallback(char *lfn, void *param)
{
  char *rc;
  int count = 0;
  checkCallbackParam_t *cbp = (checkCallbackParam_t *)param;

  rc = lookupValueInHashTable(cbp->ht, lfn);
  if (rc) {
    if (strcmp(rc, "(null)")) {
      count = atoi(rc);
    }
    globus_libc_free(rc);
  }

  if (count != 0) {
    /* print it if not default */
    globus_libc_printf("Replication count for %s is %d\n", lfn, count);
  }
  return 1;
}

/*
 * Callback function, called for each file in the logical directory
 * while checking whether the replication counts are all the same
 */
static int checkCallback(char *lfn, void *param)
{
  char *rc;
  int count = 0;
  checkCallbackParam_t *cbp = (checkCallbackParam_t *)param;

  rc = lookupValueInHashTable(cbp->ht, lfn);
  if (rc) {
    if (strcmp(rc, "(null)")) {
      count = atoi(rc);
    }
    globus_libc_free(rc);
  }

  if (cbp->rc < 0) {
    /* if first file, store count here */
    cbp->rc = count;
  }
  else {
    if (cbp->rc != count) {
      /* counts differ, return failure */
      return 0;
    }
  }

 /* counts matched, OK */
  return 1;
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of digs-replica-count executable. Sets everything
*   up, then sends a message to the main node
*    
*   Parameters:                                            [I/O]
*
*     (optional) "-R" or "--recursive" to set/query the     I
*                count for a directory tree recursively
*     (optional) "-c" to check the existing replica count   I
*                instead of changing it
*     (optional) "-d" to set the replica count back to the  I
*                default for this file/directory
*     Logical filename of file to be unlocked               I
*     New replica count value (not needed if "-c" or "-d"   I
*       given)
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
  char *usage = "Usage: digs-replica-count [-c] [-d] [-R|--recursive] "
    "<filename> [<count>]\n";
  int i;
  int recursive = 0;
  int check = 0;
  int deflt = 0;
  int count = -1;
  char *filename = NULL;

  /* check params */
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-c")) {
      check = 1;
    }
    else if (!strcmp(argv[i], "-d")) {
      deflt = 1;
    }
    else if ((!strcmp(argv[i], "-R")) || (!strcmp(argv[i], "--recursive"))) {
      recursive = 1;
    }
    else {
      if (filename == NULL) {
	filename = argv[i];
      }
      else if (count < 0) {
	count = atoi(argv[i]);
      }
      else {
	globus_libc_printf(usage);
	return 1;
      }
    }
  }

  /* sanity check arguments */
  if ((filename == NULL) ||
      ((count < 0) && (deflt == 0) && (check == 0)) ||
      ((count >= 0) && (deflt == 1)) ||
      ((count >= 0) && (check == 1)) ||
      ((deflt == 1) && (check == 1))) {
    globus_libc_printf(usage);
    return 1;
  }
  
  /*
   * Initialise grid
   */
  if (!qcdgridInit(0)) {
    globus_libc_fprintf(stderr, "Error loading grid config\n");
    return 1;
  }
  atexit(qcdgridShutdown);

  if (check) {
    /* checking count of file/directory */
    char *rc;
    if (recursive) {
      checkCallbackParam_t cbp;

      cbp.ht = getAllAttributesValues("replcount");
      cbp.rc = -1;

      /*
       * Recursive check: if all files in directory have the same replication
       * count, print it out in one line. If they're different need to print
       * them individually
       */
      if (forEachFileInDirectory(filename, checkCallback, &cbp)) {
	/* can do easy version */
	if (cbp.rc == 0) {
	  globus_libc_printf("Directory %s has default replication count (%d)\n",
			     filename, getConfigIntValue("miscconf", "min_copies", 2));
	}
	else {
	  globus_libc_printf("Directory %s replication count is %d\n", filename,
			     cbp.rc);
	}
      }
      else {
	/* have to do them individually */
	forEachFileInDirectory(filename, printCallback, &cbp);
      }

      destroyKeyAndValueHashTable(cbp.ht);
    }
    else {
      /* check replication count for a single file */
      rc = getRLSAttribute(filename, "replcount");
      if ((rc == NULL) || (!strcmp(rc, "(null)"))) {
	globus_libc_printf("Replication count for %s defaults to %d\n", filename,
			   getConfigIntValue("miscconf", "min_copies", 2));
      }
      else {
	globus_libc_printf("Replication count for %s is %s\n", filename, rc);
      }
      if (rc) globus_libc_free(rc);
    }
  }
  else {
    char *msgBuffer;

    if (deflt) {
      /* default is represented by a count of 0 */
      count = 0;
    }

    if (recursive) {
      if (asprintf(&msgBuffer, "replcountdir %s %d", filename, count) < 0) {
	globus_libc_fprintf(stderr, "Out of memory\n");
	return 1;
      }
    }
    else {
      if (asprintf(&msgBuffer, "replcount %s %d", filename, count) < 0) {
	globus_libc_fprintf(stderr, "Out of memory\n");
	return 1;
      }
    }

    /* send message to actually change count */
    if (!sendMessageToMainNode(msgBuffer)) {
      globus_libc_free(msgBuffer);
      globus_libc_fprintf(stderr, "Error sending message to control thread\n");
      return 1;
    }

    globus_libc_free(msgBuffer);
  }

  return 0;
}
