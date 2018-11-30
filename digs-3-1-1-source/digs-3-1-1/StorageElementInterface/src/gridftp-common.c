/***********************************************************************
 *
 *   Filename:   gridftp-common.c
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

#include <globus_ftp_client.h>

#include "gridftp-common.h"
#include "misc.h"

/*
 * Protects the FTP transactions list from concurrent access by multiple threads
 */
static globus_mutex_t transactionListLock_;


/***********************************************************************
*   int startupReplicationSystem()
*
*   Starts up the replication system, allocating the data structures
*   and buffers required.
*
*   The replications are now dynamically allocated, not using a static
*   array any more, so the parameter here is meaningless
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
int startupReplicationSystem()
{
    logMessage(DEBUG, "startupReplicationSystem");

    if (globus_mutex_init(&transactionListLock_, NULL))
    {
	logMessage(ERROR, "Unable to initialise FTP transaction list mutex");
	return 0;
    }

    return 1;
}


/***********************************************************************
 *   int releaseTransactionListMutex()
 *
 *   Releases the mutex protecting the list of FTP transactions
 *    
 *   Parameters:                                               [I/O]
 *
 *     None
 *    
 *   Returns: 1 on success, 0 on failure
 ***********************************************************************/
int releaseTransactionListMutex() {
	if (globus_mutex_unlock(&transactionListLock_)) {
		return 0;
	}
	return 1;
}

/***********************************************************************
 *   int acquireTransactionListMutex()
 *
 *   Attempts to acquire the mutex protecting the list of FTP
 *   transactions
 *    
 *   Parameters:                                               [I/O]
 *
 *     None
 *    
 *   Returns: 1 on success, 0 on failure
 ***********************************************************************/
int acquireTransactionListMutex() {
	if (globus_mutex_lock(&transactionListLock_)) {
		return 0;
	}
	return 1;
}

/***********************************************************************
 *   int isTransactionDone(ftpTransaction_t *t)
 *
 *   Thread-safe function for checking whether an FTP transaction has
 *   completed
 *    
 *   Parameters:                                               [I/O]
 *
 *     t      points to the transaction structure               I
 *    
 *   Returns: 1 if transaction has completed (successfully or otherwise)
 *            0 if it is still in progress
 *           -1 if an error occurred
 ***********************************************************************/
int isTransactionDone(ftpTransaction_t *t) {
	int result;

	if (!acquireTransactionListMutex()) {
		return -1;
	}

	result = t->done;

	releaseTransactionListMutex();
	return result;
}

/***********************************************************************
 *   int isTransactionDoneLastCheck(ftpTransaction_t *t)
 *
 *   Atomic function intended to be used to do a final check on the
 *   status of an FTP transaction before it times out. Checks whether
 *   the operation completed. If not, sets its 'waiting' flag to 0. This
 *   means the originating thread has abandoned this transaction so it
 *   is up to the worker thread to destroy it when it finally does end.
 *
 *   Returns without releasing the transaction list mutex!
 *
 *   Parameters:                                               [I/O]
 *
 *     t      points to the transaction structure               I
 *    
 *   Returns: 1 if transaction has completed (successfully or otherwise)
 *            0 if it is still in progress
 *           -1 if an error occurred
 ***********************************************************************/
int isTransactionDoneLastCheck(ftpTransaction_t *t) {
	int result;

	if (!acquireTransactionListMutex()) {
		return -1;
	}

	result = t->done;
	if (!result) {
		t->waiting = 0;
	}

	return result;
}

/*
 * Each transaction is given a unique ID number. This variable holds the next ID
 * number, and is incremented each time one is used
 */
static int transactionId_ = 0;

/*
 * Start of current FTP transactions list
 */
static ftpTransaction_t *transactionListHead_= NULL;

/***********************************************************************
 *   ftpTransaction_t *newFtpTransaction(char *opname)
 *
 *   Creates a new FTP transaction structure, initialises it ready to go,
 *   and links it onto the list. Does not allocate a transfer buffer in
 *   the structure as not all operations need one.
 *
 *   Caller must hold the transaction list mutex!
 *
 *   Parameters:                                               [I/O]
 *
 *     opname   Name of FTP operation, for error reports        I
 *    
 *   Returns: pointer to new transaction, NULL on error
 ***********************************************************************/
ftpTransaction_t *newFtpTransaction(char *opname) {

	char *method_name = "newFtpTransaction";
	ftpTransaction_t *t;
	globus_result_t err;

	globus_ftp_client_handleattr_t handleAttr;

	logMessage(DEBUG, "%s(%s)", method_name, opname);

	/* allocate structure */
	t = globus_libc_malloc(sizeof(ftpTransaction_t));
	if (!t) {
		errorExit("Out of memory in newFtpTransaction");
	}

	/* copy operation name */
	t->opName = safe_strdup(opname);
	if (!t->opName) {
		errorExit("Out of memory in newFtpTransaction");
	}

	/* initialise transaction status */
	t->done = 0;
	t->succeeded = 0;
	t->waiting = 1;
	t->error = NULL;
	t->length = 0;
	t->buffer = NULL;
	t->bigBuffer = NULL;
	t->offset = 0;
	t->writing = 0;
	t->checksum = NULL;
	t->destFilepath = NULL;
	t->hostname = NULL;
	t->readToBuffer = 0;
	t->file = NULL;
	t->id = transactionId_;
	transactionId_++;

	/* initialise globus handle and attributes */
	err = globus_ftp_client_handleattr_init(&handleAttr);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_handleattr_init");
		globus_libc_free(t->opName);
		globus_libc_free(t);
		return NULL;
	}

	/* try to turn on connection caching */
	err = globus_ftp_client_handleattr_set_cache_all(&handleAttr, GLOBUS_TRUE);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_handleattr_set_cache_all");
		globus_libc_free(t->opName);
		globus_libc_free(t);
		return NULL;
	}

	err = globus_ftp_client_handle_init(&t->handle, &handleAttr);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_handle_init");
		globus_libc_free(t->opName);
		globus_libc_free(t);
		return NULL;
	}

	err = globus_ftp_client_operationattr_init(&t->attr);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_operationattr_init");
		globus_ftp_client_handle_destroy(&t->handle);
		globus_libc_free(t->opName);
		globus_libc_free(t);
		return NULL;
	}

	err = globus_ftp_client_operationattr_init(&t->attr2);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_operationattr_init");
		globus_ftp_client_handle_destroy(&t->handle);
		globus_libc_free(t->opName);
		globus_libc_free(t);
		return NULL;
	}

	/* link onto the list */
	t->next = transactionListHead_;
	transactionListHead_ = t;

	return t;
}

/***********************************************************************
 *   int destroyFtpTransaction(ftpTransaction_t *t)
 *
 *   Destroys an FTP transaction, freeing all its storage, destroying the
 *   Globus handles within, and unlinking it from the list
 *
 *   Caller must hold the list's mutex!
 *
 *   Parameters:                                               [I/O]
 *
 *     t    points to the structure to destroy                  I
 *    
 *   Returns: 1 on success, 0 on failure
 ***********************************************************************/
int destroyFtpTransaction(ftpTransaction_t *t) {
	ftpTransaction_t *t2;
	globus_result_t err;

	logMessage(DEBUG, "destroyFtpTransaction id %d", t->id);

	/* unlink transaction from list */
	if (transactionListHead_ == t) {
		transactionListHead_ = t->next;
	} else {
		t2 = transactionListHead_;
		while ((t2->next != NULL) && (t2->next != t)) {
			t2 = t2->next;
		}

		if (t2->next == NULL) {
			/* couldn't find that transaction in the list */
			logMessage(WARN,
					"Tried to destroy FTP transaction not found in list");
			return 0;
		}

		t2->next = t->next;
	}

	/* destroy transaction */
	err = globus_ftp_client_handle_destroy(&t->handle);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_handle_destroy");

		/*
		 * we report an error if the handle destroy failed.
		 * Probably the least destructive option here though
		 * is to leak the memory if that happens.
		 */
		return 0;
	}

	globus_ftp_client_operationattr_destroy(&t->attr);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_operationattr_destroy");
	}
	globus_ftp_client_operationattr_destroy(&t->attr2);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_operationattr_destroy");
	}

	if (t->buffer) {
		globus_libc_free(t->buffer);
	}
	if (t->bigBuffer) {
			globus_libc_free(t->bigBuffer);
	}
	if (t->checksum) {
			globus_libc_free(t->checksum);
	}
	if (t->destFilepath) {
			globus_libc_free(t->destFilepath);
	}
	if (t->hostname) {
			globus_libc_free(t->hostname);
	}
	globus_libc_free(t->opName);
	globus_libc_free(t);

	return 1;
}

/***********************************************************************
 *   void completeCallback(void *data, globus_ftp_client_handle_t *handle,
 *		          globus_object_t *error)
 *    
 *   Should never be called directly. Passed to Globus FTP functions as a
 *   callback to notify this module of an FTP operation finishing
 *    
 *   Parameters:                                               [I/O]
 *
 *    data     Holds the transaction structure pointer.         I
 *    handle   Globus FTP handle of operation which completed   I
 *    error    Error encountered by operation                   I
 *    
 *   Returns: (void)
 ***********************************************************************/
void completeCallback(void *data, globus_ftp_client_handle_t *handle,
		globus_object_t *error) {
	
	ftpTransaction_t *t;
	t = (ftpTransaction_t *)data;

	/* try to get access to list */
	if (acquireTransactionListMutex()) {
		/* flag transaction as done */
		t->done = 1;
		t->error = globus_object_copy(error);
		if (error == GLOBUS_SUCCESS) {
			t->succeeded = 1;
		} else {
			printErrorObject(error, t->opName);
			t->succeeded = 0;
		}

		/* if the initiator is no longer waiting for this transaction, we
		 * must destroy it here */
		if (!t->waiting) {
			destroyFtpTransaction(t);
		}

		releaseTransactionListMutex();
	} else {
		logMessage(WARN,
				"FTP completion callback for %d unable to acquire mutex", t->id);
	}
}

/***********************************************************************
 *   void waitOnFtp(ftpTransaction_t *transaction, float timeOut)
 *    
 *   Waits for a Globus FTP operation to complete (or time out)
 *    
 *   Parameters:                                               [I/O]
 *
 *    transaction   transaction to wait for                     I
 *    timeOut       time out in seconds                         I
 *    
 *   Returns: 1 if the operation completed, 0 if it timed out
 ***********************************************************************/
int waitOnFtp(ftpTransaction_t *transaction, float timeOut) {
	time_t startTime;
	time_t endTime;

	logMessage(DEBUG, "waitOnFtp id %d,%f", transaction->id, timeOut);

	if (timeOut < 0.0) {
		timeOut = 45.0;
	}

	startTime = time(NULL);

	endTime = time(NULL);
	while ((!isTransactionDone(transaction)) && (difftime(endTime, startTime)
			< timeOut)) {
		globus_libc_usleep(1000);
		endTime = time(NULL);
	}

	if (!isTransactionDoneLastCheck(transaction)) {
		logMessage(WARN, "Error in %s: operation timed out",
				transaction->opName);

		/*
		 * Note: in older (2.0) versions of Globus, aborting an existence check
		 * or 'get length' operation would cause an assertion failure in the
		 * Globus code. This doesn't seem to happen any more with 2.4 so I have
		 * removed the unsatisfactory work around
		 */
		if (globus_ftp_client_abort(&transaction->handle) != GLOBUS_SUCCESS) {
			logMessage(WARN, "Error aborting FTP operation %s",
					transaction->opName);
		}

		releaseTransactionListMutex();

		/*
		 * Return 0: we've finished with this transaction but should leave it alone
		 */
		return 0;
	}

	releaseTransactionListMutex();
	/*
	 * Return 1: transaction completed and we should clean up after it
	 */
	return 1;
}

/***********************************************************************
 *   char* getErrorAndMessageFromGlobus(globus_object_t *error, 
 * 				struct digs_result_t *result )
 *
 * 	Get a DiGs result structure from a globus error.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 error 			a globus error to extract the corresponding 
 * 					digs error and description from						I
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 result			the digs_result_t structue indicating success or 	O
 * 					failure and an error description string
 *    
 *   Returns: A DiGs error code 
 ***********************************************************************/
digs_error_code_t getErrorAndMessageFromGlobus(globus_object_t *error,
		char *errorMessage) {

	int errorType;
	char errorDesc[MAX_ERROR_MESSAGE_LENGTH];
	errorDesc[0] = '\0';
	globus_bool_t doGetGlobusErrorDesc= GLOBUS_TRUE;
	digs_error_code_t error_code = DIGS_UNKNOWN_ERROR;

	globus_module_descriptor_t *module;

	/* Just need these methods for */
	/* Find which module caused the error */
	module = globus_error_get_source(error);
	logMessage(DEBUG, "error module: %s", module->module_name);

	/* Get the error code */
	errorType = globus_error_get_type(error);
	logMessage(DEBUG, "errorType: %d", errorType);

	/* globus_error_match searches through causal errors for matches too. */

	/* Unable to connect to the node */
	if (globus_error_match(error, 
	GLOBUS_XIO_MODULE, GLOBUS_XIO_ERROR_WRAPPED)) {
		error_code = DIGS_NO_CONNECTION;
	}
	/* File can't be found on the remote node, 
	 * or the user does not have permission to access the file,
	 * or the remote node isn't running globus,
	 * or if the proxy is not valid  on the remote node 
	 * (has timed out, or grid-mapfile is missing)
	 */
	else if (globus_error_match(error, 
	GLOBUS_FTP_CLIENT_MODULE, GLOBUS_FTP_CLIENT_ERROR_RESPONSE)) {
		error_code = DIGS_UNSPECIFIED_SERVER_ERROR;
	}
	/*   grid-proxy-init hasn't beeen run on this node.*/
	else if (globus_error_match(error, 
	GLOBUS_FTP_CONTROL_MODULE, GLOBUS_FTP_UNKNOWN_REPLY)) {
		error_code = DIGS_NO_USER_PROXY;
		/* globus_error_print_friendly crashes the system when used for this error.*/
		doGetGlobusErrorDesc = GLOBUS_FALSE;
	} else {
		error_code = DIGS_UNKNOWN_ERROR;
	}

	if (doGetGlobusErrorDesc && (globus_error_print_friendly(error)!= NULL)) {
		strncpy(errorMessage, globus_error_print_friendly(error), 
		MAX_ERROR_MESSAGE_LENGTH);
	}

	return error_code;
}

/***********************************************************************
 * void dataCallback(void *userArg, globus_ftp_client_handle_t *handle,
 *		    globus_object_t *error, globus_byte_t *buffer,
 * 		    globus_size_t length, globus_off_t offset,
 *		    globus_bool_t eof)
 *
 *  Callback used by Globus FTP client to inform us that a read/write
 *  operation on the buffer completed.
 *    
 *  Parameters:                                                     [I/O]
 *
 *    userArg  Points to ftpTransaction_t structure                  I
 *    handle   Pointer to the FTP client handle in use               I
 *    error    Error code. GLOBUS_SUCCESS unless something wrong     I
 *    buffer   Pointer to the buffer being used                      I
 *    length   Length of data in the buffer                          I
 *    offset   Offset within the file of this buffer                 I
 *    eof      Set if end of file reached                            I
 *    
 *  Returns: (void)
 ***********************************************************************/
void dataCallback(void *userArg, globus_ftp_client_handle_t *handle,
		globus_object_t *error, globus_byte_t *buffer, globus_size_t length,
		globus_off_t offset, globus_bool_t eof) {
	//releaseTransactionListMutex();

	ftpTransaction_t *t;

	t = (ftpTransaction_t *)userArg;

	acquireTransactionListMutex();

	/* First check for an error from the FTP module */
	if (error != GLOBUS_SUCCESS) {
		printErrorObject(error, "data transfer");
		globus_ftp_client_abort(handle);
		t->succeeded = 0;
		t->error = error;

		releaseTransactionListMutex();
		return;
	}

	/* See if it's a read or a write currently in process */
	if (t->writing) {
		/* It's a write (put). See if it's finished */
		if ((eof) || ((offset+length) >= t->length)) {
			releaseTransactionListMutex();
			return;
		}

		/* Not finished yet. Start the next block */
		if (!startFtpWriteBlock(t)) {
			globus_ftp_client_abort(handle);
			t->succeeded = 0;

			releaseTransactionListMutex();
			return;
		}
	} 
	else if (t->readToBuffer) {
		/* We're reading data to a buffer. Save the latest buffer 
		 * into our big buffer. */
		/* Increase size of big buffer by data length. */
		t->bigBuffer = globus_libc_realloc(t->bigBuffer, (offset+length+1)*sizeof(char));
		if (!t->bigBuffer) {
			errorExit("Out of memory in readToBuffer");
		}
		/* Append data to the big buffer. */
		//strncat(t->bigBuffer, t->buffer, length);
		memcpy(t->bigBuffer+offset, t->buffer, length);
		t->bigBuffer[offset+length] = 0;

		/* See if we've reached the end yet */
		if (eof) {
			releaseTransactionListMutex();
			return;
		}

		/* Not reached the end, so start reading the next block */
		if (!startFtpReadBlock(t)) {
			globus_ftp_client_abort(handle);
			t->succeeded = 0;
		}
	} else {
		/* We're reading data. Save the latest buffer into our file */
		fwrite(t->buffer, 1, length, t->file);

		/* See if we've reached the end yet */
		if (eof) {
			fclose(t->file);
			releaseTransactionListMutex();
			return;
		}

		/* Not reached the end, so start reading the next block */
		if (!startFtpReadBlock(t)) {
			globus_ftp_client_abort(handle);

			t->succeeded = 0;
		}
	}
	releaseTransactionListMutex();
}

/***********************************************************************
 *   int startFtpReadBlock(ftpTransaction_t *t)
 *
 *   Starts reading a new block of data on the given FTP handle
 *
 *   Caller must hold the transaction list mutex!
 *    
 *   Parameters:                                                     [I/O]
 *
 *     t      ftp transaction handle                                  I
 *    
 *   Returns: 1 on success, 0 on error
 ***********************************************************************/
int startFtpReadBlock(ftpTransaction_t *t) {
	globus_result_t err;

	/* Tell Globus to read more data into the buffer */
	err = globus_ftp_client_register_read(&t->handle,
			(globus_byte_t *)t->buffer, FTP_DATA_BUFFER_SIZE, dataCallback, t);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_register_read");
		return 0;
	}

	return 1;
}

/***********************************************************************
 *   int startFtpWriteBlock(ftpTransaction_t *t)
 *
 *   Starts writing a new block of data on the given FTP handle
 *
 *   Caller must hold transaction list mutex!
 *    
 *   Parameters:                                                     [I/O]
 *
 *     t    transaction structure pointer                             I
 *    
 *   Returns: 1 on success, 0 on error
 ***********************************************************************/
int startFtpWriteBlock(ftpTransaction_t *t) {
	int len;
	int reachedEnd;
	globus_result_t err;

	/* See if there's a full buffer's worth still to send */
	if ((t->length - t->offset) >= FTP_DATA_BUFFER_SIZE) {
		len = FTP_DATA_BUFFER_SIZE;
	} else {
		len = (t->length - t->offset);
	}

	/* Fill the buffer with data from the local file */
	fread(t->buffer, 1, len, t->file);

	/* Move up the file */
	t->offset += len;

	/* Check if we've got to the end yet */
	reachedEnd = 0;

	if (t->offset >= t->length) {
		reachedEnd = 1;
		fclose(t->file);
	}

	/* Tell Globus to send this buffer of data */
	err = globus_ftp_client_register_write(&t->handle,
			(globus_byte_t *)t->buffer, len, t->offset-len, reachedEnd,
			dataCallback, t);
	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_register_write");
		return 0;
	}

	return 1;
}
/***********************************************************************
 *   int startFtpWrite(char *filename, ftpTransaction_t *t, char *errorMessage)
 *
 *   Starts off a write (put) operation on the given handle
 *
 *   Caller must hold transaction list mutex!
 *    
 *   Parameters:                                                     [I/O]
 *
 *     filename  Name of local file to transfer                       I
 *     t         transaction structure pointer                        I
 *    
 *   Returns: 1 on success, 0 on error
 ***********************************************************************/
int startFtpWrite(const char *filename, ftpTransaction_t *t,
		char *errorMessage) {
	logMessage(1, "startFtpWrite(%s,%d)", filename, t->id);

	/* Work out how many bytes to transfer */
	t->length = getFileLength(filename);

	/* Open it */
	t->file = fopen(filename, "rb");
	if (!t->file) {
		logMessage(3, "startFtpWrite: unable to open %s for reading", filename);
		strncpy(errorMessage, "Could not open remote file to read from.",
				MAX_ERROR_MESSAGE_LENGTH);
		return 0;
	}

	/* This is a write operation */
	t->writing = 1;
	t->readToBuffer = 0;

	/* So far, transferred 0 bytes */
	t->offset = 0;

	/* Not failed yet */
	t->succeeded = 1;

	/* Send the first block */
	return startFtpWriteBlock(t);
}

/***********************************************************************
 *   void replicateCompleteCallback(void *data, globus_ftp_client_handle_t *handle,
 *	 		           globus_object_t *error)
 *    
 *   Callback function invoked by Globus when a replication operation
 *   completes
 *    
 *   Parameters:                                               [I/O]
 *
 *    data   Pointer to transaction structure of operation      I
 *    handle Pointer to handle of operation                     I
 *    error  Pointer to Globus error object indicating result   I    
 *
 *   Returns: (void)
 ***********************************************************************/
void replicateCompleteCallback(void *data,
		globus_ftp_client_handle_t *handle, globus_object_t *error) {
	ftpTransaction_t *t;

	t = (ftpTransaction_t *)data;
	
	acquireTransactionListMutex();

	/* Save result in case anything else wants it */
	t->done = 1;
	t->succeeded = (error == GLOBUS_SUCCESS);
	
	if (error != GLOBUS_SUCCESS) {
		t->error = globus_object_copy(error);
		if (t->writing) {
			printErrorObject(error, "copy from local");
		} else {
			printErrorObject(error, "copy to local");
		}
	}

	if (!t->waiting) {
		logMessage(3,
				"*** Timed out transaction (%d:%s) finally completed ***",
				t->id, t->opName);
		destroyFtpTransaction(t);
	}

	releaseTransactionListMutex();
}
/***********************************************************************
 *   int waitOnFtpLong(ftpTransaction_t *t, float timeOut)
 *    
 *   FTP wait function used for data transfers (get and put)
 *    
 *   Parameters:                                               [I/O]
 *
 *    t        Transaction structure pointer                    I
 *    timeOut  Time out value in seconds                        I
 *    
 *   Returns: 1 if transaction completed, 0 if it timed out
 ***********************************************************************/
int waitOnFtpLong(ftpTransaction_t *t, float timeOut) {
	time_t startTime;
	time_t endTime;

	logMessage(1, "waitOnFtpLong id %d,%f", t->id, timeOut);

	if (timeOut < 0.0) {
		timeOut = 600.0;
	}

	startTime = time(NULL);

	endTime = time(NULL);
	while ((!isTransactionDone(t)) && (difftime(endTime, startTime) < timeOut)) {
		globus_libc_usleep(1000);
		endTime = time(NULL);
	}

	if (!isTransactionDoneLastCheck(t)) {
		if (t->writing) {
			logMessage(3, "Error in copy from local: operation timed out");
		} else {
			logMessage(3, "Error in copy to local: operation timed out");
		}
		globus_ftp_client_abort(&t->handle);
		releaseTransactionListMutex();
		return 0;
	}
	releaseTransactionListMutex();
	return 1;
}

/***********************************************************************
*   int startFtpRead(char *filename, ftpTransaction_t *t)
*
*   Starts off a read (get) operation on the given handle
*    
*   Caller must hold the transaction list mutex!
*
 *   Parameters:                                                     [I/O]
 *
 *   filename 		Name of local file to transfer into                  I
 *   t         		transaction structure pointer                        I
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			 O
 *    
 *   Returns: 1 on success, 0 on error
***********************************************************************/
int startFtpRead(const char *filename, ftpTransaction_t *t, char *errorMessage)
{
    logMessage(DEBUG, "startFtpRead(%s,%d)", filename, t->id);

    /* Open the file we're storing the data in */
    t->file = fopen(filename, "wb");
    if (!t->file) {
		logMessage(WARN, "startFtpRead: opening file %s for writing failed",
				filename);
		strncpy(errorMessage, "Could not open local file to write to.",
				MAX_ERROR_MESSAGE_LENGTH);
		return 0;
    }

    /* This is not a write operation */
    t->writing = 0;
    t->readToBuffer = 0; /* nor a read to buffer. */

    /* FIXME: I think this is useless */
    t->offset = 0;

    /* Not failed yet */
    t->succeeded = 1;

    /* Request the first block of file */
    return startFtpReadBlock(t);
}

/***********************************************************************
*   int startFtpReadToBuffer( ftpTransaction_t *t)
*
*   Starts off a read to buffer (get) operation on the given handle
*    
*   Caller must hold the transaction list mutex!
*
 *   Parameters:                                                     [I/O]
 *
 *   t         		transaction structure pointer                        I
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			 O
 *    
 *   Returns: 1 on success, 0 on error
***********************************************************************/
int startFtpReadToBuffer( ftpTransaction_t *t)
{
    logMessage(DEBUG, "startFtpReadToBuffer(%s,%d)",t->bigBuffer, t->id);

    /* This is not a write operation */
    t->writing = 0;
    
    t->readToBuffer = 1;

    /* FIXME: I think this is useless */
    t->offset = 0;

    /* Not failed yet */
    t->succeeded = 1;

    /* Request the first block of file */
    return startFtpReadBlock(t);
}

/***********************************************************************
*   ftpTransaction_t *findTransaction(int handle)
*
*   Finds the transaction to which the handle refers
*    
*   Caller must hold the transaction list mutex!
*
*   Parameters:                                                     [I/O]
*
*     handle   the identifier of the transaction                     I
*    
*   Returns: transaction, or NULL if not found
***********************************************************************/
ftpTransaction_t *findTransaction(int handle)
{
  ftpTransaction_t *t;

  t = transactionListHead_;
  while ((t != NULL) && (t->id != handle)) {
    t = t->next;
  }
  return t;
}
