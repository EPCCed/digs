/***********************************************************************
 *
 *   Filename:   srm.c
 *
 *   Authors:    James Perry     (jamesp)   EPCC.
 *
 *   Purpose:    The SRM storage element adaptor
 *
 *   Contents:   Implementation of SE adaptor functions for SRM
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

/* delete files from inbox if they're still there after 2 days */
#define INBOX_DELETION_TIME (48.0 * 60.0 * 60.0)

#include "misc.h"
#include "node.h"
#include "srm.h"
#include "srm-transfer-gridftp.h"

#include "soapH.h"
#include "gsi.h"
#include "srmSoapBinding.nsmap"

#include <globus_common.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Information and soap structures for each SRM node used so far
 */
typedef struct srm_node_info_s
{
    char *hostname;
    struct soap soap;
    char *endpoint;
} srm_node_info_t;

static int numSrmNodes_ = 0;
static srm_node_info_t *srmNodes_ = NULL;

/*
 * Information on transfers in progress
 */
enum { DIGS_SRM_GET_TRANSFER, DIGS_SRM_PUT_TRANSFER };
enum { DIGS_SRM_WAITING_FOR_TURL, DIGS_SRM_WAITING_FOR_GRIDFTP,
       DIGS_SRM_FINISHED, DIGS_SRM_ERROR };

typedef struct srm_transfer_s
{
    /* DiGS handle for transfer */
    int handle;

    /* GridFTP ID for transfer */
    int gid;
    
    /* hostname transfer is from/to */
    char *hostname;
    
    /* transfer URL, if we have it */
    char *turl;

    /* local filename transfer is from/to */
    char *localFile;

    /* full path to remote file */
    char *remoteFile;

    /* SRM request token */
    char *token;

    /* get, put */
    int type;

    /* waiting for TURL, waiting for gridftp operation */
    int status;
} srm_transfer_t;

static int numSrmTransfers_ = 0;
static srm_transfer_t *srmTransfers_ = NULL;

/*
 * GridFTP adaptor allocates handles starting from 0, so allocate
 * them independently starting from 1 billion to keep them
 * distinctive
 */
static int nextSrmHandle_ = 1000000000;

/***********************************************************************
 * srm_transfer_t *newSrmTransfer(char *hostname, char *localFile,
 *                                int type)
 * 
 * Creates a new SRM transfer structure and initialises it
 * 
 *   Parameters:                                                 [I/O]
 *
 *     hostname   FQDN of the host being accessed                 I
 *     localFile  full path to local file being transferred       I
 *     type       transfer type (see enum above)                  I
 * 
 *   Returns: pointer to new transfer on success, NULL on error
 ***********************************************************************/
static srm_transfer_t *newSrmTransfer(const char *hostname,
				      const char *localFile,
				      int type)
{
    srm_transfer_t *t;

    logMessage(DEBUG, "newSrmTransfer(%s,%s,%d)", hostname, localFile,
	       type);

    /* allocate new transfer structure */
    numSrmTransfers_++;
    srmTransfers_ = realloc(srmTransfers_,
			    numSrmTransfers_*sizeof(srm_transfer_t));
    t = &srmTransfers_[numSrmTransfers_-1];
  
    /* get a handle */
    t->handle = nextSrmHandle_;
    nextSrmHandle_++;
    
    /* initialise */
    t->hostname = safe_strdup(hostname);
    t->localFile = safe_strdup(localFile);
    t->gid = -1;
    t->remoteFile = NULL;
    t->turl = NULL;
    t->token = NULL;
    t->type = type;
    t->status = DIGS_SRM_WAITING_FOR_TURL;
    
    return t;
}

/***********************************************************************
 * srm_transfer_t *findSrmTransfer(int handle)
 * 
 * Finds an existing SRM transfer given its handle
 * 
 *   Parameters:                                                 [I/O]
 *
 *     handle   handle identifying the transfer                   I
 * 
 *   Returns: pointer to transfer on success, NULL on error
 ***********************************************************************/
static srm_transfer_t *findSrmTransfer(int handle)
{
    srm_transfer_t *t = NULL;
    int i;
    
    logMessage(DEBUG, "findSrmTransfer(%d)", handle);
    
    for (i = 0; i < numSrmTransfers_; i++) {
	if (srmTransfers_[i].handle == handle) {
	    t = &srmTransfers_[i];
	    break;
	}
    }
    return t;
}

/***********************************************************************
 * void destroySrmTransfer(int handle)
 * 
 * Frees an existing SRM transfer given its handle
 * 
 *   Parameters:                                                 [I/O]
 *
 *     handle   handle identifying the transfer                   I
 * 
 *   Returns: (void)
 ***********************************************************************/
static void destroySrmTransfer(int handle)
{
    int i;

    logMessage(DEBUG, "destroySrmTransfer(%d)", handle);
    
    for (i = 0; i < numSrmTransfers_; i++) {
	if (srmTransfers_[i].handle == handle) {
	    break;
	}
    }
    if (i >= numSrmTransfers_) {
	/* didn't find it */
	return;
    }
    
    if (srmTransfers_[i].hostname)
	globus_libc_free(srmTransfers_[i].hostname);
    if (srmTransfers_[i].localFile)
	globus_libc_free(srmTransfers_[i].localFile);
    if (srmTransfers_[i].turl)
	globus_libc_free(srmTransfers_[i].turl);
    if (srmTransfers_[i].token)
	globus_libc_free(srmTransfers_[i].token);
    if (srmTransfers_[i].remoteFile)
	globus_libc_free(srmTransfers_[i].remoteFile);
    
    numSrmTransfers_--;
    for (; i < numSrmTransfers_; i++) {
	srmTransfers_[i] = srmTransfers_[i+1];
    }
}

/***********************************************************************
 *   int srmInit(char *hostname, struct soap **soap, char **endpoint)
 * 
 *     Prepares to access the SRM server on the specified host. Caller
 *     should not free anything returned from this function, but should
 *     call srmDone when finished with it.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     hostname   FQDN of the host being accessed                 I
 *     soap       receives soap structure for calling SRM server    O
 *     endpoint   receives endpoint address for SRM server          O
 * 
 *   Returns: 1 on success, 0 on error
 ***********************************************************************/
static int srmInit(const char *hostname, struct soap **soap,
		   char **endpoint)
{
    int i;
    
    logMessage(DEBUG, "srmInit(%s)", hostname);
    
    /* look for structure, see if this host is already initialised */
    for (i = 0; i < numSrmNodes_; i++) {
	if (!strcmp(hostname, srmNodes_[i].hostname)) {
	    /* found it */
	    *soap = &srmNodes_[i].soap;
	    *endpoint = srmNodes_[i].endpoint;
	    return 1;
	}
    }
    
    /* not found, need to initialise for this node */
    *soap = NULL;
    *endpoint = NULL;
    
    numSrmNodes_++;
    srmNodes_ = globus_libc_realloc(srmNodes_,
				    numSrmNodes_ *
				    sizeof(srm_node_info_t));
    if (!srmNodes_) {
	logMessage(ERROR, "Out of memory in srmInit");
	return 0;
    }
    
    srmNodes_[numSrmNodes_ - 1].hostname = safe_strdup(hostname);
    if (!srmNodes_[numSrmNodes_ - 1].hostname) {
	logMessage(ERROR, "Out of memory in srmInit");
	return 0;
    }
    
    /* get SRM node info from DiGS config */
    srmNodes_[numSrmNodes_ - 1].endpoint = getNodeProperty(hostname,
							   "endpoint");
    if (!srmNodes_[numSrmNodes_ - 1].endpoint) {
	logMessage(ERROR, "SRM node %s has no endpoint property",
		   hostname);
	return 0;
    }
    
    /* initialise GSOAP */
    soap_init(&srmNodes_[numSrmNodes_ - 1].soap);
    
    /* initialise GSI plugin */
    if (soap_register_plugin(&srmNodes_[numSrmNodes_ - 1].soap,
			     globus_gsi)) {
	logMessage(ERROR, "Error registering GSOAP GSI plugin");
	return 0;
    }
    
    if (gsi_acquire_credential(&srmNodes_[numSrmNodes_ - 1].soap)
	< 0) {
	logMessage(ERROR, "Error acquiring credential in GSI plugin");
	return 0;
    }
    
    *soap = &srmNodes_[numSrmNodes_ - 1].soap;
    *endpoint = srmNodes_[numSrmNodes_ - 1].endpoint;
    
    return 1;
}

/***********************************************************************
 *   int srmDone(char *hostname, struct soap *soap)
 * 
 *   Cleans up after access to SRM server. After this function is
 *   called, any results from the last SOAP call will no longer be valid
 *   so duplicate them elsewhere first if they are needed.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     hostname   FQDN of the host being accessed                 I
 *     soap       soap structure for calling SRM server           I
 * 
 *   Returns: (void)
 ***********************************************************************/
static void srmDone(const char *hostname, struct soap *soap)
{
    logMessage(DEBUG, "srmDone(%s)", hostname);
    soap_destroy(soap);
    soap_end(soap);
}

/***********************************************************************
 *   char *constructSRMPath(char *hostname, char *filePath)
 * 
 *   Construct the path necessary for accessing a file on an SRM server
 * 
 *   Parameters:                                                 [I/O]
 *
 *     hostname   FQDN of the host being accessed                 I
 *     filePath   full path to file being accessed                I
 * 
 *   Returns: SRM path, caller should free. NULL on error
 ***********************************************************************/
static char *constructSRMPath(const char *hostname, const char *filePath)
{
    char *result;
    if (safe_asprintf(&result, "srm://%s%s", hostname, filePath) < 0) {
	logMessage(ERROR, "Out of memory in constructSRMPath");
	return NULL;
    }
    return result;
}

/*
 * Text versions of the SRM status codes
 */
static const char *srmStatusMessages_[] =
{
    "success", /* 0 */
    "failure",
    "authentication failure",
    "authorisation failure",
    "invalid request",
    "invalid path", /* 5 */
    "file lifetime expired",
    "space lifetime expired",
    "exceed allocation",
    "no user space",
    "no free space", /* 10 */
    "duplication error",
    "non-empty directory",
    "too many results",
    "internal error",
    "fatal internal error", /* 15 */
    "not supported",
    "request queued", 
    "in progress",
    "request suspended",
    "aborted", /* 20 */
    "released",
    "file pinned",
    "file in cache",
    "space available",
    "lower space granted", /* 25 */
    "done",
    "partial success",
    "request timed out",
    "last copy",
    "file busy", /* 30 */
    "file lost",
    "file unavailable",
    "custom status" /* 33 */
};

/*
 * DiGS error code translations of the SRM status codes
 */
static const digs_error_code_t srmToDigsErrorMap_[] =
{
    DIGS_SUCCESS, /* 0 */
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR, /* 5 */
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR, /* 10 */
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR, /* 15 */
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_SUCCESS,
    DIGS_SUCCESS,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR, /* 20 */
    DIGS_SUCCESS,
    DIGS_SUCCESS,
    DIGS_SUCCESS,
    DIGS_SUCCESS,
    DIGS_SUCCESS, /* 25 */
    DIGS_SUCCESS,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR, /* 30 */
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR,
    DIGS_UNSPECIFIED_SERVER_ERROR /* 33 */
};

/***********************************************************************
 *   digs_error_code_t processSrmError(char *errorMessage,
 *                                     struct ns1__TReturnStatus *status)
 * 
 *   Translates an SRM error status to a DiGS error code and a message
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     status         SRM status                                  I
 * 
 *   Returns: DiGS error code corresponding to SRM error
 ***********************************************************************/
static
digs_error_code_t processSrmError(char *errorMessage,
				  struct ns1__TReturnStatus *status)
{
    digs_error_code_t result;
    int code = status->statusCode;

    if ((code > 33) || (code < 0)) {
	code = 33;
    }
    
    /*
     * translate SRM status code to DiGS error code
     */
    result = srmToDigsErrorMap_[code];
    if (status->explanation) {
	snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "SRM error %d: %s",
		 status->statusCode, status->explanation);
    }
    else {
	snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "SRM error %d: %s",
		 status->statusCode, srmStatusMessages_[code]);
    }
    
    logMessage(WARN, "%s", errorMessage);
    return result;
}

/*
 * SRM status codes that probably don't mean failure:
 * 0:  success
 * 17: queued
 * 18: in progress
 * 21: file in cache
 * 22: file pinned
 * 24: space available
 * 25: lower space granted
 */

/***********************************************************************
 * digs_error_code_t checkPutRequest(char *errorMessage,
 *                                   struct soap *soap,
 *                                   char *endpoint,
 *                                   char *token,
 *                                   char **TURL);
 * 
 * Checks the status of an SRM put request. If the request completed
 * successfully, returns DIGS_SUCCESS and sets TURL to the transport
 * URL. If still pending, returns DIGS_SUCCESS and sets TURL to NULL.
 * If it failed, returns an error code.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     soap           soap structure for contacting server        I
 *     endpoint       SRM endpoint URL                            I
 *     token          token identifying request                   I
 *     TURL           receives transport URL if available           O
 *                    (caller should free)
 * 
 *   Returns: DiGS error code corresponding to SRM error
 ***********************************************************************/
static digs_error_code_t checkPutRequest(char *errorMessage,
					 struct soap *soap,
					 char *endpoint,
					 char *token,
					 char **TURL)
{
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmStatusOfPutRequestRequest req;
    struct ns1__srmStatusOfPutRequestResponse_ resp;
    struct ns1__TPutRequestFileStatus *status;
    
    logMessage(DEBUG, "checkPutRequest(%s,%s)", endpoint, token);
    
    *TURL = NULL;
    
    req.requestToken = token;
    req.authorizationID = NULL;
    req.arrayOfTargetSURLs = NULL;
    
    if (soap_call_ns1__srmStatusOfPutRequest(soap, endpoint,
					     "StatusOfPutRequest", &req,
					     &resp) == SOAP_OK) {
	switch (resp.srmStatusOfPutRequestResponse->returnStatus->statusCode) {
	case 0:
	case 17:
	case 18:
	case 21:
	case 22:
	case 24:
	case 25:
	{
	    status =
		&resp.srmStatusOfPutRequestResponse->arrayOfFileStatuses->statusArray[0];
	    switch (status->status->statusCode) {
	    case 0:
	    case 17:
	    case 18:
	    case 21:
	    case 22:
	    case 24:
	    case 25:
		if (status->transferURL) {
		    *TURL = safe_strdup(status->transferURL);
		}
		break;
	    default:
		result = processSrmError(errorMessage, status->status);
		break;
	    }
	}
	break;
	default:
	    /* request for status failed */
	    result = 
		processSrmError(errorMessage,
				resp.srmStatusOfPutRequestResponse->returnStatus);
	    break;
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    return result;
}

/***********************************************************************
 * digs_error_code_t checkGetRequest(char *errorMessage,
 *                                   struct soap *soap,
 *                                   char *endpoint,
 *                                   char *token,
 *                                   char **TURL);
 * 
 * Checks the status of an SRM get request. If the request completed
 * successfully, returns DIGS_SUCCESS and sets TURL to the transport
 * URL. If still pending, returns DIGS_SUCCESS and sets TURL to NULL.
 * If it failed, returns an error code.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     soap           soap structure for contacting server        I
 *     endpoint       SRM endpoint URL                            I
 *     token          token identifying request                   I
 *     TURL           receives transport URL if available           O
 *                    (caller should free)
 * 
 *   Returns: DiGS error code corresponding to SRM error
 ***********************************************************************/
static digs_error_code_t checkGetRequest(char *errorMessage,
					 struct soap *soap,
					 char *endpoint,
					 char *token,
					 char **TURL)
{
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmStatusOfGetRequestRequest req;
    struct ns1__srmStatusOfGetRequestResponse_ resp;
    struct ns1__TGetRequestFileStatus *status;
    
    logMessage(DEBUG, "checkGetRequest(%s,%s)", endpoint, token);
    
    *TURL = NULL;
    
    req.requestToken = token;
    req.authorizationID = NULL;
    req.arrayOfSourceSURLs = NULL;
    
    if (soap_call_ns1__srmStatusOfGetRequest(soap, endpoint, "StatusOfGetRequest", &req, &resp)
	== SOAP_OK) {
	switch (resp.srmStatusOfGetRequestResponse->returnStatus->statusCode) {
	case 0:
	case 17:
	case 18:
	case 21:
	case 22:
	{
	    status = &resp.srmStatusOfGetRequestResponse->arrayOfFileStatuses->statusArray[0];
	    switch (status->status->statusCode) {
	    case 0:
	    case 17:
	    case 18:
	    case 21:
	    case 22:
		if (status->transferURL) {
		    *TURL = safe_strdup(status->transferURL);
		}
		break;
	    default:
		result = processSrmError(errorMessage, status->status);
		break;
	    }
	}
	break;
	default:
	    /* request for status failed */
	    result = processSrmError(errorMessage, resp.srmStatusOfGetRequestResponse->returnStatus);
	    break;
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    return result;
}


/***********************************************************************
 * digs_error_code_t initiatePutRequest(char *errorMessage,
 *                                      struct soap *soap,
 *                                      char *endpoint,
 *                                      char *path,
 *                                      char **token,
 *                                      char **protocol,
 *                                      ULONG64 size);
 * 
 * Initiates an SRM put request.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     soap           soap structure for contacting server        I
 *     endpoint       SRM endpoint URL                            I
 *     path           URL to destination                          I
 *     token          receives token identifying request            O
 *                    (caller should free)
 *     protocol       receives protocol identifier                  O
 *                    (caller should free)
 *     size           size of file to be put                      I
 * 
 *   Returns: DiGS error code corresponding to SRM error
 ***********************************************************************/
static digs_error_code_t initiatePutRequest(char *errorMessage,
					    struct soap *soap,
					    char *endpoint,
					    char *path,
					    char **token,
					    char **protocol,
					    ULONG64 size)
{
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmPrepareToPutRequest req;
    struct ns1__srmPrepareToPutResponse_ resp;
    struct ns1__ArrayOfTPutFileRequest reqs;
    struct ns1__TPutFileRequest pfr;
    
    logMessage(DEBUG, "initiatePutRequest(%s,%s,%d)", endpoint, path,
	       (int)size);
    
    req.authorizationID = NULL;
    req.arrayOfFileRequests = &reqs;
    req.userRequestDescription = NULL;
    req.overwriteOption = NULL;
    req.storageSystemInfo = NULL;
    req.desiredTotalRequestTime = NULL;
    req.desiredPinLifeTime = NULL;
    req.desiredFileLifeTime = NULL;
    req.desiredFileStorageType = NULL;
    req.targetSpaceToken = NULL;
    req.targetFileRetentionPolicyInfo = NULL;
    req.transferParameters = NULL;
    
    reqs.__sizerequestArray = 1;
    reqs.requestArray = &pfr;
    
    pfr.targetSURL = path;
    pfr.expectedFileSize = &size;
    
    if (soap_call_ns1__srmPrepareToPut(soap, endpoint,
				       "PrepareToPut", &req, &resp)
	== SOAP_OK) {
	/* check for errors here - 17 means request queued */
	switch (resp.srmPrepareToPutResponse->returnStatus->statusCode) {
	case 0:
	case 17:
	case 18:
	case 21:
	case 22:
	case 24:
	case 25:
	    /* success */
	    *token = safe_strdup(resp.srmPrepareToPutResponse->requestToken);
	    *protocol = safe_strdup("gsiftp");
	    break;
	default:
	    /* error */
	    result = processSrmError(errorMessage,
				     resp.srmPrepareToPutResponse->returnStatus);
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    return result;
}

/***********************************************************************
 * digs_error_code_t initiateGetRequest(char *errorMessage,
 *                                      struct soap *soap,
 *                                      char *endpoint,
 *                                      char *path,
 *                                      char **token,
 *                                      char **protocol);
 * 
 * Initiates an SRM get request.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *     soap           soap structure for contacting server        I
 *     endpoint       SRM endpoint URL                            I
 *     path           URL to source file                          I
 *     token          receives token identifying request            O
 *                    (caller should free)
 *     protocol       receives protocol identifier                  O
 *                    (caller should free)
 * 
 *   Returns: DiGS error code corresponding to SRM error
 ***********************************************************************/
static digs_error_code_t initiateGetRequest(char *errorMessage,
					    struct soap *soap,
					    char *endpoint,
					    char *path,
					    char **token,
					    char **protocol)
{
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmPrepareToGetRequest req;
    struct ns1__srmPrepareToGetResponse_ resp;
    struct ns1__ArrayOfTGetFileRequest reqs;
    struct ns1__TGetFileRequest gfr;
    struct ns1__TTransferParameters tp;
    struct ns1__ArrayOfString protos;
    struct ns1__TDirOption diropt;
    char *supportedProto = "gsiftp";
    
    logMessage(DEBUG, "initiateGetRequest(%s,%s)", endpoint, path);
    
    *token = NULL;
    *protocol = NULL;
    
    diropt.isSourceADirectory = xsd__boolean__false_;
    diropt.allLevelRecursive = NULL;
    diropt.numOfLevels = NULL;
    gfr.sourceSURL = path;
    gfr.dirOption = &diropt;
    
    reqs.__sizerequestArray = 1;
    reqs.requestArray = &gfr;
    
    protos.__sizestringArray = 1;
    protos.stringArray = &supportedProto;
    
    tp.accessPattern = NULL;
    tp.connectionType = NULL;
    tp.arrayOfClientNetworks = NULL;
    tp.arrayOfTransferProtocols = &protos;
    
    req.authorizationID = NULL;
    req.arrayOfFileRequests = &reqs;
    req.userRequestDescription = NULL;
    req.storageSystemInfo = NULL;
    req.desiredFileStorageType = NULL;
    req.desiredTotalRequestTime = NULL;
    req.desiredPinLifeTime = NULL;
    req.targetSpaceToken = NULL;
    req.targetFileRetentionPolicyInfo = NULL;
    req.transferParameters = &tp;
    
    if (soap_call_ns1__srmPrepareToGet(soap, endpoint, "PrepareToGet", &req, &resp)
	== SOAP_OK) {
	/* check for errors here - 17 means request queued */
	switch (resp.srmPrepareToGetResponse->returnStatus->statusCode) {
	case 0:
	case 17:
	case 18:
	case 21:
	case 22:
	    /* success */
	    *token = safe_strdup(resp.srmPrepareToGetResponse->requestToken);
	    *protocol = safe_strdup("gsiftp");
	    break;
	default:
	    /* error */
	    result = processSrmError(errorMessage, resp.srmPrepareToGetResponse->returnStatus);
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    return result;
}
					    

/***********************************************************************
 * digs_error_code_t getGroupInfo(char *errorMessage,
 *                                struct soap *soap, char *endpoint,
 *                                char *path, char **group,
 *                                enum ns1__TPermissionMode *perms)
 * 
 * Gets the group of a file and its permissions from the SRM server.
 * This attempts to work out which group should be considered the
 * correct one as there may be multiple ACLs. If only one group has a
 * non-zero permission entry, that one is returned. Otherwise the last
 * group is returned.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     soap           SOAP structure for contacting server        I
 *     endpoint       SRM server endpoint URL                     I
 *     path           URL to the file                             I
 *     group          receives group name (caller should free)      O
 *     perms          receives group permission value               O
 * 
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
static digs_error_code_t getGroupInfo(char *errorMessage,
				      struct soap *soap, char *endpoint,
				      char *path, char **group,
				      enum ns1__TPermissionMode *perms)
{
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmGetPermissionRequest req;
    struct ns1__srmGetPermissionResponse_ resp;
    struct ns1__ArrayOfAnyURI surls;
    struct ns1__TPermissionReturn *perm;
    int i;
    int grp;
    
    logMessage(DEBUG, "getGroupInfo(%s)", path);
    
    *group = NULL;
    *perms = 0;
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.arrayOfSURLs = &surls;
    surls.__sizeurlArray = 1;
    surls.urlArray = &path;
    
    if (soap_call_ns1__srmGetPermission(soap, endpoint, "getPermission",
					&req, &resp) == SOAP_OK) {
	if (resp.srmGetPermissionResponse->returnStatus->statusCode == 0) {
	    perm = resp.srmGetPermissionResponse->arrayOfPermissionReturns->permissionArray;
	    if ((perm) && (perm->arrayOfGroupPermissions)) {
		/* We have the group permissions now. Find the right one */
		grp = -1;
		
		/*
		 * If there's only one entry, return that
		 */
		if (perm->arrayOfGroupPermissions->__sizegroupPermissionArray == 1) {
		    grp = 0;
		}
		else {
		    /* look for the last one that has non-zero mode */
		    for (i = 0; i < perm->arrayOfGroupPermissions->__sizegroupPermissionArray; i++) {
			if (perm->arrayOfGroupPermissions->groupPermissionArray[i].mode) {
			    grp = i;
			}
		    }
		    
		    /* they were all zero, return the last one */
		    if (grp < 0) {
			grp = perm->arrayOfGroupPermissions->__sizegroupPermissionArray - 1;
		    }
		}
		
		/* return its name and mode */
		*group = 
		    safe_strdup(perm->arrayOfGroupPermissions->groupPermissionArray[grp].groupID);
		*perms =
		    perm->arrayOfGroupPermissions->groupPermissionArray[grp].mode;
	    }
	    else {
		strcpy(errorMessage, "No group permissions returned");
		result = DIGS_UNKNOWN_ERROR;
	    }
	}
	else {
	    result =
		processSrmError(errorMessage,
				resp.srmGetPermissionResponse->returnStatus);
	}
    }
    else {
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    return result;
}

/***********************************************************************
 * digs_error_code_t modifyGroupACL(char *errorMessage,
 *                                  struct soap *soap, char *endpoint,
 *                                  char *path, char *group,
 *                                  enum ns1__TPermissionMode perms,
 *                                  enum ns1__TPermissionType action)
 * 
 * Changes group permissions for a file. Can add a new group ACL,
 * modify an existing one or remove one.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     soap           SOAP structure for contacting server        I
 *     endpoint       SRM server endpoint URL                     I
 *     path           URL to the file                             I
 *     group          group to modify                             I
 *     perms          permission value for group (unused for      I
 *                    ACL removals)
 *     action         whether to add, modify or remove the ACL    I
 * 
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
static
digs_error_code_t modifyGroupACL(char *errorMessage,
				 struct soap *soap,
				 char *endpoint,
				 char *path, const char *group,
				 enum ns1__TPermissionMode perms,
				 enum ns1__TPermissionType action)
{
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmSetPermissionRequest req;
    struct ns1__srmSetPermissionResponse_ resp;
    
    struct ns1__ArrayOfTGroupPermission gps;
    struct ns1__TGroupPermission gp;
    
    logMessage(DEBUG, "modifyGroupACL(%s,%s,%d,%d)", path, group, perms,
	       action);
    
    req.authorizationID = NULL;
    req.SURL = path;
    req.permissionType = action;
    req.ownerPermission = NULL;
    req.arrayOfUserPermissions = NULL;
    req.arrayOfGroupPermissions = &gps;
    req.otherPermission = NULL;
    req.storageSystemInfo = NULL;
    
    gps.__sizegroupPermissionArray = 1;
    gps.groupPermissionArray = &gp;
    
    gp.groupID = (char *)group;
    gp.mode = perms;
    
    if (soap_call_ns1__srmSetPermission(soap, endpoint, "setPermission",
					&req, &resp) == SOAP_OK) {
	if (resp.srmSetPermissionResponse->returnStatus->statusCode != 0) {
	    result =
		processSrmError(errorMessage,
				resp.srmSetPermissionResponse->returnStatus);
	}
    }
    else {
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }

    return result;
}


/***********************************************************************
 *   digs_error_code_t digs_getLength_srm(char *errorMessage, 
 * 		                          const char *filePath,
 *                                        const char *hostname,
 *                                        long long int *fileLength)
 * 
 * 	Gets a size in bytes of a file on a node.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     filePath       full path to the file                       I
 *     hostname       the FQDN of the host to contact             I
 *     fileLength     Size of file, -1 on error                     O
 * 
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getLength_srm(char *errorMessage,
				     const char *filePath,
				     const char *hostname,
				     long long int *fileLength)
{
    struct soap *soap;
    char *endpoint;
    
    digs_error_code_t result = DIGS_SUCCESS;
    
    char *paths[2] = { "\0", "\0\0" };
    
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__true_;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_getLength_srm(%s,%s)", filePath, hostname);
    
    *fileLength = -1LL;
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	/*
	 * for attribute querying we allow status code 1 - it will be
	 * returned if we call srmLs on a directory with no execute
	 * permissions
	 */
	if (resp.srmLsResponse->returnStatus->statusCode < 2) {
	    /* check all needed elements were returned */
	    if ((!resp.srmLsResponse->details) ||
		(!resp.srmLsResponse->details->__sizepathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].type) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].size)) {
		result = DIGS_UNSPECIFIED_SERVER_ERROR;
		sprintf(errorMessage,
			"Required element missing from SRM server response");
	    }
	    else {
		if ((*resp.srmLsResponse->details->pathDetailArray[0].type) == 1) {
		    result = DIGS_FILE_IS_DIR;
		}
		else {
		    *fileLength = (long long int)
			*resp.srmLsResponse->details->pathDetailArray[0].size;
		}
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }

    globus_libc_free(paths[0]);
    
    srmDone(hostname, soap);

    return result;
}

/***********************************************************************
 * digs_error_code_t
 * digs_getChecksum_srm (char *errorMessage, 
 *                       const char *filePath,
 *                       const char *hostname,
 *                       char **fileChecksum,
 *                       digs_checksum_type_t checksumType)
 * 
 * Gets a 32 character checksum of a file on a node using the defined 
 * checksum type.
 * Hex digits will be returned in uppercase.
 *
 * Returned checksum is stored in variable fileChecksum.
 * Note that fileChecksum should be freed 
 * accordingly to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                               	 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     filePath       full path to the file                       I
 *     hostname       the FQDN of the host to contact             I
 *     fileChecksum   the checksum of the file                      O
 *     checksumType   the type of checksum to retrieve            I
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getChecksum_srm(char *errorMessage,
				       const char *filePath,
				       const char *hostname,
				       char **fileChecksum,
				       digs_checksum_type_t checksumType)
{
    struct soap *soap;
    char *endpoint;
    char *path;
    digs_error_code_t result = DIGS_SUCCESS;
    char *token;
    char *protocol;
    char *turl = NULL;
    int isdir = 0;
    
    logMessage(DEBUG, "digs_getChecksum_srm(%s,%s,%d)", filePath, hostname,
	       checksumType);
    
    *fileChecksum = globus_libc_malloc(CHECKSUM_LENGTH+1);
    *fileChecksum[0] = 0;
    errorMessage[0] = 0;
    
    /* test checksum type */
    if (checksumType != DIGS_MD5_CHECKSUM) {
	strcpy(errorMessage, "Only MD5 checksum algorithm is supported");
	return DIGS_UNSUPPORTED_CHECKSUM_TYPE;
    }
    
    /* check if file is directory */
    result = digs_isDirectory_srm(errorMessage, filePath, hostname,
				  &isdir);
    if (isdir) {
	strcpy(errorMessage, "Expected file but found directory");
	return DIGS_FILE_IS_DIR;
    }
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    path = constructSRMPath(hostname, filePath);
    
    /* first get a gsiftp transfer URL for the file */
    result = initiateGetRequest(errorMessage, soap, endpoint, path,
				&token, &protocol);
    if (result == DIGS_SUCCESS) {
	
	/* wait for transfer URL to become available */
	while ((result == DIGS_SUCCESS) && (turl == NULL)) {
	    result = checkGetRequest(errorMessage, soap, endpoint, token,
				     &turl);
	    globus_libc_usleep(10000);
	}
	
	globus_libc_free(protocol);
	globus_libc_free(token);
    }
    
    globus_libc_free(path);
    srmDone(hostname, soap);
    
    if (turl != NULL) {
	/* if we got the TURL successfully, actually do the checksum */
	result = srm_gsiftp_checksum(errorMessage, turl, fileChecksum);
	
	globus_libc_free(turl);
    }
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_isDirectory_srm (char *errorMessage,
 *                                         const char *filePath,
 *                                         const char *hostname,
 *                                         int *isDirectory)
 * 
 * Checks if a file on a node is a directory.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     filePath       full path to the file                       I
 *     hostname       the FQDN of the host to contact             I
 *     isDirectory    1 if the path is to a directory, else 0       O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_isDirectory_srm(char *errorMessage,
				       const char *filePath,
				       const char *hostname,
				       int *isDirectory)
{
    struct soap *soap;
    char *endpoint;
  
    digs_error_code_t result = DIGS_SUCCESS;

    char *paths[2] = { "\0", "\0\0" };

    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__true_;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_isDirectory_srm(%s,%s)", filePath, hostname);
    
    *isDirectory = 0;
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	/*
	 * for attribute querying we allow status code 1 - it will be
	 * returned if we call srmLs on a directory with no execute
	 * permissions
	 */
	if (resp.srmLsResponse->returnStatus->statusCode < 2) {
	    /* check all needed elements were returned */
	    if ((!resp.srmLsResponse->details) ||
		(!resp.srmLsResponse->details->__sizepathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].type)) {
		result = DIGS_UNSPECIFIED_SERVER_ERROR;
		sprintf(errorMessage,
			"Required element missing from SRM server response");
	    }
	    else {
		switch (*resp.srmLsResponse->details->pathDetailArray[0].type) {
		case 0: /* file */
		case 2: /* link */
		    *isDirectory = 0;
		    break;
		case 1: /* directory */
		    *isDirectory = 1;
		    break;
		}
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
	
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(paths[0]);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_doesExist_srm (char *errorMessage,
 *                                       const char *filePath,
 *                                       const char *hostname,
 *                                       int *doesExist)
 * 
 * Checks if a file/directory exists on a node.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     filePath       full path to the file                       I
 *     hostname       the FQDN of the host to contact             I
 *     doesExist      receives 1 if the file exists, 0 if not       O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_doesExist_srm(char *errorMessage,
				     const char *filePath,
				     const char *hostname,
				     int *doesExist)
{
    struct soap *soap;
    char *endpoint;
    
    digs_error_code_t result = DIGS_SUCCESS;
    
    char *paths[2] = { "\0", "\0\0" };
    
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__true_;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_doesExist_srm(%s,%s)", filePath, hostname);
    
    *doesExist = 0;
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	if (resp.srmLsResponse->returnStatus->statusCode == 0) {
	    /* File exists */
	    *doesExist = 1;
	}
	else if (resp.srmLsResponse->returnStatus->statusCode == 1) {
	    /*
	     * File doesn't exist. Globus treats a file not existing as
	     * an error even if all we did was ask whether it exists or
	     * not. To avoid confusing the calling code, do the same here.
	     */
	    result = DIGS_UNSPECIFIED_SERVER_ERROR;
	    *doesExist = 0;
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(paths[0]);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_ping_srm(char *errorMessage,
 *                                 const char * hostname);
 * 
 * Pings node and checks that it has a data directory.
 * 
 *   Parameters:                                                 [I/O]
 *
 *     errorMessage   buffer to receive error message               O
 *                    (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     hostname       the FQDN of the host to contact             I
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_ping_srm(char *errorMessage, const char *hostname)
{
    struct ns1__srmPingRequest req;
    struct ns1__srmPingResponse_ resp;
    digs_error_code_t result = DIGS_SUCCESS;
    
    struct soap *soap;
    char *endpoint;
    
    logMessage(DEBUG, "digs_ping_srm(%s)", hostname);
    
    errorMessage[0] = 0;
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    req.authorizationID = NULL;
    if (soap_call_ns1__srmPing(soap, endpoint, "Ping", &req, &resp)
	!= SOAP_OK) {
	result = DIGS_NO_SERVICE;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    srmDone(hostname, soap);
    
    return result;
}

/***********************************************************************
 *   void digs_free_string_srm(char **string) 
 *
 * 	 Free string
 * 
 *   Parameters:                                               	 [I/O]
 *
 *     string    the string to be freed                           I
 ***********************************************************************/
void digs_free_string_srm(char **string)
{
    globus_libc_free(*string);
    *string = NULL;
}

/***********************************************************************
 *  digs_error_code_t digs_getOwner_srm(char *errorMessage,
 *                                      const char *filePath,
 *                                      const char *hostname,
 *                                      char **ownerName);
 * 
 * Gets the owner of the file.
 * 
 *   Parameters:                                                 [I/O]
 *	
 *     errorMessage     buffer to receive error message             O
 *                      (at least MAX_ERROR_MESSAGE_LENGTH chars)
 *     filePath         the full path to the file                 I
 *     hostname         the FQDN of the host to contact           I
 *     ownerName        receives pointer to owner of file (caller   O
 *                      should free)
 *
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getOwner_srm(char *errorMessage,
				    const char *filePath,
				    const char *hostname,
				    char **ownerName)
{
    struct soap *soap;
    char *endpoint;
  
    digs_error_code_t result = DIGS_SUCCESS;
    
    char *paths[2] = { "\0", "\0\0" };
    
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__true_;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_getOwner_srm(%s,%s)", filePath, hostname);
    
    *ownerName = safe_strdup("");
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	/*
	 * for attribute querying we allow status code 1 - it will be
	 * returned if we call srmLs on a directory with no execute
	 * permissions
	 */
	if (resp.srmLsResponse->returnStatus->statusCode < 2) {
	    if ((!resp.srmLsResponse->details) ||
		(!resp.srmLsResponse->details->__sizepathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].ownerPermission) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].ownerPermission->userID)) {
		result = DIGS_UNSPECIFIED_SERVER_ERROR;
		sprintf(errorMessage,
			"Required element missing from SRM server response");
	    }
	    else {
		globus_libc_free(*ownerName);
		*ownerName =
		    safe_strdup(resp.srmLsResponse->details->
				pathDetailArray[0].ownerPermission->userID);
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(paths[0]);
    
    srmDone(hostname, soap);
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_getGroup_srm(char *errorMessage,
 *                                     const char *filePath,
 *                                     const char *hostname,
 *                                     char **groupName);
 * 
 * Gets the group of a file on a node.
 *
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (should be at
 *                  least MAX_ERROR_MESSAGE_LENGTH chars long)      O
 *   filePath       the full path to the file                     I
 *   hostname       the FQDN of the host to contact               I
 *   groupName      receives pointer to group of file (caller       O
 *                  should free)
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getGroup_srm(char *errorMessage,
				    const char *filePath,
				    const char *hostname,
				    char **groupName)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *path;
    char *group;
    enum ns1__TPermissionMode perms;
    
    logMessage(DEBUG, "digs_getGroup_srm(%s,%s)", filePath, hostname);
    
    *groupName = safe_strdup("");
    errorMessage[0] = 0;
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    path = constructSRMPath(hostname, filePath);
    result = getGroupInfo(errorMessage, soap, endpoint, path, &group,
			  &perms);
    if (group != NULL) {
	globus_libc_free(*groupName);
	*groupName = group;
    }
    
    globus_libc_free(path);
    srmDone(hostname, soap);
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_setGroup_srm(char *errorMessage, 
 *                                     const char *filePath,
 *                                     const char *hostname,
 *                                     char *groupName);
 * 
 * Sets up/changes a group of a file/directory on a node (chgrp).
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at     O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   filePath       full path to the file                         I
 *   hostname       FQDN of host to contact                       I
 *   groupName      group name to change file to                  I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_setGroup_srm(char *errorMessage,
				    const char *filePath,
				    const char * hostname,
				    const char *groupName)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *oldgroup;
    char *path;
    enum ns1__TPermissionMode perms;
    
    logMessage(DEBUG, "digs_setGroup_srm(%s,%s,%s)", filePath, hostname,
	       groupName);
    
    errorMessage[0] = 0;
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_CONNECTION;
    }
    
    path = constructSRMPath(hostname, filePath);
    
    /* first get existing group and permissions */
    result = getGroupInfo(errorMessage, soap, endpoint, path, &oldgroup,
			  &perms);
    if (oldgroup) {
	/* only change things if new group is actually different */
	if (strcmp(oldgroup, groupName)) {
	    /* add new group with permissions */
	    result = modifyGroupACL(errorMessage, soap, endpoint, path,
				    groupName, perms,
				    ns1__TPermissionType__ADD);
	    if (result == DIGS_SUCCESS) {
		/* set old group's permissions to 0 */
		result = modifyGroupACL(errorMessage, soap, endpoint, path,
					oldgroup, ns1__TPermissionMode__NONE,
					ns1__TPermissionType__CHANGE);
		
		/* delete old group if possible */
		modifyGroupACL(errorMessage, soap, endpoint, path,
			       oldgroup, ns1__TPermissionMode__NONE,
			       ns1__TPermissionType__REMOVE);
	    }
	    else {
		result = DIGS_UNKNOWN_ERROR;
	    }
	}
	globus_libc_free(oldgroup);
    }
    
    globus_libc_free(path);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_getPermissions_srm(char *errorMessage,
 *                                           const char *filePath,
 *                                           const char *hostname,
 *                                           char **permissions);
 * 
 * Gets the permissions of a file on a node.
 *
 * Returned permissions are stored in variable permissions, in a
 * numberic representation. For example 754 would mean:	
 * owner: read, write and execute permissions,
 * group: read and execute permissions,
 * others: only read permissions. 
 * 
 * Note that permissions should be freed 
 * according to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)           O
 *   filePath       full path to the file                         I
 *   hostname       FQDN of the host to contact                   I
 *   permissions    receives pointer to permissions string          O
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getPermissions_srm(char *errorMessage,
					  const char *filePath,
					  const char *hostname,
					  char **permissions)
{
    struct soap *soap;
    char *endpoint;
    
    digs_error_code_t result = DIGS_SUCCESS;
    
    char *paths[2] = { "\0", "\0\0" };
    
    char *group;
    enum ns1__TPermissionMode groupperms;
    
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__true_;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_getPermissions_srm(%s,%s)", filePath,
	       hostname);
    
    *permissions = safe_strdup("0000");
    (*permissions)[0] = 0;
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	/*
	 * for attribute querying we allow status code 1 - it will be
	 * returned if we call srmLs on a directory with no execute
	 * permissions
	 */
	if (resp.srmLsResponse->returnStatus->statusCode < 2) {
	    if ((!resp.srmLsResponse->details) ||
		(!resp.srmLsResponse->details->__sizepathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].groupPermission) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].ownerPermission) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].otherPermission)) {
		result = DIGS_UNSPECIFIED_SERVER_ERROR;
		sprintf(errorMessage,
			"Required element missing from SRM server response");
	    }
	    else {
		(*permissions)[0] = '0';
		(*permissions)[1] =
		    '0' + resp.srmLsResponse->details->
		    pathDetailArray[0].ownerPermission->mode;
		(*permissions)[2] =
		    '0' + resp.srmLsResponse->details->
		    pathDetailArray[0].groupPermission->mode;
		(*permissions)[3] = 
		    '0' + *resp.srmLsResponse->details->
		    pathDetailArray[0].otherPermission;
		
		/*
		 * Try to get proper group permissions
		 */
		result = getGroupInfo(errorMessage, soap, endpoint, paths[0],
				      &group, &groupperms);
		if (group) {
		    (*permissions)[2] = '0' + groupperms;
		    globus_libc_free(group);
		}
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(paths[0]);
    
    srmDone(hostname, soap);
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_setPermissions_srm(char *errorMessage, 
 *                                           const char *filePath,
 *                                           const char *hostname,
 *                                           const char *permissions);
 * 
 * Sets up/changes permissions of a file/directory on a node. 
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at     O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   filePath       full path to the file                         I
 *   hostname       FQDN of host to contact                       I
 *   permissions    octal string of permissions to set              O
 *                  (e.g. "0640")
 *
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_setPermissions_srm(char *errorMessage,
					  const char *filePath,
					  const char *hostname,
					  const char *permissions)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *group;
    char *path;
    
    enum ns1__TPermissionMode ownerperm = ns1__TPermissionMode__NONE;
    enum ns1__TPermissionMode otherperm = ns1__TPermissionMode__NONE;
    enum ns1__TPermissionMode groupperm = ns1__TPermissionMode__NONE;
    
    struct ns1__srmSetPermissionRequest req;
    struct ns1__srmSetPermissionResponse_ resp;
    
    logMessage(DEBUG, "digs_setPermissions_srm(%s,%s,%s)", filePath,
	       hostname, permissions);
    
    errorMessage[0] = 0;
    
    if (strlen(permissions) != 4) {
	strcpy(errorMessage,
	       "Permissions passed to digs_setPermissions_srm are invalid");
	return DIGS_UNKNOWN_ERROR;
    }
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	globus_libc_free(group);
	return DIGS_NO_SERVICE;
    }
    
    path = constructSRMPath(hostname, filePath);
    
    /* get group for file */
    result = getGroupInfo(errorMessage, soap, endpoint, path, &group,
			  &groupperm);
    
    ownerperm = permissions[1] - '0';
    groupperm = permissions[2] - '0';
    otherperm = permissions[3] - '0';
    
    req.authorizationID = NULL;
    req.SURL = path;
    req.permissionType = ns1__TPermissionType__CHANGE;
    req.ownerPermission = &ownerperm;
    req.arrayOfUserPermissions = NULL;
    req.arrayOfGroupPermissions = NULL;
    req.otherPermission = &otherperm;
    req.storageSystemInfo = NULL;
    
    /* set owner and other permissions using standard call */
    if (soap_call_ns1__srmSetPermission(soap, endpoint, "setPermission",
					&req, &resp) == SOAP_OK) {
	if (resp.srmSetPermissionResponse->returnStatus->statusCode != 0) {
	    result =
		processSrmError(errorMessage,
				resp.srmSetPermissionResponse->returnStatus);
	}
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    /* set group ACL */
    if (group) {
	result = modifyGroupACL(errorMessage, soap, endpoint, path, group,
				groupperm, ns1__TPermissionType__CHANGE);
	globus_libc_free(group);
    }
    
    globus_libc_free(path);
    
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t
 * digs_getModificationTime_srm(char *errorMessage, 
 *                              const char *filePath,
 *                              const char *hostname, 
 *                              time_t *modificationTime);
 * 
 * Returns the modification time of a file located on a node.
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)           O
 *   filePath       full path to file                             I
 *   hostname       FQDN of the host to contact                   I
 *   modificationTime  receives file modification time              O
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getModificationTime_srm(char *errorMessage,
					       const char *filePath,
					       const char *hostname,
					       time_t *modificationTime)
{
    struct soap *soap;
    char *endpoint;
    
    digs_error_code_t result = DIGS_SUCCESS;
    
    char *paths[2] = { "\0", "\0\0" };
    
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__true_;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_getModificationTime_srm(%s,%s)", filePath,
	       hostname);
    
    *modificationTime = 0;
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	/*
	 * for attribute querying we allow status code 1 - it will be
	 * returned if we call srmLs on a directory with no execute
	 * permissions
	 */
	if (resp.srmLsResponse->returnStatus->statusCode < 2) {
	    if ((!resp.srmLsResponse->details) ||
		(!resp.srmLsResponse->details->__sizepathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray) ||
		(!resp.srmLsResponse->details->pathDetailArray[0].lastModificationTime)) {
		result = DIGS_UNSPECIFIED_SERVER_ERROR;
		sprintf(errorMessage,
			"Required element missing from SRM server response");
	    }
	    else {
		*modificationTime =
		    *resp.srmLsResponse->details->
		    pathDetailArray[0].lastModificationTime;
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
    }
    else {
	/* SOAP error */
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(paths[0]);
    
    srmDone(hostname, soap);
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_startPutTransfer_srm(char *errorMessage,
 *                                             const char *hostname,
 *		                               const char *localPath,
 *                                             const char *SURL,
 *                                             int *handle);
 *
 * Start a put operation (push) of a local file to a remote hostname
 * with a specified remote location (SURL). A handle is returned to
 * uniquely identify this transfer.
 * 
 * If a file with the chosen name already exists at the remote location,
 * it may be overwriten without a warning.
 *
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive message on error (must be at  O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   localPath      full path to local file to upload             I
 *   hostname       FQDN of the host to contact                   I
 *   SURL           location to upload file to                    I
 *   handle         receives unique id for transfer                 O
 *
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startPutTransfer_srm(char *errorMessage,
					    const char *hostname,
					    const char *localPath,
					    const char *SURL,
					    int *handle)
{
    struct soap *soap;
    char *endpoint;
    char *path;
    char *token;
    char *protocol;
    char *dirname;
    char *slash;
    digs_error_code_t result = DIGS_SUCCESS;
    srm_transfer_t *t;
    long long size;
    
    logMessage(DEBUG, "digs_startPutTransfer_srm(%s,%s,%s)", hostname,
	       localPath, SURL);
    
    errorMessage[0] = 0;
    *handle = -1;
    
    /*
     * Get the local file size
     */
    size = getFileLength(localPath);
    if (size < 0) {
	snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH,
		 "Cannot get size of local file %s", localPath);
	return DIGS_UNKNOWN_ERROR;
    }
    
    path = constructSRMPath(hostname, SURL);
    
    /* make sure the remote directory exists */
    dirname = safe_strdup(SURL);
    slash = strrchr(dirname, '/');
    if (slash) {
	*slash = 0;
	digs_mkdirtree_srm(errorMessage, hostname, dirname);
    }
    globus_libc_free(dirname);
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	globus_libc_free(path);
	return DIGS_NO_SERVICE;
    }
    
    result = initiatePutRequest(errorMessage, soap, endpoint, path,
				&token, &protocol, (ULONG64)size);
    if (result == DIGS_SUCCESS) {
	t = newSrmTransfer(hostname, localPath, DIGS_SRM_PUT_TRANSFER);
	t->token = token; /* will be freed when transfer destroyed */
	t->remoteFile = path;
	*handle = t->handle;
	globus_libc_free(protocol);
    }
    else {
	globus_libc_free(path);
    }
    
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
*   char *substituteSlashes(char *filename)
*    
*   Utility function to replace all the slashes (directory separators)
*   in a filename with the sequence '-DIR-'. This is done to avoid
*   maintaining a complex, globally-writable directory hierarchy within
*   the 'NEW' directory.
*    
*   Parameters:                                    [I/O]
*
*     filename pointer to the filename to process   I
*    
*   Returns: pointer to the processed filename. Should be freed by the
*            caller. Returns NULL on error.
***********************************************************************/
static char *substituteSlashes(const char *filename)
{
    char *newFilename;
    int i, j;
    
    logMessage(1, "substituteSlashes(%s)", filename);
    
    newFilename = globus_libc_malloc(5 * strlen(filename));
    if (!newFilename)
    {
	errorExit("Out of memory in substituteSlashes");
    }

    i = 0;
    j = 0;
    while (filename[i])
    {
	if (filename[i] == '/')
	{
	    newFilename[j] = '-';
	    newFilename[j+1] = 'D';
	    newFilename[j+2] = 'I';
	    newFilename[j+3] = 'R';
	    newFilename[j+4] = '-';
	    j += 5;
	}
	else
	{
	    newFilename[j] = filename[i];
	    j++;
	}

	i++;
    }
    newFilename[j] = 0;
    return newFilename;
}

/***********************************************************************
 * digs_error_code_t digs_startCopyToInbox_srm(char *errorMessage,
 *		                               const char *hostname,
 *                                             const char *localPath,
 *                                             const char *lfn,
 *                                             int *handle) 
 * 
 * This method is effectively a wrapper around digs_startPutTransfer()
 * that firstly resolves the target SURL based on the specific 
 * configuration of the SE (identified by its hostname). Not all
 * SEs have Inboxes. If this function is called on an SE that does not
 * have an Inbox, then an appropriate error is returned.
 *
 * A handle is returned to uniquely identify this transfer.
 * 
 * If a file with the chosen name already exists at the remote
 * location, it will be overwriten without a warning.
 * 
 * Parameters:                                                  [I/O]
 *
 *   errorMessage   buffer to receive message on error (must be    O
 *                  at least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of the host to contact                  I
 *   localPath      full path to local file to upload            I
 *   lfn            logical name for file on grid                I
 *   handle         receives unique ID for transfer                O
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startCopyToInbox_srm(char *errorMessage,
					    const char *hostname,
					    const char *localPath,
					    const char *lfn,
					    int *handle)
{
    digs_error_code_t result = DIGS_SUCCESS;
    char *inbox;
    char *flatFilename;
    char *remoteFilename;
    
    logMessage(DEBUG, "digs_startCopyToInbox_srm(%s,%s,%s)", hostname,
	       localPath, lfn);
    
    *handle = -1;
    errorMessage[0] = 0;
    
    inbox = getNodeInbox(hostname);
    if (!inbox) {
	return DIGS_NO_INBOX;
    }
    
    flatFilename = substituteSlashes(lfn);
    if (!flatFilename) {
	errorExit("Out of memory in digs_startCopyToInbox_srm");
    }
    
    if (safe_asprintf(&remoteFilename, "%s/%s", inbox,
		      flatFilename) < 0) {
	errorExit("Out of memory in digs_startCopyToInbox_srm");    
    }
    globus_libc_free(flatFilename);
    
    result = digs_startPutTransfer_srm(errorMessage, hostname, localPath,
				       remoteFilename, handle);
    
    globus_libc_free(remoteFilename);
    return result;
}


/***********************************************************************
 * digs_error_code_t
 * digs_monitorTransfer_srm(char *errorMessage, int handle,
 *                          digs_transferStatus_t *status,
 *                          int *percentComplete);
 * 
 * Checks the progress of a transfer, identified by a handle, and
 * returns its status. Also returns the percentage of the transfer
 * that has been completed.
 * 
 * Parameters:                                                  [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at    O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   handle         ID of the transfer                           I
 *   status         receives status of the transfer                O
 *   percentComplete  receives percentage progress of transfer     O
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t
digs_monitorTransfer_srm(char *errorMessage, int handle, 
			 digs_transfer_status_t *status,
			 int *percentComplete)
{
    srm_transfer_t *t;
    digs_error_code_t result = DIGS_SUCCESS;
    struct soap *soap;
    char *endpoint;
    int pcl;
    
    logMessage(DEBUG, "digs_monitorTransfer_srm(%d)", handle);
    
    errorMessage[0] = 0;
    
    *status = DIGS_TRANSFER_IN_PROGRESS;
    
    t = findSrmTransfer(handle);
    if (!t) {
	/* bad handle */
	strcpy(errorMessage, "Bad handle passed to digs_monitorTransfer");
	return DIGS_UNKNOWN_ERROR;
    }
    
    switch (t->status) {
    case DIGS_SRM_FINISHED:
	*status = DIGS_TRANSFER_DONE;
	*percentComplete = 100;
	break;
	
    case DIGS_SRM_ERROR:
	*status = DIGS_TRANSFER_FAILED;
	*percentComplete = 0;
	result = DIGS_UNKNOWN_ERROR;
	break;
	
    case DIGS_SRM_WAITING_FOR_TURL:
    {
	*percentComplete = 0;
	if (!srmInit(t->hostname, &soap, &endpoint)) {
	    return DIGS_NO_SERVICE;
	}
	
	switch (t->type) {
	case DIGS_SRM_GET_TRANSFER:
	{
	    /* monitor SRM get request */
	    result = checkGetRequest(errorMessage, soap, endpoint,
				     t->token, &t->turl);
	    if (result != DIGS_SUCCESS) {
		/* failed */
		logMessage(WARN, "checkGetRequest returned failure");
		t->status = DIGS_SRM_ERROR;
		*status = DIGS_TRANSFER_FAILED;
		*percentComplete = 0;
	    }
	    else if (t->turl) {
		/* ready to start GridFTP transfer */
		result = srm_gsiftp_startGetTransfer(errorMessage,
						     t->hostname,
						     t->turl,
						     t->localFile,
						     &t->gid);
		if (result == DIGS_SUCCESS) {
		    *percentComplete = 50;
		    *status = DIGS_TRANSFER_IN_PROGRESS;
		    t->status = DIGS_SRM_WAITING_FOR_GRIDFTP;
		}
		else {
		    logMessage(WARN, "starting gridftp get failed");
		    *percentComplete = 0;
		    *status = DIGS_TRANSFER_FAILED;
		    t->status = DIGS_SRM_ERROR;
		}
	    }
	}
	break;
	case DIGS_SRM_PUT_TRANSFER:
	{
	    /* monitor SRM put request */
	    result = checkPutRequest(errorMessage, soap, endpoint,
				     t->token, &t->turl);
	    if (result != DIGS_SUCCESS) {
		/* failed */
		logMessage(WARN, "checkPutRequest returned failure");
		t->status = DIGS_SRM_ERROR;
		*status = DIGS_TRANSFER_FAILED;
		*percentComplete = 0;
	    }
	    else if (t->turl) {
		/* ready to start GridFTP transfer */
		result = srm_gsiftp_startPutTransfer(errorMessage,
						     t->hostname,
						     t->turl,
						     t->localFile,
						     &t->gid);
		if (result == DIGS_SUCCESS) {
		    *percentComplete = 50;
		    *status = DIGS_TRANSFER_IN_PROGRESS;
		    t->status = DIGS_SRM_WAITING_FOR_GRIDFTP;
		}
		else {
		    logMessage(WARN, "starting gridftp put failed");
		    *percentComplete = 0;
		    *status = DIGS_TRANSFER_FAILED;
		    t->status = DIGS_SRM_ERROR;
		}
	    }
	}
	break;
	}
	
	srmDone(t->hostname, soap);
    }
    break;
    
    case DIGS_SRM_WAITING_FOR_GRIDFTP:
    {
	/* monitor GridFTP operation */
	*percentComplete = 50;
	result = srm_gsiftp_monitorTransfer(errorMessage, t->gid,
					    status, &pcl);
	if ((*status) == DIGS_TRANSFER_DONE) {
	    logMessage(DEBUG, "gridftp transfer completed");
	    *percentComplete = 100;
	    t->status = DIGS_SRM_FINISHED;
	}
	else if ((*status) == DIGS_TRANSFER_FAILED) {
	    logMessage(WARN, "gridftp transfer failed");
	    *percentComplete = 0;
	    t->status = DIGS_SRM_ERROR;
	}
    }
    break;
    }
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_endTransfer_srm(char *errorMessage,
 *                                        int handle)
 * 
 * Cleans up a transfer identified by a handle. Checks that the file 
 * has been transferred correctly (using checksum) and removes the 
 * LOCKED postfix if it has. If the transfer has not completed 
 * correctly, removes partially transferred files.
 *
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)           O
 *   handle         handle of transfer to end                     I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_endTransfer_srm(char *errorMessage, int handle)
{
    srm_transfer_t *t;
    digs_error_code_t result = DIGS_SUCCESS;
    struct soap *soap;
    char *endpoint;
    struct ns1__srmPutDoneRequest req;
    struct ns1__srmPutDoneResponse_ resp;
    struct ns1__ArrayOfAnyURI surls;
    char *path;
    
    logMessage(DEBUG, "digs_endTransfer_srm(%d)", handle);
    
    errorMessage[0] = 0;
    
    t = findSrmTransfer(handle);
    if (!t) {
	/* bad handle */
	strcpy(errorMessage, "Bad handle passed to digs_endTransfer");
	return DIGS_UNKNOWN_ERROR;
    }
    
    result = srm_gsiftp_endTransfer(errorMessage, t->gid);
    
    if ((t->type == DIGS_SRM_PUT_TRANSFER) && (result == DIGS_SUCCESS)) {
	/* for successful put transfers, call the srmPutDone */
	if (!srmInit(t->hostname, &soap, &endpoint)) {
	    result = DIGS_NO_SERVICE;
	}
	else {
	    req.authorizationID = NULL;
	    req.requestToken = t->token;
	    req.arrayOfSURLs = &surls;
	    
	    surls.__sizeurlArray = 1;
	    surls.urlArray = &path;
	    
	    path = t->remoteFile;
	    
	    if (soap_call_ns1__srmPutDone(soap, endpoint, "PutDone", &req,
					  &resp) == SOAP_OK) {
		if (resp.srmPutDoneResponse->returnStatus->statusCode != 0) {
		    result = processSrmError(errorMessage,
					     resp.srmPutDoneResponse->returnStatus);
		}
	    }
	    else {
		soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
		result = DIGS_NO_CONNECTION;
	    }
	    
	    srmDone(t->hostname, soap);
	}
    }
    
    destroySrmTransfer(handle);
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_cancelTransfer_srm(char *errorMessage,
 *                                           int handle)
 * 
 * Cancels and cleans up a transfer identified by a handle. Removes any
 * partially or completely transferred files.
 * 
 * Parameters:                                                    [I/O]
 *
 *   errorMessage   buffer to receive message on error (must be at   O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   handle         handle of transfer to cancel                   I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_cancelTransfer_srm(char *errorMessage, int handle)
{
    srm_transfer_t *t;
    digs_error_code_t result = DIGS_SUCCESS;
    struct soap *soap;
    char *endpoint;
    struct ns1__srmAbortRequestRequest req;
    struct ns1__srmAbortRequestResponse_ resp;
    
    logMessage(DEBUG, "digs_cancelTransfer_srm(%d)", handle);

    errorMessage[0] = 0;
    
    t = findSrmTransfer(handle);
    if (!t) {
	/* bad handle */
	strcpy(errorMessage, "Bad handle passed to digs_endTransfer");
	return DIGS_UNKNOWN_ERROR;
    }
    
    /* check whether there's a gridftp transaction to be cancelled */
    switch (t->status) {
    case DIGS_SRM_WAITING_FOR_GRIDFTP:
    case DIGS_SRM_FINISHED:
    case DIGS_SRM_ERROR:
	/* do gridftp cancel */
	srm_gsiftp_cancelTransfer(errorMessage, t->gid);
	break;
    }
    
    /* abort the SRM request */
    if (!srmInit(t->hostname, &soap, &endpoint)) {
	result = DIGS_NO_SERVICE;
    }
    else {
	req.requestToken = t->token;
	req.authorizationID = NULL;
	
	if (soap_call_ns1__srmAbortRequest(soap, endpoint, "Abort", &req,
					   &resp) == SOAP_OK) {
	    if (resp.srmAbortRequestResponse->returnStatus->statusCode != 0) {
		result = processSrmError(errorMessage,
					 resp.srmAbortRequestResponse->returnStatus);
	    }
	}
	else {
	    soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	    result = DIGS_NO_CONNECTION;
	}
	
	srmDone(t->hostname, soap);
    }
    
    destroySrmTransfer(handle);
    
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_mkdir_srm(char *errorMessage,
 *                                  const char *hostname,
 *                                  const char *filePath);
 * 
 * Creates a directory on a node.
 * Will return an error if the directory already exists.
 * 
 * Parameters:                                                    [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at      O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       the FQDN of the host to contact                I
 *   filePath       the full path of the directory to be created   I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mkdir_srm(char *errorMessage,
				 const char *hostname,
				 const char *filePath)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *path;
    
    struct ns1__srmMkdirRequest req;
    struct ns1__srmMkdirResponse_ resp;
    
    logMessage(DEBUG, "digs_mkdir_srm(%s,%s)", hostname, filePath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    path = constructSRMPath(hostname, filePath);
    
    req.authorizationID = NULL;
    req.SURL = path;
    req.storageSystemInfo = NULL;
    
    if (soap_call_ns1__srmMkdir(soap, endpoint, "Mkdir", &req, &resp) ==
	SOAP_OK) {
	if (resp.srmMkdirResponse->returnStatus->statusCode != 0) {
	    result = processSrmError(errorMessage,
				     resp.srmMkdirResponse->returnStatus);
	}
    }
    else {
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(path);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_mkdirtree_srm(char *errorMessage, 
 *                                      const char *hostname,
 *                                      const char *filePath);
 * 
 * Creates a tree of directories at one go on a node 
 * (e.g. 'mkdir lnf:/ukqcd/DWF')
 * Will return an error if the directory already exists.
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at     O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       the FQDN of the host to contact               I
 *   filePath       the path of the directory to be created       I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mkdirtree_srm(char *errorMessage,
				     const char *hostname,
				     const char *filePath )
{
    char *ns, *ps;
    char *path;
    digs_error_code_t result = DIGS_SUCCESS;
    
    logMessage(DEBUG, "digs_mkdirtree_srm(%s,%s)", hostname, filePath);
    
    errorMessage[0] = 0;
    
    path = safe_strdup(filePath);
    ps = strchr(path, '/');
    if (!ps) {
	/* must have at least the initial slash */
	strcpy(errorMessage, "Invalid directory path passed to mkdirtree");
	globus_libc_free(path);
	return DIGS_UNKNOWN_ERROR;
    }
    ns = strchr(ps+1, '/');
    while (ns) {
	/* terminate string at that slash */
	*ns = 0;
	
	/*
	 * make next dir of path. Don't worry about errors here, will
	 * frequently get "already exists" errors. Any others will be
	 * caught on the final mkdir call.
	 */
	result = digs_mkdir_srm(errorMessage, hostname, path);
	
	/* put slash back in */
	*ns = '/';
	
	/* find next slash if any */
	ps = ns;
	ns = strchr(ps+1, '/');
    }
    
    /* make the final directory */
    result = digs_mkdir_srm(errorMessage, hostname, path);
    
    globus_libc_free(path);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_startGetTransfer_srm(char *errorMessage,
 *		                               const char *hostname,
 *                                             const char *SURL,
 *                                             const char *localPath,
 *		                               int *handle);
 *
 * Starts a get operation (pull) of a file (SURL) from a  
 * remote hostname to local destination file path. A handle is
 * returned to uniquely identify this transfer.
 * 
 * Parameters:                                                  [I/O]
 *
 *   errorMessage   buffer to receive message on error (must be    O
 *                  at least MAX_ERROR_MESSAGE_LENGTH)
 *   hostname       FQDN of host to contact                      I
 *   SURL           full path to file                            I
 *   localPath      local path to store the file in              I
 *   handle         receives ID for transfer                       O
 *
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startGetTransfer_srm(char *errorMessage,
					    const char *hostname,
					    const char *SURL,
					    const char *localPath,
					    int *handle)
{
    struct soap *soap;
    char *endpoint;
    char *path;
    char *token;
    char *protocol;
    digs_error_code_t result = DIGS_SUCCESS;
    srm_transfer_t *t;
    FILE *f;
    
    logMessage(DEBUG, "digs_startGetTransfer_srm(%s,%s,%s)", hostname,
	       SURL, localPath);
    
    errorMessage[0] = 0;
    *handle = -1;
    
    /*
     * Check we can write to the file
     */
    f = fopen(localPath, "wb");
    if (!f) {
	snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH,
		 "Cannot open local file %s for writing", localPath);
	return DIGS_UNKNOWN_ERROR;
    }
    fclose(f);
    unlink(localPath);
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    path = constructSRMPath(hostname, SURL);
    
    result = initiateGetRequest(errorMessage, soap, endpoint, path,
				&token, &protocol);
    if (result == DIGS_SUCCESS) {
	t = newSrmTransfer(hostname, localPath, DIGS_SRM_GET_TRANSFER);
	t->token = token; /* will be freed when transfer destroyed */
	t->remoteFile = path;
	*handle = t->handle;
	globus_libc_free(protocol);
    }
    else {
	globus_libc_free(path);
    }
    
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_mv_srm(char *errorMessage,
 *                               const char *hostname,
 *                               const char *filePathFrom,
 *                               const char *filePathTo);
 * 
 * Moves/Renames a file on an SE (hostname). 
 * 
 * Parameters:                                                    [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at      O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of the host to contact                    I
 *   filePathFrom   path to move file from                         I
 *   filePathTo     path to move file to                           I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mv_srm(char *errorMessage, const char *hostname, 
			      const char *filePathFrom,
			      const char *filePathTo)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    struct ns1__srmMvRequest req;
    struct ns1__srmMvResponse_ resp;
    int isdir = 0;
    int exists = 0;
    
    logMessage(DEBUG, "digs_mv_srm(%s,%s,%s)", hostname, filePathFrom,
	       filePathTo);
    
    errorMessage[0] = 0;
    
    /* check if file is directory */
    digs_isDirectory_srm(errorMessage, filePathFrom, hostname,
			 &isdir);
    if (isdir) {
	strcpy(errorMessage, "Expected file but found directory");
	return DIGS_FILE_IS_DIR;
    }
    
    /* check if destination file already exists */
    digs_doesExist_srm(errorMessage, filePathTo, hostname,
		       &exists);
    if (exists) {
	/* remove it if so */
	digs_rm_srm(errorMessage, hostname, filePathTo);
    }
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    req.authorizationID = NULL;
    req.fromSURL = constructSRMPath(hostname, filePathFrom);
    req.toSURL = constructSRMPath(hostname, filePathTo);
    req.storageSystemInfo = NULL;
    
    if (soap_call_ns1__srmMv(soap, endpoint, "Mv", &req, &resp)
	== SOAP_OK) {
	if (resp.srmMvResponse->returnStatus->statusCode != 0) {
	    result = processSrmError(errorMessage,
				     resp.srmMvResponse->returnStatus);
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    globus_libc_free(req.fromSURL);
    globus_libc_free(req.toSURL);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_rm_srm(char *errorMessage,
 *                               const char *hostname, 
 *                               const char *filePath);
 * 
 * Deletes a file from a node.
 * 
 * Parameters:                                                    [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at      O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of host to contact                        I
 *   filePath       path of file to remove                         I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rm_srm(char *errorMessage, const char *hostname,
			      const char *filePath)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    int isdir = 0;
    
    struct ns1__srmRmRequest req;
    struct ns1__srmRmResponse_ resp;
    struct ns1__ArrayOfAnyURI surls;
    char *paths[2] = { NULL, NULL };
    
    logMessage(DEBUG, "digs_rm_srm(%s,%s)", hostname, filePath);
    
    errorMessage[0] = 0;
    
    /* check if file is directory */
    result = digs_isDirectory_srm(errorMessage, filePath, hostname,
				  &isdir);
    if (isdir) {
	strcpy(errorMessage, "Expected file but found directory");
	return DIGS_FILE_IS_DIR;
    }
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    paths[0] = constructSRMPath(hostname, filePath);
    
    req.authorizationID = NULL;
    req.arrayOfSURLs = &surls;
    req.storageSystemInfo = NULL;
    surls.__sizeurlArray = 1;
    surls.urlArray = &paths[0];
    
    if (soap_call_ns1__srmRm(soap, endpoint, "Rm", &req, &resp)
	== SOAP_OK) {
	if (resp.srmRmResponse->returnStatus->statusCode != 0) {
	    result = processSrmError(errorMessage,
				     resp.srmRmResponse->returnStatus);
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    globus_libc_free(paths[0]);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_rmdir_srm(char *errorMessage,
 *                                  const char *hostname, 
 *                                  const char *filePath);
 * 
 * Deletes a directory from a node.
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at     O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of the host to contact                   I
 *   dirPath        path to directory to be removed               I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rmdir_srm(char *errorMessage,
				 const char *hostname,
				 const char *dirPath)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *path;
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    struct ns1__srmRmdirRequest req;
    struct ns1__srmRmdirResponse_ resp;
    
    logMessage(DEBUG, "digs_rmdir_srm(%s,%s)", hostname, dirPath);
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    path = constructSRMPath(hostname, dirPath);
    
    req.authorizationID = NULL;
    req.SURL = path;
    req.storageSystemInfo = NULL;
    req.recursive = &recursive;
    
    if (soap_call_ns1__srmRmdir(soap, endpoint, "Rmdir", &req, &resp)
	== SOAP_OK) {
	if (resp.srmRmdirResponse->returnStatus->statusCode != 0) {
	    result = processSrmError(errorMessage,
				     resp.srmRmdirResponse->returnStatus);
	}
    }
    else {
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(path);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_copyFromInbox_srm(char *errorMessage, 
 *                                          const char *hostname,
 *                                          const char *lfn,
 *                                          const char *targetPath);
 * 
 * Moves a file (identified by lfn) from the Inbox folder to
 * permanent storage on an SE (identified by hostname), with a 
 * specified destination file path. 
 * Not all SEs have Inboxes. If this function is called on an SE that 
 * does not have an Inbox, then an appropriate error is returned.
 * 
 * If a file with the chosen name already exists at the remote
 * location, it will be overwriten without a warning.
 * 
 * Parameters:                                                 [I/O]
 *
 *   errorMessage   buffer to receive message on error (must be   O
 *                  at least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of the host to contact                 I
 *   lfn            logical name for file on grid               I
 *   targetPath     full path to copy file to                   I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_copyFromInbox_srm(char *errorMessage,
					 const char *hostname,
					 const char *lfn,
					 const char *targetPath)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *inbox;
    char *flatFilename;
    char *sourcePath;
    char *sourceSurl;
    char *targetSurl;
    char *proto;
    char *sourceToken;
    char *targetToken;
    long long size;
    char *sourceTurl = NULL;
    char *targetTurl = NULL;
    char *targetDir;
    char *slash;
    
    logMessage(DEBUG, "digs_copyFromInbox_srm(%s,%s,%s)", hostname, lfn,
	       targetPath);
    
    errorMessage[0] = 0;
    
    /* work out full path to source file */
    inbox = getNodeInbox(hostname);
    if (!inbox) {
	return DIGS_NO_INBOX;
    }
    flatFilename = substituteSlashes(lfn);
    if (safe_asprintf(&sourcePath, "%s/%s", inbox, flatFilename) < 0) {
	errorExit("Out of memory in digs_copyFromInbox_srm");
    }
    globus_libc_free(flatFilename);
    
    /* get source file size */
    result = digs_getLength_srm(errorMessage, sourcePath, hostname, &size);
    if (result != DIGS_SUCCESS) {
	globus_libc_free(sourcePath);
	return result;
    }
    
    /* work out SURLs for both */
    sourceSurl = constructSRMPath(hostname, sourcePath);
    targetSurl = constructSRMPath(hostname, targetPath);
    globus_libc_free(sourcePath);
    
    /* make sure destination directory exists */
    targetDir = safe_strdup(targetPath);
    slash = strrchr(targetDir, '/');
    if (slash) {
	*slash = 0;
	digs_mkdirtree_srm(errorMessage, hostname, targetDir);
    }
    globus_libc_free(targetDir);
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    /* request TURLs for both files */
    result = initiateGetRequest(errorMessage, soap, endpoint, sourceSurl,
				&sourceToken, &proto);
    if (result == DIGS_SUCCESS) {
	globus_libc_free(proto);
	
	result = initiatePutRequest(errorMessage, soap, endpoint, targetSurl,
				    &targetToken, &proto, (ULONG64)size);
	if (result == DIGS_SUCCESS) {
	    globus_libc_free(proto);
	    
	    /* wait for both TURLs to become available */
	    while ((!sourceTurl) || (!targetTurl)) {
		result = checkGetRequest(errorMessage, soap, endpoint, sourceToken,
					 &sourceTurl);
		if (result != DIGS_SUCCESS) {
		    break;
		}
		
		result = checkPutRequest(errorMessage, soap, endpoint, targetToken,
					 &targetTurl);
		if (result != DIGS_SUCCESS) {
		    break;
		}
		
		globus_libc_usleep(10000);
	    }
	    
	    /* if we got both TURLs, do the transfer */
	    if ((sourceTurl) && (targetTurl)) {
		result = srm_gsiftp_thirdPartyCopy(errorMessage, hostname,
						   sourceTurl, targetTurl);
		if (result == DIGS_SUCCESS) {
		    struct ns1__srmPutDoneRequest req;
		    struct ns1__srmPutDoneResponse_ resp;
		    struct ns1__ArrayOfAnyURI surls;
		    char *tpath;
		    
		    /* call putDone */
		    tpath = targetSurl;
		    
		    req.authorizationID = NULL;
		    req.requestToken = targetToken;
		    req.arrayOfSURLs = &surls;
		    
		    surls.__sizeurlArray = 1;
		    surls.urlArray = &tpath;
		    
		    if (soap_call_ns1__srmPutDone(soap, endpoint, "PutDone", &req,
						  &resp) == SOAP_OK) {
			if (resp.srmPutDoneResponse->returnStatus->statusCode != 0) {
			    result =
				processSrmError(errorMessage,
						resp.srmPutDoneResponse->returnStatus);
			}
			else {
			    
			}
		    }
		    else {
			soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
			result = DIGS_NO_CONNECTION;
		    }
		}
	    }
	    
	    if (sourceTurl) globus_libc_free(sourceTurl);
	    if (targetTurl) globus_libc_free(targetTurl);
	    
	    globus_libc_free(targetToken);
	}
	globus_libc_free(sourceToken);
    }
    
    srmDone(hostname, soap);
    globus_libc_free(sourceSurl);
    globus_libc_free(targetSurl);
    return result;
}

/***********************************************************************
 * int countSrmList(struct ns1__ArrayOfTMetaDataPathDetail *l,
 *                  int allFiles)
 * 
 * Helper function which counts the number of regular files in an SRM
 * list. Directories are not included in the count. The function
 * automatically recurses into sub-directories (assuming the original
 * list operation was recursive).
 * 
 * Parameters:                                                   [I/O]
 *
 *   l          pointer to directory info returned by SRM         I
 *   allFiles   whether to include files with a "-LOCKED" suffix  I
 *    
 * Returns: number of regular files in listing
 ***********************************************************************/
static int countSrmList(struct ns1__ArrayOfTMetaDataPathDetail *l,
			int allFiles)
{
    int i;
    int count = 0;
    
    if (!l) {
	return 0;
    }
    
    for (i = 0; i < l->__sizepathDetailArray; i++) {
	if ((*l->pathDetailArray[i].type) == 1) {
	    /* subdirectory */
	    count += countSrmList(l->pathDetailArray[i].arrayOfSubPaths,
				  allFiles);
	}
	else {
	    /* file */
	    if (allFiles) {
		count++;
	    }
	    else {
		int len = strlen(l->pathDetailArray[i].path);
		if (strcmp(&l->pathDetailArray[i].path[len-7], "-LOCKED")) {
		    count++;
		}
	    }
	}
    }
    return count;
}

/***********************************************************************
 * int populateSrmList(struct ns1__ArrayOfTMetaDataPathDetail *l,
 *                     int allFiles, char **list, int listpos)
 * 
 * Helper function which translates an SRM list structure into a simple
 * list of filenames as required by the DiGS SE interface. Recurses into
 * sub-directories if they're included in the listing.
 * 
 * Parameters:                                                   [I/O]
 *
 *   l          pointer to directory info returned by SRM         I
 *   allFiles   whether to include files with a "-LOCKED" suffix  I
 *   list       DiGS list to add files to (must have enough space I
 *              allocated)
 *   listpos    first unused index in list                        I
 *    
 * Returns: next unused index in list
 ***********************************************************************/
static int populateSrmList(struct ns1__ArrayOfTMetaDataPathDetail *l,
			   int allFiles, char **list, int listpos)
{
    int i;
    int pos = listpos;
    
    if (!l) {
	return pos;
    }
    
    for (i = 0; i < l->__sizepathDetailArray; i++) {
	if ((*l->pathDetailArray[i].type) == 1) {
	    /* subdirectory */
	    pos = populateSrmList(l->pathDetailArray[i].arrayOfSubPaths,
				  allFiles, list, pos);
	}
	else {
	    int len = strlen(l->pathDetailArray[i].path);
	    if ((allFiles) || (strcmp(&l->pathDetailArray[i].path[len-7],
				      "-LOCKED"))) {
		/* file */
		list[pos] = safe_strdup(l->pathDetailArray[i].path);
		pos++;
	    }
	}
    }
    return pos;
}

/***********************************************************************
 * digs_error_code_t digs_scanNode_srm(char *errorMessage, 
 *                                     const char *hostname,
 *                                     char ***list,
 *                                     int *listLength,
 *                                     int allFiles);
 * 
 * Gets a list of all the DiGS file locations. Recursive directory
 * listing of all the DiGS data files
 * Setting the allFiles flag to zero will hide any temporary (locked) 
 * files.
 *
 * Returned an array of the file paths is stored in variable list. Note    
 * that list should be freed accordingly to the way it was allocated in 
 * the implementation! Use digs_free_string_array.
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at     O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of the host to contact                   I
 *   list           receives array of full filepaths                O
 *   listLength     receives length of array                        O
 *   allFiles       whether to show -LOCKED files                 I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_scanNode_srm(char *errorMessage,
				    const char *hostname,
				    char ***list, int *listLength,
				    int allFiles)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *topDir = getNodePath(hostname);
    char *dataDir;
    int dirnum = 1;
    int exists = 0;
    int count;
    int pos;
    
    char *paths[2] = { NULL, NULL };
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__false_;
    enum xsd__boolean recursive = xsd__boolean__true_;
    
    logMessage(DEBUG, "digs_scanNode_srm(%s)", hostname);
    
    if (!topDir) {
	strcpy(errorMessage, "Error getting node path in scanNode");
	return DIGS_UNKNOWN_ERROR;
    }
    
    dataDir = globus_libc_malloc(strlen(topDir) + 10);
    
    /* initialise list to empty */
    *list = globus_libc_malloc(sizeof(char*));
    *listLength = 0;
    
    errorMessage[0] = 0;
    
    /* loop over all data directories */
    sprintf(dataDir, "%s/data", topDir);
    digs_doesExist_srm(errorMessage, dataDir, hostname, &exists);
    
    while (exists) {
	/*
	 * have to re-init soap every time as existence check method
	 * will undo it
	 */
	if (!srmInit(hostname, &soap, &endpoint)) {
	    return DIGS_NO_SERVICE;
	}
	
	/* list this data directory */
	req.authorizationID = NULL;
	req.storageSystemInfo = NULL;
	req.fileStorageType = NULL;
	req.fullDetailedList = &detailed;
	req.allLevelRecursive = &recursive;
	req.numOfLevels = NULL;
	req.offset = NULL;
	req.count = NULL;
	req.arrayOfSURLs = &arrayOfSURLs;
	arrayOfSURLs.__sizeurlArray = 1;
	arrayOfSURLs.urlArray = &paths[0];
	paths[0] = constructSRMPath(hostname, dataDir);
	
	logMessage(DEBUG, "listing %s", dataDir);
	
	if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp) ==
	    SOAP_OK) {
	    
	    if (resp.srmLsResponse->returnStatus->statusCode == 0) {
		/* count how many files */
		count = countSrmList(resp.srmLsResponse->details, allFiles);
		
		if (count > 0) {
		    /* expand list as needed */
		    pos = *listLength;
		    (*listLength) += count;
		    (*list) = realloc((*list), (*listLength)*sizeof(char*));
		    
		    /* put the filenames in the list */
		    populateSrmList(resp.srmLsResponse->details, allFiles, *list, pos);
		}
	    }
	    else {
		result = processSrmError(errorMessage,
					 resp.srmLsResponse->returnStatus);
	    }
	}
	else {
	    result = DIGS_NO_CONNECTION;
	    soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	}
	
	srmDone(hostname, soap);
	
	globus_libc_free(paths[0]);
	
	sprintf(dataDir, "%s/data%d", topDir, dirnum);
	dirnum++;
	digs_doesExist_srm(errorMessage, dataDir, hostname, &exists);
    }
    
    globus_libc_free(dataDir);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_scanInbox_srm(char *errorMessage, 
 *                                      const char *hostname,
 *                                      char ***list, int *listLength,
 *                                      int allFiles);
 * 
 * Gets a list of all the DiGS file locations in the inbox.
 * Directory listing of all the files under inbox.
 * Setting the allFiles flag to zero will hide any temporary (locked) 
 * files.
 *
 * Returned an array of the file paths is stored in variable list. Note    
 * that list should be freed accordingly to the way it was allocated in 
 * the implementation! Use digs_free_string_array.
 * 
 * Parameters:                                                   [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)           O
 *   hostname       FQDN of host to contact                       I
 *   list           receives pointer to list of files in inbox      O
 *   listLength     receives number of files in inbox               O
 *   allFiles       whether to list locked files or not           I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_scanInbox_srm(char *errorMessage,
				     const char *hostname,
				     char ***list, int *listLength,
				     int allFiles)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *inboxDir;
    int count;
    char *paths[2] = { NULL, NULL };
    struct ns1__srmLsRequest req;
    struct ns1__srmLsResponse_ resp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__false_;
    /* inbox should never have nested directories in it */
    enum xsd__boolean recursive = xsd__boolean__false_;
    
    logMessage(DEBUG, "digs_scanInbox_srm(%s)", hostname);
    
    *list = globus_libc_malloc(sizeof(char*));
    *listLength = 0;
    
    inboxDir = getNodeInbox(hostname);
    if (!inboxDir) {
	return DIGS_NO_INBOX;
    }
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    req.authorizationID = NULL;
    req.storageSystemInfo = NULL;
    req.fileStorageType = NULL;
    req.fullDetailedList = &detailed;
    req.allLevelRecursive = &recursive;
    req.numOfLevels = NULL;
    req.offset = NULL;
    req.count = NULL;
    req.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &paths[0];
    paths[0] = constructSRMPath(hostname, inboxDir);
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &req, &resp)
	== SOAP_OK) {
	if (resp.srmLsResponse->returnStatus->statusCode == 0) {
	    count = countSrmList(resp.srmLsResponse->details, allFiles);
	    if (count > 0) {
		(*listLength) = count;
		(*list) = realloc((*list), (*listLength)*sizeof(char*));
		populateSrmList(resp.srmLsResponse->details, allFiles, *list,
				0);
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     resp.srmLsResponse->returnStatus);
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    
    globus_libc_free(paths[0]);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * digs_error_code_t digs_rmr_srm(char *errorMessage,
 *                                const char *hostname, 
 *                                const char *filePath);
 * 
 * Deletes a directory with subdirectories (recursively) from a node.
 * 
 * Parameters:                                                    [I/O]
 *
 *   errorMessage   buffer to receive error message (must be at      O
 *                  least MAX_ERROR_MESSAGE_LENGTH chars)
 *   hostname       FQDN of the host to contact                    I
 *   filePath       path of directory to be removed                I
 *    
 * Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rmr_srm(char *errorMessage, const char *hostname,
			       const char *filePath)
{
    struct soap *soap;
    char *endpoint;
    digs_error_code_t result = DIGS_SUCCESS;
    char *path;
    
    struct ns1__srmLsRequest lreq;
    struct ns1__srmLsResponse_ lresp;
    struct ns1__ArrayOfAnyURI arrayOfSURLs;
    enum xsd__boolean detailed = xsd__boolean__false_;
    enum xsd__boolean recursive = xsd__boolean__true_;
    
    int isdir = 0;
    
    struct ns1__srmRmdirRequest req;
    struct ns1__srmRmdirResponse_ resp;
    
    char **list = NULL;
    int listLength = 0;
    int i;
    
    logMessage(DEBUG, "digs_rmr_srm(%s,%s)", hostname, filePath);
    
    /* 
     * This is supposed to work on files or directories, so check which
     * we're dealing with
     */
    result = digs_isDirectory_srm(errorMessage, filePath, hostname,
				  &isdir);
    if (!isdir) {
	result = digs_rm_srm(errorMessage, hostname, filePath);
	return result;
    }
    
    errorMessage[0] = 0;
    if (!srmInit(hostname, &soap, &endpoint)) {
	return DIGS_NO_SERVICE;
    }
    
    path = constructSRMPath(hostname, filePath);
    
    /*
     * handle recursive directory delete. SRM's recursive rmdir won't
     * work if there are regular files within the directory tree (at
     * least on DPM). So we have to scan for them and delete them first.
     */
    lreq.authorizationID = NULL;
    lreq.storageSystemInfo = NULL;
    lreq.fileStorageType = NULL;
    lreq.fullDetailedList = &detailed;
    lreq.allLevelRecursive = &recursive;
    lreq.numOfLevels = NULL;
    lreq.offset = NULL;
    lreq.count = NULL;
    lreq.arrayOfSURLs = &arrayOfSURLs;
    arrayOfSURLs.__sizeurlArray = 1;
    arrayOfSURLs.urlArray = &path;
    
    if (soap_call_ns1__srmLs(soap, endpoint, "Ls", &lreq, &lresp)
	== SOAP_OK) {
	if (lresp.srmLsResponse->returnStatus->statusCode == 0) {
	    listLength = countSrmList(lresp.srmLsResponse->details, 1);
	    if (listLength) {
		list = globus_libc_malloc(listLength * sizeof(char*));
		populateSrmList(lresp.srmLsResponse->details, 1, list, 0);
	    }
	}
	else {
	    result = processSrmError(errorMessage,
				     lresp.srmLsResponse->returnStatus);
	}
    }
    else {
	result = DIGS_NO_CONNECTION;
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
    }
    srmDone(hostname, soap);
    
    /* delete the files listed */
    if (list) {
	for (i = 0; i < listLength; i++) {
	    digs_rm_srm(errorMessage, hostname, list[i]);
	}
	digs_free_string_array_srm(&list, &listLength);
    }
    
    if (!srmInit(hostname, &soap, &endpoint)) {
	globus_libc_free(path);
	return DIGS_NO_SERVICE;
    }
    
    /* now we can finally remove the directory structure */
    req.authorizationID = NULL;
    req.SURL = path;
    req.storageSystemInfo = NULL;
    req.recursive = &recursive;
    
    if (soap_call_ns1__srmRmdir(soap, endpoint, "Rmdir", &req, &resp)
	== SOAP_OK) {
	if (resp.srmRmdirResponse->returnStatus->statusCode != 0) {
	    result = processSrmError(errorMessage,
				     resp.srmRmdirResponse->returnStatus);
	}
    }
    else {
	soap_sprint_fault(soap, errorMessage, MAX_ERROR_MESSAGE_LENGTH);
	result = DIGS_NO_CONNECTION;
    }
    
    globus_libc_free(path);
    srmDone(hostname, soap);
    return result;
}


/***********************************************************************
 * void digs_free_string_array_srm(char ***arrayOfStrings,
 *                                 int *listLength);
 *
 * Free an array of strings according to the correct implementation.
 * 
 * Parameters:                                                [I/O]	
 * 
 *   arrayOfStrings   The array of strings to be freed	       I
 *   listLength       The length of the array of filepaths     I
 ***********************************************************************/
void digs_free_string_array_srm(char ***arrayOfStrings, int *listLength)
{
    int i;
    for (i = 0; i < (*listLength); i++) {
	globus_libc_free((*arrayOfStrings)[i]);
    }
    globus_libc_free(*arrayOfStrings);
    *arrayOfStrings = NULL;
    *listLength = 0;
}


/***********************************************************************
 * void digs_housekeeping_srm(char *errorMessage, char *hostname);
 *
 * Perform periodic housekeeping tasks (currently just delete old files
 * from the inbox)
 * 
 * Parameters:                                                 [I/O]	
 * 
 *   errorMessage  Receives error message if an error occurs      O
 *   hostname      Name of host to perform housekeeping on      I
 *
 * Returns DIGS_SUCCESS on success or an error code on failure.
 ***********************************************************************/
digs_error_code_t digs_housekeeping_srm(char *errorMessage,
					char *hostname)
{
    /*
     * We could do this more efficiently by doing a single detailed srmLs
     * to get all the modification times, but this method isn't called
     * very frequently and there shouldn't be many files in the inbox at
     * once, so this way should be fine.
     */
    char **list;
    int listLength;
    digs_error_code_t result = DIGS_SUCCESS;
    int i;
    time_t tm, now;
    
    logMessage(DEBUG, "digs_housekeeping_srm(%s)", hostname);
    
    now = time(NULL);
    
    /* first get list of all files in the inbox */
    result = digs_scanInbox_srm(errorMessage, hostname, &list, &listLength,
				1);
    if (result != DIGS_SUCCESS) {
	return result;
    }
    
    /* loop over them all */
    for (i = 0; i < listLength; i++) {
	/* get mod time for this one */
	result = digs_getModificationTime_srm(errorMessage, list[i],
					      hostname, &tm);
	if (result == DIGS_SUCCESS) {
	    if (difftime(now, tm) > INBOX_DELETION_TIME) {
		logMessage(WARN, "Removing old inbox file %s", list[i]);
		result = digs_rm_srm(errorMessage, hostname, list[i]);
	    }
	}
    }
    
    digs_free_string_array_srm(&list, &listLength);
    
    return result;
}

