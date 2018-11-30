/***********************************************************************
 *
 *   Filename:   gridftp-common.h
 *
 *   Authors:    James Perry, Eilidh Grant     (jamesp, egrant1)   EPCC.
 *
 *   Purpose:    Low level gridftp code shared between Globus and SRM
 *               SE adaptors
 *
 *   Contents:   Various FTP function wrappers
 *
 *   Used in:    Called by other modules for copying, deleting, getting
 *               information on files and directories
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

#ifndef _GRIDFTP_COMMON_H_
#define _GRIDFTP_COMMON_H_

#include <globus_ftp_client.h>
#include "misc.h"

/*
 * A linked list of these structures represents all the GridFTP transactions
 * currently in progress
 */
typedef struct ftpTransaction_s {
	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t handle;

	/* Globus FTP attributes structure for this transaction */
	globus_ftp_client_operationattr_t attr;

	/* Two attributes structures required for 3rd party operations */
	globus_ftp_client_operationattr_t attr2;

	/* The name of the FTP operation, for error reporting */
	char *opName;

	/* Cleared until transaction completes, then set */
	int done;

	/* Once 'done' is set, this is set if the transaction was a success */
	int succeeded;

	/* The error returned by the transaction */
	globus_object_t *error;

	/*
	 * This is set if the thread which initiated the transaction is still
	 * waiting for the result. If it is, the thread performing the
	 * transaction should leave it in the list and set its 'done' flag
	 * (and possibly 'succeeded' flag) when it finishes.
	 * If the 'waiting' flag is clear, the thread which initiated the
	 * transaction has given up on it as timed out, and the thread
	 * doing the work should free the transaction
	 */
	int waiting;

	/*
	 * Variable which receives the file length if the FTP operation is a
	 * 'get length'
	 */
	globus_off_t length;

	/*
	 * The buffer used to store data being transferred
	 */
	char *buffer;

	/*
	 * The buffer used to collect all of the data that has been transferred
	 */
	char *bigBuffer;
	
	/*
	 * File handle of the file being copied to/from in the local
	 * file system
	 */
	FILE *file;

	/*
	 * Set if the operation is a PUT
	 */
	int writing;
	
	/*
	 * Set file checksum for a transfer operation
	 */
	char *checksum;
	
	/*
	 * Set the destination filepath for a transfer operation
	 */
	char *destFilepath;

	/*
	 * Set the hostname for a transfer operation
	 */
	char *hostname;
	
	/*
	 * Set if the operation is a read to buffer
	 */
	int readToBuffer;

	/*
	 * The offset which has been reached in the transfer, in bytes
	 * from the start of the file
	 */
	globus_off_t offset;

	/*
	 * ID number for transaction
	 */
	int id;

	/* Points to next transaction in linked list */
	struct ftpTransaction_s *next;
} ftpTransaction_t;

#define FTP_DATA_BUFFER_SIZE 1048576

/***********************************************************************
*   int startupReplicationSystem()
*
*   Starts up the replication system, allocating the data structures
*   and buffers required.
*
*   Returns: 1 on success, 0 on error
***********************************************************************/
int startupReplicationSystem();

int releaseTransactionListMutex();
int acquireTransactionListMutex();

int isTransactionDone(ftpTransaction_t *t);
int isTransactionDoneLastCheck(ftpTransaction_t *t);

ftpTransaction_t *newFtpTransaction(char *opname);
int destroyFtpTransaction(ftpTransaction_t *t);
ftpTransaction_t *findTransaction(int handle);

void completeCallback(void *data, globus_ftp_client_handle_t *handle,
		      globus_object_t *error);
void dataCallback(void *userArg, globus_ftp_client_handle_t *handle,
		  globus_object_t *error, globus_byte_t *buffer, globus_size_t length,
		  globus_off_t offset, globus_bool_t eof);
void replicateCompleteCallback(void *data,
			       globus_ftp_client_handle_t *handle, globus_object_t *error);



int waitOnFtp(ftpTransaction_t *transaction, float timeOut);
int waitOnFtpLong(ftpTransaction_t *t, float timeOut);

digs_error_code_t getErrorAndMessageFromGlobus(globus_object_t *error,
					       char *errorMessage);


int startFtpReadBlock(ftpTransaction_t *t);
int startFtpWriteBlock(ftpTransaction_t *t);
int startFtpWrite(const char *filename, ftpTransaction_t *t,
		  char *errorMessage);
int startFtpRead(const char *filename, ftpTransaction_t *t, char *errorMessage);
int startFtpReadToBuffer( ftpTransaction_t *t);

#endif
