/***********************************************************************
 *
 *   Filename:   srm-transfer-gridftp.c
 *
 *   Authors:    James Perry     (jamesp)   EPCC.
 *
 *   Purpose:    GridFTP protocol transfer module for SRM adaptor
 *
 *   Contents:   GridFTP protocol transfer module for SRM adaptor
 *
 *   Used in:    Control thread, clients
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

#include "srm-transfer-gridftp.h"
#include "gridftp-common.h"

/*
 * This module contains all the functions that would need to be
 * re-implemented in order to support a different transfer protocol for
 * SRM.
 */

/***********************************************************************
 * digs_error_code_t srm_gsiftp_thirdPartyCopy(char *errorMessage,
 *                                             const char *hostname,
 *                                             char *sourceTurl,
 *                                             char *targetTurl)
 * 
 * Copies a file using a 3rd party GridFTP transfer.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     hostname       name of host being accessed                 I
 *     sourceTurl     URL of source file                          I
 *     targetTurl     URL of destination file                     I
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t srm_gsiftp_thirdPartyCopy(char *errorMessage,
					    const char *hostname,
					    char *sourceTurl,
					    char *targetTurl)
{
    digs_error_code_t result = DIGS_SUCCESS;
    globus_result_t err;
    ftpTransaction_t *t;
    globus_object_t *errorObject;
    
    logMessage(DEBUG, "srm_gsiftp_thirdPartyCopy(%s,%s,%s)", hostname,
	       sourceTurl, targetTurl);
    
    acquireTransactionListMutex();
    t = newFtpTransaction("3rd party copy");
    if (!t) {
	releaseTransactionListMutex();
	strcpy(errorMessage, "Error creating new FTP transaction");
	return DIGS_UNKNOWN_ERROR;
    }
    
    err = globus_ftp_client_third_party_transfer(&t->handle, sourceTurl,
						 &t->attr, targetTurl,
						 &t->attr2, NULL,
						 completeCallback, t);
    if (err != GLOBUS_SUCCESS) {
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	
	errorObject = globus_error_get(err);
	return getErrorAndMessageFromGlobus(errorObject, errorMessage);
    }
    
    releaseTransactionListMutex();
    
    /* FIXME: should time-out be longer, or configurable? */
    if (!waitOnFtp(t, 120.0)) {
	strcpy(errorMessage, "3rd party copy timed out");
	return DIGS_NO_RESPONSE;
    }
    
    acquireTransactionListMutex();
    if (!t->succeeded) {
	errorObject = (globus_object_t *)t->error;
	
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	return getErrorAndMessageFromGlobus(errorObject, errorMessage);
    }
    
    destroyFtpTransaction(t);
    releaseTransactionListMutex();
    
    return result;
}

/***********************************************************************
 * digs_error_code_t srm_gsiftp_cancelTransfer(char *errorMessage,
 *                                             int handle)
 * 
 * Cancels a GridFTP put/get transfer that's in progress
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     handle         handle identifying transfer                 I
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t srm_gsiftp_cancelTransfer(char *errorMessage,
					    int handle)
{
    digs_error_code_t result = DIGS_SUCCESS;
    ftpTransaction_t *t;
    char *lockedPath;
    
    logMessage(DEBUG, "srm_gsiftp_cancelTransfer(%d)", handle);
    
    acquireTransactionListMutex();
    t = findTransaction(handle);
    if (!t) {
	releaseTransactionListMutex();
	strcpy(errorMessage,
	       "Invalid handle passed to srm_gsiftp_cancelTransfer");
	return DIGS_UNKNOWN_ERROR;
    }
    
    /* cancel gridftp operation */
    if (globus_ftp_client_abort(&t->handle) != GLOBUS_SUCCESS) {
	logMessage(WARN, "Error aborting FTP operation");
    }
    
    /* this will cause the callback to destroy the FTP transaction */
    t->waiting = 0;
    
    if (!t->writing) {
	/* for a get transfer, remove the downloaded file(s) */
	if (safe_asprintf(&lockedPath, "%s-LOCKED", t->destFilepath) < 0) {
	    errorExit("Out of memory in srm_gsiftp_cancelTransfer");
	}
	
	unlink(lockedPath);
	unlink(t->destFilepath);
	
	globus_libc_free(lockedPath);
    }
    
    releaseTransactionListMutex();
    
    return result;
}


/***********************************************************************
 * digs_error_code_t srm_gsiftp_endTransfer(char *errorMessage,
 *                                          int handle)
 * 
 * Ends a GridFTP put/get transfer. Verifies checksum on file.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     handle         handle identifying transfer                 I
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t srm_gsiftp_endTransfer(char *errorMessage, int handle)
{
    ftpTransaction_t *t;
    digs_error_code_t result = DIGS_SUCCESS;
    globus_object_t *error;
    char *lockedPath;
    char *csum;
    
    logMessage(DEBUG, "srm_gsiftp_endTransfer(%d)", handle);
    
    /* find our transaction */
    acquireTransactionListMutex();
    t = findTransaction(handle);
    if (!t) {
	releaseTransactionListMutex();
	strcpy(errorMessage,
	       "Invalid handle passed to srm_gsiftp_endTransfer");
	return DIGS_UNKNOWN_ERROR;
    }
    
    /* if it failed, return the error */
    if (!t->succeeded) {
	error = (globus_object_t *)t->error;
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	return getErrorAndMessageFromGlobus(error, errorMessage);
    }

    if (t->writing) {
	/* put transfer completed. Checksum the uploaded file */
	releaseTransactionListMutex();
	
	csum = globus_libc_malloc(CHECKSUM_LENGTH+1);
	csum[0] = 0;
	result = srm_gsiftp_checksum(errorMessage, t->destFilepath, &csum);
	if (result == DIGS_SUCCESS) {
	    if (strcmp(csum, t->checksum)) {
		strcpy(errorMessage, "Checksum failure on uploaded file");
		result = DIGS_INVALID_CHECKSUM;
	    }
	}
	globus_libc_free(csum);
	
	acquireTransactionListMutex();
    }
    else {
	/*
	 * get transfer completed. Checksum the downloaded file
	 */
	if (safe_asprintf(&lockedPath, "%s-LOCKED", t->destFilepath) < 0) {
	    errorExit("Out of memory in srm_gsiftp_endTransfer");
	}
	getChecksum(lockedPath, &csum);
	
	/* make sure it matches the remote one */
	if (!strcmp(csum, t->checksum)) {
	    /* success - unlock the file */
	    rename(lockedPath, t->destFilepath);
	}
	else {
	    /* checksum failure - remove the downloaded file */
	    unlink(lockedPath);
	    strcpy(errorMessage, "Checksum failure on downloaded file");
	    result = DIGS_INVALID_CHECKSUM;
	}
	
	globus_libc_free(csum);
	globus_libc_free(lockedPath);
    }
    
    destroyFtpTransaction(t);
    releaseTransactionListMutex();
    
    return result;
}

/***********************************************************************
 * digs_error_code_t
 * srm_gsiftp_monitorTransfer(char *errorMessage,
 *                            int handle,
 *                            digs_transfer_status_t *status,
 *                            int *percentComplete)
 * 
 * Gets the status of a GridFTP put/get transfer
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage      buffer to receive error message            O
 *     handle            handle identifying transfer              I
 *     status            receives transfer status code              O
 *     percentComplete   receives percentage completion             O
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t
srm_gsiftp_monitorTransfer(char *errorMessage, int handle,
			   digs_transfer_status_t *status,
			   int *percentComplete)
{
    ftpTransaction_t *t;
    
    logMessage(DEBUG, "srm_gsiftp_monitorTransfer(%d)", handle);
    
    *status = DIGS_TRANSFER_FAILED;
    *percentComplete = 0;
    
    acquireTransactionListMutex();
    t = findTransaction(handle);
    if (!t) {
	releaseTransactionListMutex();
	strcpy(errorMessage,
	       "Invalid transaction handle passed to srm_gsiftp_monitorTransfer");
	return DIGS_UNKNOWN_ERROR;
    }
    
    if (!t->done) {
	releaseTransactionListMutex();
	*status = DIGS_TRANSFER_IN_PROGRESS;
	return DIGS_SUCCESS;
    }
    
    if (t->succeeded) {
	*status = DIGS_TRANSFER_DONE;
	*percentComplete = 100;
    }
    else {
	*status = DIGS_TRANSFER_FAILED;
	releaseTransactionListMutex();
	return getErrorAndMessageFromGlobus(t->error, errorMessage);
    }
    
    releaseTransactionListMutex();
    return DIGS_SUCCESS;
}


/***********************************************************************
 * digs_error_code_t
 * srm_gsiftp_startPutTransfer(char *errorMessage,
 *                             char *hostname,
 *                             char *turl,
 *                             char *localFile,
 *                             int *handle)
 * 
 * Initiates a GridFTP put transfer
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage      buffer to receive error message            O
 *     hostname          FQDN of host to transfer to              I
 *     turl              transfer URL of remote file              I
 *     localFile         full path to local file                  I
 *     handle            receives handle identifying transfer       O
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t srm_gsiftp_startPutTransfer(char *errorMessage,
					      char *hostname,
					      char *turl,
					      char *localFile,
					      int *handle)
{
    globus_result_t err;
    digs_error_code_t result = DIGS_SUCCESS;
    ftpTransaction_t *t;
    globus_object_t *errorObject;
    char *csum;
    
    logMessage(DEBUG, "srm_gsiftp_startPutTransfer(%s,%s,%s)", hostname,
	       turl, localFile);
    
    *handle = -1;
    
    /* get FTP transaction */
    acquireTransactionListMutex();
    t = newFtpTransaction("write");
    if (!t) {
	releaseTransactionListMutex();
	strcpy(errorMessage, "Error creating new FTP write transaction");
	return DIGS_UNKNOWN_ERROR;
    }
    
    t->buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
    if (!t->buffer) {
	errorExit("Out of memory in srm_gsiftp_startPutTransfer");
    }
    
    /* get local checksum for comparison after transfer */
    getChecksum(localFile, &csum);
    t->checksum = csum; /* will be freed when transaction destroyed */
    
    /*
     * it should be safe to put a URL in here as the low level gridftp
     * code doesn't use it; we want it at the end for the checksum
     */
    t->destFilepath = safe_strdup(turl);
    t->hostname = safe_strdup(hostname);
    
    /* start the put operation */
    err = globus_ftp_client_put(&t->handle, turl, &t->attr, NULL,
				replicateCompleteCallback, t);
    if (err != GLOBUS_SUCCESS) {
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	errorObject = globus_error_get(err);
	return getErrorAndMessageFromGlobus(errorObject, errorMessage);
    }
    
    /* start sending the data */
    if (!startFtpWrite(localFile, t, errorMessage)) {
	releaseTransactionListMutex();
	globus_ftp_client_abort(&t->handle);
	return DIGS_UNKNOWN_ERROR;
    }
    releaseTransactionListMutex();
    *handle = t->id;
    return result;
}

/***********************************************************************
 * digs_error_code_t
 * srm_gsiftp_startGetTransfer(char *errorMessage,
 *                             char *hostname,
 *                             char *turl,
 *                             char *localFile,
 *                             int *handle)
 * 
 * Initiates a GridFTP get transfer
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage      buffer to receive error message            O
 *     hostname          FQDN of host to transfer from            I
 *     turl              transfer URL of remote file              I
 *     localFile         full path to local file                  I
 *     handle            receives handle identifying transfer       O
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t srm_gsiftp_startGetTransfer(char *errorMessage,
					      char *hostname,
					      char *turl,
					      char *localFile,
					      int *handle)
{
    globus_result_t err;
    globus_object_t *errorObject;
    ftpTransaction_t *t;
    char *csum;
    char *lockedPath;
    digs_error_code_t result;
    
    logMessage(DEBUG, "srm_gsiftp_startGetTransfer(%s,%s,%s)", hostname,
	       turl, localFile);
    *handle = -1;
    
    csum = globus_libc_malloc(CHECKSUM_LENGTH+1);
    csum[0] = 0;
    
    /* first get the checksum of the remote file */
    result = srm_gsiftp_checksum(errorMessage, turl, &csum);
    if (result != DIGS_SUCCESS) {
	return result;
    }
    
    /* create the FTP transaction */
    acquireTransactionListMutex();
    t = newFtpTransaction("read");
    if (!t) {
	releaseTransactionListMutex();
	globus_libc_free(csum);
	strcpy(errorMessage, "Creating FTP transaction failed");
	return DIGS_UNKNOWN_ERROR;
    }
    
    /* allocate the buffer */
    t->buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
    if (!t->buffer) {
	errorExit("Out of memory in srm_gsiftp_startGetTransfer");
    }
    
    /* initialise other fields of structure */
    t->destFilepath = safe_strdup(localFile);
    t->hostname = safe_strdup(hostname);
    t->checksum = csum;
    
    /* initiate actual gridftp transfer */
    err = globus_ftp_client_get(&t->handle, turl, &t->attr, NULL,
				replicateCompleteCallback, t);
    if (err != GLOBUS_SUCCESS) {
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	errorObject = globus_error_get(err);
	return getErrorAndMessageFromGlobus(errorObject, errorMessage);
    }
    
    if (safe_asprintf(&lockedPath, "%s-LOCKED", localFile) < 0) {
	errorExit("Out of memory in srm_gsiftp_startGetTransfer");
    }
    
    /* and start reading data */
    if (!startFtpRead(lockedPath, t, errorMessage)) {
	globus_ftp_client_abort(&t->handle);
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	globus_libc_free(lockedPath);
	return DIGS_UNKNOWN_ERROR;
    }
    
    /* return handle to caller */
    *handle = t->id;
    releaseTransactionListMutex();
    globus_libc_free(lockedPath);
    return DIGS_SUCCESS;
}


/***********************************************************************
 * digs_error_code_t srm_gsiftp_checksum(char *errorMessage,
 *                                       char *turl,
 *                                       char **fileChecksum)
 * 
 * Gets an MD5 checksum of a file on a GridFTP server.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     turl           gsiftp URL for the file                     I
 *     fileChecksum   receives pointer to checksum (caller          O
 *                    should allocate buffer before calling)
 * 
 *   Returns: DiGS error code
 ***********************************************************************/
digs_error_code_t srm_gsiftp_checksum(char *errorMessage, char *turl,
				      char **fileChecksum)
{
    ftpTransaction_t *t;
    digs_error_code_t result = DIGS_SUCCESS;
    globus_result_t err;
    globus_object_t *errorObject;
    char *p;
    
    logMessage(DEBUG, "srm_gsiftp_checksum(%s)", turl);
    
    errorMessage[0] = 0;
    
    /* initiate checksum operation */
    acquireTransactionListMutex();
    t = newFtpTransaction("get remote file checksum");
    if (!t) {
	releaseTransactionListMutex();
	return DIGS_UNKNOWN_ERROR;
    }
    
    err = globus_ftp_client_cksm(&t->handle, turl, &t->attr, *fileChecksum,
				 0, -1, "MD5", completeCallback, t);
    if (err != GLOBUS_SUCCESS) {
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	errorObject = globus_error_get(err);
	return getErrorAndMessageFromGlobus(errorObject, errorMessage);
    }
    
    releaseTransactionListMutex();
    
    /* wait for it to complete */
    /* FIXME: should the time-out be variable? */
    if (!waitOnFtp(t, 120.0)) {
	strcpy(errorMessage, "Checksum operation timed out");
	return DIGS_NO_RESPONSE;
    }
    
    acquireTransactionListMutex();
    if (!t->succeeded) {
	errorObject = (globus_object_t *)t->error;
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	return getErrorAndMessageFromGlobus(errorObject, errorMessage);
    }
    
    destroyFtpTransaction(t);
    releaseTransactionListMutex();
    
    /* convert to upper case */
    p = *fileChecksum;
    while (*p) {
	*p = toupper(*p);
	p++;
    }
    
    return result;
}
