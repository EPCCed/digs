/***********************************************************************
 *
 *   Filename:   gridftp.c
 *
 *   Authors:    James Perry, Eilidh Grant     (jamesp, egrant1)   EPCC.
 *
 *   Purpose:    Provides a simple interface to Globus FTP functions to
 *               the other modules
 *
 *   Contents:   Various FTP function wrappers
 *
 *   Used in:    Called by other modules for copying, deleting, getting
 *               information on files and directories
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

#include <globus_ftp_client.h>
#include "gridftp.h"
#include "misc.h"
#include "node.h"
#include "job.h"
#include "gridftp-common.h"

char *TIMEOUT_MESSAGE = "Timed out.";

/***********************************************************************
 *   digs_error_code_t digs_getLength_globus(char *errorMessage, 
 * 		const char *filePath,const char *hostname, long long int *fileLength)
 * 
 * 	Gets a size in bytes of a file on a node.
 * 
 *   Parameters:                                                 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 fileLength		Size of file is stored in variable fileLength, 		O
 * 					or -1 if unsuccessful
 * 
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getLength_globus(char *errorMessage,
		const char *filePath, const char *hostname, long long int *fileLength) {

	errorMessage[0] = '\0';
	logMessage(DEBUG, "digs_getLength_globus(%s,%s)", hostname, filePath);

	long long length;
	globus_result_t err;
	digs_error_code_t digsError;
	ftpTransaction_t *t;

	*fileLength = -1;

	char *urlBuffer;

	int isDir = 0;
	digsError = digs_isDirectory_globus(errorMessage, filePath, hostname, &isDir);
	if (digsError != DIGS_SUCCESS) {
		return digsError;
	}
	if (isDir) {
		return DIGS_FILE_IS_DIR;
	}

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath) < 0) {
		errorExit("Out of memory in digs_getLength_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("get remote file length");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_size(handle, urlBuffer, &t->attr, &t->length,
			completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	int timeOut = getNodeFtpTimeout(hostname);

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	length = t->length;

	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	*fileLength = length;

	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_doesExist_globus (char *errorMessage,
 * const char *filePath, const char *hostname, int *doesExist)
 * 
 * Checks if a file/directory exists on a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 doesExist		1 if the specified file exists, else 0				O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_doesExist_globus(char *errorMessage,
		const char *filePath, const char *hostname, int *doesExist) {

	logMessage(DEBUG, "digs_doesExist_globus(%s,%s)", hostname, filePath);
	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;
	*doesExist = GLOBUS_FALSE;
	errorMessage[0] = '\0';

	logMessage(DEBUG, "digs_doesExist_globus(%s,%s)", hostname, filePath);

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath)<0) {
		errorExit("Out of memory in doesItExist");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("existence check");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	err = globus_ftp_client_exists(&t->handle, urlBuffer, &t->attr,
			completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);

		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}
	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	timeOut = getNodeFtpTimeout(hostname);
	/*TODO replace fixed timeout with  timeOut = getNodeFtpTimeout(host);*/
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	digs_error_code_t error_code;
	if (t->succeeded) {
		*doesExist = GLOBUS_TRUE;
		error_code = DIGS_SUCCESS;
	} else {
		//TODO will need to add some special error error handling. shouldn't 
		// return an error just because the file didn't exist.
		globus_object_t *error = (globus_object_t *)t->error;
		error_code =  getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	return error_code;
}

/***********************************************************************
 * digs_error_code_t digs_ping_globus(char *errorMessage, const char * hostname);
 * 
 * Pings node and checks that it has a data directory.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I								
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_ping_globus(char *errorMessage, const char * hostname){
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *topDir = safe_strdup(getNodePath(hostname));

	logMessage(DEBUG, "getNodePath (%s)",hostname);
	logMessage(DEBUG, "topDir is (%s)",topDir);
	
	int stringLength = strlen(topDir)+strlen("data10");
	char *dataPath= globus_malloc((stringLength +2)*sizeof(char));
	if (!dataPath) {
		errorExit("Out of memory in digs_ping_globus");
	}
	strncpy(dataPath, topDir, stringLength);
	strncat(dataPath, "/", stringLength);
	strncat(dataPath, "data", stringLength);
	int doesExist = 0;
	result  = digs_doesExist_globus(errorMessage, dataPath, hostname, &doesExist);

	globus_libc_free(dataPath);
	return result;
}

/*Type=dir;Modify=20080512151244;Size=4096;Perm=cfmpel;UNIX.mode=0775;UNIX.owner
 * =eilidh;UNIX.group=eilidh;Unique=fd00-35010a; /home/eilidh/testData*/
char* parseMlst(char *attribute, char *mlst) {

	char *item; //TODO allocate memory to this?

	item = strtok(mlst, ";=.");
	if (item != NULL) {
		if (strcmp(item, attribute)==0) {
			return strtok('\0', ";=.");
		}
		while (item) {
			item = strtok('\0', ";=.");
			if (item && strcmp(item, attribute)==0) {
				return strtok('\0', ";=.");
			}
		}
	}
	return GLOBUS_NULL;
}


/***********************************************************************
 * digs_error_code_t digs_isDirectory_globus (char *errorMessage,
 * const char *filePath, const char *hostname, int *isDirectory)
 * 
 * Checks if a file on a node is a directory.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 isDirectory	1 if the path is to a directory, else 0				O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_isDirectory_globus(char *errorMessage,
		const char *filePath, const char *hostname, int *isDirectory) {
	
	globus_byte_t byte_t= GLOBUS_NULL;
	globus_byte_t *byte_t_pointer = &byte_t;

	/*A pointer to a globus_byte_t * to be allocated and filled with the MLST 
	 * fact string of the file, if it exists. Otherwise, the value pointed to by 
	 * it is undefined. It is up to the user to free this memory.*/
	globus_byte_t ** mlst_buffer = &byte_t_pointer;
	/*A pointer to a globus_size_t to be filled with the length of the data in 
	 * mlst_buffer.*/
	globus_size_t buffer_length =0;
	globus_size_t * mlst_buffer_length = &buffer_length;

	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;
	*isDirectory = GLOBUS_FALSE;

	logMessage(DEBUG, "digs_isDirectory_globus(%s,%s)", hostname, filePath);

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath)<0) {
		errorExit("Out of memory in digs_isDirectory_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("existence check");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	mlst_buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!mlst_buffer) {
		errorExit("Out of memory in digs_isDirectory_globus");
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_mlst(handle, urlBuffer, &t->attr, mlst_buffer,
			mlst_buffer_length, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		globus_libc_free(mlst_buffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);

		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	timeOut = getNodeFtpTimeout(hostname);
	/*TODO replace fixed timeout with  timeOut = getNodeFtpTimeout(host);*/
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();

	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	char *type = parseMlst("Type", (char *)*mlst_buffer);
	if (type == GLOBUS_NULL){
		
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		strncpy(errorMessage, "Could not parse the file type.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}
	if (strcmp(type, "dir")==0) {
		*isDirectory = GLOBUS_TRUE;
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	globus_libc_free(mlst_buffer);
	return DIGS_SUCCESS;
}

/***********************************************************************
 *   void digs_free_string_globus(char **string) 
 *
 * 	 Free string
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 string 		the string to be freed								I
 ***********************************************************************/
void digs_free_string_globus(char **string) {
	globus_libc_free(*string);
	*string = NULL;
}

/***********************************************************************
 *  digs_error_code_t digs_getOwner_globus(char *errorMessage, const char 
 * *filePath, const char *hostname, char **ownerName);
 * 
 * Gets the owner of the file.
 *
 * Returned owner is stored in variable ownerName.
 * Note that owner should be freed 
 * accordingly to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                                	 	[I/O]
 *	
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)				O
 * 	 filePath 		the full path to the file								I
 *   hostname  		the FQDN of the host to contact          				I
 * 	 ownerName		the owner of the file is stored in variable ownerName. 	O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getOwner_globus(char *errorMessage,
		const char *filePath, const char *hostname, char **ownerName) {
	
	globus_byte_t byte_t= GLOBUS_NULL;
	globus_byte_t *byte_t_pointer = &byte_t;

	/*A pointer to a globus_byte_t * to be allocated and filled with the MLST 
	 * fact string of the file, if it exists. Otherwise, the value pointed to by 
	 * it is undefined. It is up to the user to free this memory.*/
	globus_byte_t ** mlst_buffer = &byte_t_pointer;
	/*A pointer to a globus_size_t to be filled with the length of the data in 
	 * mlst_buffer.*/
	globus_size_t buffer_length =0;
	globus_size_t * mlst_buffer_length = &buffer_length;

	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;

	*ownerName= safe_strdup("");
	errorMessage[0] = '\0';

	logMessage(DEBUG, "digs_getOwner_globus(%s,%s)", hostname, filePath);

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath)<0) {
		errorExit("Out of memory in digs_getOwner_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("existence check");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	mlst_buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!mlst_buffer) {
		errorExit("Out of memory in digs_getOwner_globus");
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_mlst(handle, urlBuffer, &t->attr, mlst_buffer,
			mlst_buffer_length, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		globus_libc_free(mlst_buffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);

		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	timeOut = getNodeFtpTimeout(hostname);
	/*TODO replace fixed timeout with  timeOut = getNodeFtpTimeout(host);*/
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();

	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	char *mlstOwner = parseMlst("owner", (char *)*mlst_buffer);
	if (mlstOwner == GLOBUS_NULL){
		
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		strncpy(errorMessage, "Could not parse the file owner.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}

	logMessage(DEBUG, "owner is %s", mlstOwner);

	globus_libc_free(*ownerName); // Free the empty string.
	*ownerName = safe_strdup(mlstOwner);

	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	globus_libc_free(mlst_buffer);
	return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_getGroup_globus(char *errorMessage,
 * const char *filePath, const char *hostname, char **groupName);
 * 
 * Gets the group of a file on a node.
 *
 * Returned group name is stored in variable groupName.
 * Note that groupName should be freed 
 * accordingly to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 groupName		the group name of the file is stored in variable 	0
 * 					groupName. 									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getGroup_globus(char *errorMessage,
		const char *filePath, const char *hostname, char **groupName) {

	globus_byte_t byte_t= GLOBUS_NULL;
	globus_byte_t *byte_t_pointer = &byte_t;

	/*A pointer to a globus_byte_t * to be allocated and filled with the MLST 
	 * fact string of the file, if it exists. Otherwise, the value pointed to by 
	 * it is undefined. It is up to the user to free this memory.*/
	globus_byte_t ** mlst_buffer = &byte_t_pointer;
	/*A pointer to a globus_size_t to be filled with the length of the data in 
	 * mlst_buffer.*/
	globus_size_t buffer_length =0;
	globus_size_t * mlst_buffer_length = &buffer_length;

	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;

	*groupName = safe_strdup("");
	errorMessage[0] = '\0';

	logMessage(DEBUG, "digs_getGroup_globus(%s,%s)", hostname, filePath);

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath)<0) {
		errorExit("Out of memory in digs_getGroup_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("existence check");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	mlst_buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!mlst_buffer) {
		errorExit("Out of memory in digs_getGroup_globus");
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_mlst(handle, urlBuffer, &t->attr, mlst_buffer,
			mlst_buffer_length, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		globus_libc_free(mlst_buffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);

		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	timeOut = getNodeFtpTimeout(hostname);
	/*TODO replace fixed timeout with  timeOut = getNodeFtpTimeout(host);*/
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();

	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	const char *mlstGroup = parseMlst("group", (char *)*mlst_buffer);
	if (mlstGroup == GLOBUS_NULL){
		
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		strncpy(errorMessage, "Could not parse the file group.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}

	logMessage(DEBUG, "group is %s", mlstGroup);

	globus_libc_free(*groupName);
	*groupName = safe_strdup(mlstGroup);

	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	globus_libc_free(mlst_buffer);
	return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_setGroup_globus(char *errorMessage, 
 * const char *filePath, const char *hostname, char *groupName);
 * 
 * Sets up/changes a group of a file/directory on a node (chgrp).
 * 
 * Relies on chgrp command existing on the storage element.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 groupName		the group name of the file is stored in variable 	I
 * 					groupName. 									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_setGroup_globus(char *errorMessage,
		const char *filePath, const char *hostname, const char *groupName) {  
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *jobName = "/bin/sh";
	char *remoteOutput;
	const int argc = 2;
	char *argv[argc];

	char *chgrp = "chgrp ";
	int strLength = 0;
	strLength += strlen(chgrp);
	strLength += strlen(groupName);
	strLength += 2; // space + end char
	strLength += strlen(filePath);
	
	char *command = globus_libc_malloc(strLength * sizeof(char));
    if (!command)
    {
		errorExit("Out of memory in digs_setGroup_globus");
    }
    strncpy(command, chgrp, strLength);
    strncat(command, groupName, strLength);
    strncat(command, " ", strLength);
    strncat(command, filePath, strLength);

	argv[0] = "-c";
    argv[1] = command;
	
	int i;
	for (i=0; i<argc; i++) {//chgrp qcdgrid largeFile 
		logMessage(DEBUG, "chgrp Remotely: argv[%d] = %s", i, argv[i]);
	}

	/* Perform remote permissions change */
	if (!executeJob(hostname, jobName, argc, argv)) {
		logMessage(WARN,
				"Error executing remote chgrp (%s) in digs_setGroup_globus",
				hostname);
		globus_libc_free(command);
		return DIGS_UNKNOWN_ERROR;
	}
	globus_libc_free(command);

	/* Get remote result TODO I dont think I get any remote output here. */
	remoteOutput = getLastJobOutput();

	logMessage(DEBUG, "Remote output '%s' returned from (%s)", remoteOutput,
			hostname);

	if (strcmp(remoteOutput, "-1") == 0) {
		logMessage(WARN, "Invalid output '%s' returned from (%s)", remoteOutput,
				hostname);
		return DIGS_UNKNOWN_ERROR;
	}
	 
	char *unknownGroup= "unknown group";
	result = digs_getGroup_globus(errorMessage, filePath, hostname, &unknownGroup );
	if (result != DIGS_SUCCESS){
		return result;
	}
	logMessage(DEBUG, "Group changed");
	if (strcmp(unknownGroup, groupName)) {
		logMessage(ERROR,
				"Failed to change group from '%s' to '%s' for file '%s'",
				unknownGroup, groupName, filePath);
		strncpy(errorMessage, "Set group failed.", MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}

	return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_getPermissions_globus(char *errorMessage,
 *		const char *filePath, const char *hostname, char **permissions);
 * 
 * Gets the permissions of a file on a node.
 *
 * Returned permissions are stored in variable permissions, in a numeric
 * representation. For example 754 would mean:	
 * owner: read, write and execute permissions,
 * group: read and execute permissions,
 * others: only read permissions. 
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 permissions	the permissions of the file/dir is stored in 
 * 					variable permissions in octal fomat.				0									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getPermissions_globus(char *errorMessage,
		const char *filePath, const char *hostname, char **permissions) {

	globus_byte_t byte_t= GLOBUS_NULL;
	globus_byte_t *byte_t_pointer = &byte_t;

	/*A pointer to a globus_byte_t * to be allocated and filled with the MLST 
	 * fact string of the file, if it exists. Otherwise, the value pointed to by 
	 * it is undefined. It is up to the user to free this memory.*/
	globus_byte_t ** mlst_buffer = &byte_t_pointer;
	/*A pointer to a globus_size_t to be filled with the length of the data in 
	 * mlst_buffer.*/
	globus_size_t buffer_length =0;
	globus_size_t * mlst_buffer_length = &buffer_length;

	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;

	*permissions = globus_libc_malloc((MAX_PERMISSIONS_LENGTH +1) * sizeof(char));
	(*permissions)[0] = '\0';

	errorMessage[0] = '\0';

	logMessage(DEBUG, "digs_getPermissions_globus(%s,%s)", hostname, filePath);

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath)<0) {
		errorExit("Out of memory in digs_getPermissions_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("existence check");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	mlst_buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!mlst_buffer) {
		errorExit("Out of memory in digs_getPermissions_globus");
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_mlst(handle, urlBuffer, &t->attr, mlst_buffer,
			mlst_buffer_length, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		globus_libc_free(mlst_buffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	timeOut = getNodeFtpTimeout(hostname);
	/*TODO replace fixed timeout with  timeOut = getNodeFtpTimeout(host);*/
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();

	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}
	logMessage(DEBUG, "mlst_buffer is %s", *mlst_buffer);

	char *perms = parseMlst("mode", (char *)*mlst_buffer);
	if (perms == GLOBUS_NULL){
		
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(mlst_buffer);

		strncpy(errorMessage, "Could not parse the file permissions.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}

	logMessage(DEBUG, "mlstMode is %s", perms);
	*permissions = safe_strdup(perms);

	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	globus_libc_free(mlst_buffer);
	return DIGS_SUCCESS;
}
/* Multiply the 'number' by 8 to the power of 'power'*/
int multiplyByEightToThePowerOf(int power, char number){
	int a = 1;
	int i;
	for (i=0;i<power;i++){
		a *=8;
	}
	char *numberString  = globus_malloc(2 * sizeof(char));
	numberString[0] = number;
	numberString[1] = '\0';
	int numberInt = atoi(numberString);
	int result =  a * numberInt;
	globus_free(numberString);
	return result;
}
/*converts an octal string to octal int. Only works for 3 digit octal number 
 * e.g. 0675 */
int convertToOctal(const char *octalString){
	return ((multiplyByEightToThePowerOf(2,octalString[1])) +
		    (multiplyByEightToThePowerOf(1,octalString[2])) +
		    (multiplyByEightToThePowerOf(0,octalString[3])));
}

/***********************************************************************
 * digs_error_code_t digs_setPermissions_globus(char *errorMessage, 
 * const char *filePath, const char *hostname, const char *permissions);
 * 
 * Sets up/changes permissions (chmod) of a file/directory on a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 permissions	the permissions of the file/dir is stored in 
 * 					variable permissions in octal fomat.				I									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_setPermissions_globus(char *errorMessage,
		const char *filePath, const char *hostname, const char *permissions) {
	
	logMessage(DEBUG, "digs_setPermissions_globus(%s,%s, %d)", hostname, filePath, permissions);
	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;
	errorMessage[0] = '\0';

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath)<0) {
		errorExit("Out of memory in digs_setPermissions_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("chmod");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	int octalPermissions = convertToOctal(permissions);
		
	err = globus_ftp_client_chmod(&t->handle,
									urlBuffer, octalPermissions, &t->attr,
									completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);

		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}
	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	timeOut = getNodeFtpTimeout(hostname);
	/*TODO replace fixed timeout with  timeOut = getNodeFtpTimeout(host);*/
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
			globus_object_t *error;
			error = (globus_object_t *)t->error;

			destroyFtpTransaction(t);
			releaseTransactionListMutex();

			return getErrorAndMessageFromGlobus(error, errorMessage);
		}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	return DIGS_SUCCESS;
}
/***********************************************************************
 *  digs_error_code_t digs_getModificationTime_globus(char *errorMessage, 
 *		const char *filePath, const char *hostname, 
 * 		time_t *modificationTime);
 * 
 *     Returns the modification time of a file located on a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 modificationTime	the time of the last modification of the file	0									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getModificationTime_globus(char *errorMessage,
		const char *filePath, const char *hostname,
		time_t *modificationTime){

	errorMessage[0] = '\0';
	
	logMessage(DEBUG, "digs_getModificationTime_globus(%s,%s)", hostname,
			filePath);

	globus_result_t err;
//	digs_error_code_t digsError;
	ftpTransaction_t *t;
	globus_abstime_t modification_time_globus;
//	modification_time = NULL;

	char *urlBuffer;

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath) < 0) {
		errorExit("Out of memory in digs_getModificationTime_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("get modification time of remote file");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_modification_time(handle, urlBuffer, &t->attr,
			&modification_time_globus, completeCallback, t);
	    	
	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	int timeOut = getNodeFtpTimeout(hostname);

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	logMessage(DEBUG,"modification_time_globus.tv_sec %d",modification_time_globus.tv_sec);
	logMessage(DEBUG,"modification_time_globus->tv_nsec %d",modification_time_globus.tv_nsec);
	
	*modificationTime = (time_t)modification_time_globus.tv_sec;
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	return DIGS_SUCCESS;

}


/* Remove the last part of the string including and after the last file
 * separator. */
void removeLastPartOfPath(char *filePath) {
	
	char *lastSlash;
	
	/* Last position of file separator. */
	lastSlash = strrchr(filePath,'/');
	if (lastSlash == NULL){
		/* No file separators in path, leave unchanged. */
	}
	else{
		*lastSlash = '\0'; // end last
	}
}

/***********************************************************************
*   int makePathValid(char *host, char *path)
*
*   Ensures that the path given is a valid name for creating a new file
*   on a remote machine. Creates any subdirectories in the path that
*   don't already exist
*    
*   Parameters:                                                     [I/O]
*
*    host  FQDN of the host to contact                               I
*    path  absolute pathname to check for validity                   I
*    
 *   Returns: 1 on success, 0 on error
 ***********************************************************************/
digs_error_code_t makePathValid(char *errorMessage, const char *host,
		const char *path) {
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *path2;
	//  char *lastSlash;

	logMessage(DEBUG, "makePathValid(%s,%s)", host, path);

	path2 = safe_strdup(path);
	removeLastPartOfPath(path2);
	errorMessage[0] = '\0';

	int doesExist = 0;
	// Check if the path exists
	result = digs_doesExist_globus(errorMessage, path2, host, &doesExist);
	if (!((result == DIGS_SUCCESS)||(result == DIGS_UNSPECIFIED_SERVER_ERROR))) {
		globus_libc_free(path2);
		return result;
	}
	/* If the directory doesn't exist then make it. */
	if (!((DIGS_SUCCESS == result) && doesExist == 1)) {
		//make the dir above this
		result = digs_mkdirtree_globus(errorMessage, host, path2);
		if (result != DIGS_SUCCESS) {
			globus_libc_free(path2);
			return result;
		}
	}

	globus_libc_free(path2);
	return result;
}

/***********************************************************************
 * 
 *digs_error_code_t digs_getChecksum_globus (char *errorMessage, 
 * const char *filePath,const char *hostname, char **fileChecksum,
 * digs_checksum_type_t checksumType)
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
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 fileChecksum	the checksum is stored in variable fileChecksum. 	O
 * 	 checksumType	the checkdigs_checksum_type_t						I
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/

digs_error_code_t digs_getChecksum_globus(char *errorMessage,
		const char *filePath, const char *hostname, char **fileChecksum,
		digs_checksum_type_t checksumType) {

	logMessage(DEBUG, "digs_getChecksum_globus(%s,%s)", filePath, hostname);

	char *checksum=  globus_libc_malloc((CHECKSUM_LENGTH +1) * sizeof(char));

	checksum[0] = '\0';
	errorMessage[0] = '\0';
	globus_result_t err;
	digs_error_code_t digsError;
	*fileChecksum = checksum;
	char algorithm[4];
	char *urlBuffer;
	globus_off_t offset = 0; /*File offset to start calculating checksum.*/
	/*Length of data to read from the starting offset. Use -1 to read the 
	 * entire file.*/
	globus_off_t length = -1;

	if (checksumType == DIGS_MD5_CHECKSUM) {
		strcpy(algorithm, "MD5");
	} else {
		strncpy(errorMessage, "Only the MD5 checksum algorithm is supported.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNSUPPORTED_CHECKSUM_TYPE;
	}

	int isDir = 0;
	digsError = digs_isDirectory_globus(errorMessage, filePath, hostname, &isDir);
	if (digsError != DIGS_SUCCESS) {
		return digsError;
	}
	if (isDir) {
		return DIGS_FILE_IS_DIR;
	}

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath) < 0) {
		errorExit("Out of memory in digs_getChecksum_globus");
	}

	acquireTransactionListMutex();
	ftpTransaction_t *t;
	t = newFtpTransaction("get remote file checksum");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR; 
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	/*	- globus_ftp_client_cksm -- good as it only supports MD5*/
	err = globus_ftp_client_cksm(handle, urlBuffer, &t->attr, *fileChecksum,
			offset, length, algorithm, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);

		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	int timeOut = getNodeFtpTimeout(hostname); //TODO relate to file length

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {

		globus_object_t *error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex(); 
	/*Convert checksum to uppercase. */
	char *p;
	p = *fileChecksum;

	  while( *p ) {
	    *p = toupper(*p);
	    p++;
	  }

	return DIGS_SUCCESS;
}

/***********************************************************************
 *digs_error_code_t digs_startPutTransfer_globus(char *errorMessage,
 *		const char *localPath, const char *hostname, const char *SURL,
 *		int *handle);
 *
 * Start a put operation (push) of a local file to a remote storage element with 
 * a specified remote location (SURL). A handle is returned to uniquely 
 * identify this transfer.
 * If a file with the chosen name already exists at the remote location, it will 
 * be overwritten without a warning.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 * 	 SURL 			the remote location to put the file to				I	
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startPutTransfer_globus(char *errorMessage,
		const char *hostname, const char *localPath, const char *SURL,
		int *handle) {

	logMessage(DEBUG, "digs_startPutTransfer_globus(%s,%s,%s)", localPath,
			hostname, SURL);
	char *urlBuffer;
	globus_result_t err;
	ftpTransaction_t *t;
	*handle = -1;
	errorMessage[0] = '\0';

	/* Construct URL for file */
	/* To avoid the possibility of the main control thread copying the file
	 * while it's still being transferred there, we name the file 
	 * <filename>-LOCKED until it's completely transferred, and the main
	 * thread will leave it alone
	 */
	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s-LOCKED", hostname, SURL)<0) {
		errorExit("Out of memory in digs_startPutTransfer_globus");
	}

	/* Make sure the directory structure exists at the other end */
	err = makePathValid(errorMessage, hostname, SURL);
	if (err != DIGS_SUCCESS) {
		logMessage(WARN,
				"makePathValid(%s,%s) failed in digs_startPutTransfer_globus",
				hostname, SURL);
		globus_libc_free(urlBuffer);
		return err;
	}
	
	acquireTransactionListMutex();
	t = newFtpTransaction("write");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	t->buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!t->buffer) {
		globus_libc_free(urlBuffer);
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		return DIGS_UNKNOWN_ERROR;
	}

	logMessage(DEBUG, "Transfer to path %s", urlBuffer);
	
	/* Set the checksum of the file before it's transferred.*/
	char *localChecksum = "no checksum set";
	getChecksum(localPath, &localChecksum);
	t->checksum = safe_strdup(localChecksum); 
	globus_libc_free(localChecksum);
	
	t->destFilepath = safe_strdup(SURL);
	t->hostname = safe_strdup(hostname);

	/* Start up a put operation */
	err = globus_ftp_client_put(&t->handle, urlBuffer, &t->attr, NULL,
			replicateCompleteCallback, t);

	if (err != GLOBUS_SUCCESS) {
		printError(err, "globus_ftp_client_put");
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	/* and start sending data to FTP module */
	if (!startFtpWrite(localPath, t, errorMessage)) {
		releaseTransactionListMutex();
		globus_ftp_client_abort(&t->handle);
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}
	releaseTransactionListMutex();

	*handle = t->id;

	globus_libc_free(urlBuffer);

	return DIGS_SUCCESS;
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
 *digs_error_code_t digs_startCopyToInbox_globus(char *errorMessage,
 *		const char *hostname, const char *localPath, const char *lfn,
 *		int *handle) 
 * 
 * This method is effectively a wrapper around digs_startPutTransfer()
 * that firstly resolves the target SURL based on the specific 
 * configuration of the SE (identified by its hostname). Not all
 * SEs have Inboxes. If this function is called on an SE that does not
 * have an Inbox, then an appropriate error is returned.
 *
 * A handle is returned to uniquely identify this transfer.
 * 
 * If a file with the chosen name already exists at the remote location, it will 
 * be overwriten without a warning.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 *   lfn    		Logical name for file on grid          			    I
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startCopyToInbox_globus(char *errorMessage,
		const char *hostname, const char *localPath, const char *lfn,
		int *handle) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
    char *flatFilename;
    char *remoteFilename;
	*handle = -1;
	errorMessage[0] = '\0';

	char *inbox = getNodeInbox(hostname);
	if(inbox==NULL){
		/* This storage element doesn't have an inbox. */
		return DIGS_NO_INBOX;
	}
	
	/* To avoid having to maintain a complex directory hierarchy in NEW
	 * as well as outside of it, we substitute the slashes in a new filename
	 * with '-DIR-'
	 */
	flatFilename = substituteSlashes(lfn);
	if (!flatFilename)
	{
	    errorExit("Out of memory in digs_startCopyToInbox_globus");
	}
	/* To avoid the possibility of the main control thread copying the file
	 * while it's still being transferred there, we name the file 
	 * LOCKED-<filename> until it's completely transferred, and the main
	 * thread will leave it alone
	 */
	if (safe_asprintf(&remoteFilename, "%s/%s", inbox, flatFilename) < 0)
	{
	    errorExit("Out of memory in digs_startCopyToInbox_globus");
	}
	globus_libc_free(flatFilename);
	
	result = digs_startPutTransfer_globus(errorMessage, hostname, localPath,
			 remoteFilename, handle);

	globus_libc_free(remoteFilename);
	return result;
}

/***********************************************************************
 *digs_error_code_t digs_startGetTransfer_globus(char *errorMessage,
 *		const char *hostname, const char *SURL, const char *localPath,
 *		int *handle);
 *
 * Starts a get operation (pull) of a file (SURL) from a  
 * remote hostname to local destination file path. A handle is
 * returned to uniquely identify this transfer.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 SURL 			the remote location to get the file from			I	
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startGetTransfer_globus(char *errorMessage,
		const char *hostname, const char *SURL, const char *localPath,
		int *handle) {

	logMessage(DEBUG, "digs_startGetTransfer_globus(%s,%s,%s)", localPath,
			hostname, SURL);
	
	char *urlBuffer;
	globus_result_t err;
	ftpTransaction_t *t;
	*handle = -1;
	errorMessage[0] = '\0';

	/* Set the checksum of the file before it's transferred.*/
	char *remoteChecksum = "no checksum set";
	digs_getChecksum_globus(errorMessage, SURL, hostname, &remoteChecksum,
			DIGS_MD5_CHECKSUM);
	
	/* Construct URL for file */
	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, SURL)<0) {
		errorExit("Out of memory in digs_startGetTransfer_globus");
	}
	
	acquireTransactionListMutex();
	t = newFtpTransaction("read");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	t->buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!t->buffer) {
		globus_libc_free(urlBuffer);
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		return DIGS_UNKNOWN_ERROR;
	}

	logMessage(DEBUG, "Transfer from path %s", urlBuffer);
	
	t->checksum = safe_strdup(remoteChecksum); 
	globus_libc_free(remoteChecksum);

	/* Construct URL for file */
	/* Name the file <filename>-LOCKED until it's completely transferred
	 */
	char *lockedLocalPath;
	if (safe_asprintf(&lockedLocalPath, "%s-LOCKED", localPath)<0) {
		errorExit("Out of memory in digs_startGetTransfer_globus");
	}
	t->destFilepath = safe_strdup(localPath);
	t->hostname = safe_strdup(hostname);

	/* Start up a put operation */
	err = globus_ftp_client_get(&t->handle, urlBuffer, &t->attr, NULL,
			replicateCompleteCallback, t);

	if (err != GLOBUS_SUCCESS) {
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	/* and start sending data to FTP module */
	if (!startFtpRead(lockedLocalPath, t, errorMessage)) {
		globus_ftp_client_abort(&t->handle);
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}
	
	*handle = t->id;
	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);
	globus_libc_free(lockedLocalPath);

	return DIGS_SUCCESS;
}

/***********************************************************************
 *digs_error_code_t digs_monitorTransfer_globus (char *errorMessage, int handle,
 * digs_transferStatus_t *status, int *percentComplete);
 * 
 * Checks a progress of a transfer, identified by a handle, by
 * returning its status. Also returns the percentage of the transfer that has 
 * been completed.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage		an error description string	(expects to have
 * 						MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 			the id of the transfer								I
 * 	 status				the status of the transfer							O	
 * 	 percentComplete	percentage of the transfer that has been completed
 * 						In this implementation goes from 0 directly to 100	O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_monitorTransfer_globus(char *errorMessage, int handle, 
		digs_transfer_status_t *status, int *percentComplete) {

	ftpTransaction_t *t;
	*status = DIGS_TRANSFER_FAILED;
	errorMessage[0] = '\0';
	*percentComplete = 0;

	acquireTransactionListMutex();

	t = findTransaction(handle);

	if (t == NULL) {
		/* couldn't find it at all */
		releaseTransactionListMutex();
		strncpy(errorMessage, "Couldn't find the transaction.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}

	if (!t->done) {
		/* still in progress */
		releaseTransactionListMutex();
		*status = DIGS_TRANSFER_IN_PROGRESS;
		return DIGS_SUCCESS;
	}

	if (t->succeeded) {
		*status = DIGS_TRANSFER_DONE;
		*percentComplete = 100;
	} else {
		*status = DIGS_TRANSFER_FAILED;
		releaseTransactionListMutex();
		return getErrorAndMessageFromGlobus(t->error, errorMessage);
	}

	releaseTransactionListMutex();
	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_endTransfer_globus(char *errorMessage, int handle)
 * 
 * Cleans up a transfer identified by a handle. Checks that the file     
 * has been transferred correctly (using checksum) and removes the 
 * LOCKED postfix if it has. If the transfer has not completed 
 * correctly, removes partially transferred files.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 		the id of the transfer								I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/


digs_error_code_t digs_endTransfer_globus(char *errorMessage, int handle) {
logMessage(DEBUG, "start digs_endTransfer_globus");
	ftpTransaction_t *t;
	errorMessage[0] = '\0';
	digs_error_code_t result= DIGS_UNKNOWN_ERROR;

	acquireTransactionListMutex();

	t = findTransaction(handle);

	if (t == NULL) {
		/* couldn't find it at all */
		releaseTransactionListMutex();
		strncpy(errorMessage, "Couldn't find the transaction.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}
	/* If the transaction wasn't successful return the error*/
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}
	/*check the checksum and remove LOCKED */
	char *actualChecksum = "no checksum set";
	char *lockedFilepath; 
	if (safe_asprintf(&lockedFilepath, "%s-LOCKED", (t->destFilepath))<0) {
		errorExit("Out of memory in digs_endTransfer_globus");
	}
	
	if (t->writing) {//put
		logMessage(DEBUG, "before get checksum in digs_endTransfer_globus");
		releaseTransactionListMutex();
		result = digs_getChecksum_globus(errorMessage, lockedFilepath,
				t->hostname, &actualChecksum, DIGS_MD5_CHECKSUM);
		acquireTransactionListMutex();
		logMessage(DEBUG, "after get checksum in digs_endTransfer_globus");
		if (result != DIGS_SUCCESS) {
			logMessage(WARN, "failed to get checksum in digs_endTransfer_globus");
			destroyFtpTransaction(t);
			releaseTransactionListMutex();
			globus_libc_free(lockedFilepath);
			return result;
		}
	} 
	else {//get
		getChecksum(lockedFilepath, &actualChecksum);
	}

	result = DIGS_SUCCESS;

	if (!strcmp(t->checksum, actualChecksum))// If the checksums match
	{
		// remove -LOCKED extension, return success.
		if (t->writing) {//put
			releaseTransactionListMutex();
			result = digs_mv_globus(errorMessage, t->hostname, lockedFilepath,
					(t->destFilepath));
			acquireTransactionListMutex();
		} else {//get
			//move file on local machine
			rename(lockedFilepath, t->destFilepath);
		}
	} else { // The checksums don't match
		// delete the LOCKED file, return fail.
		if (t->writing) {//put
			releaseTransactionListMutex();
			result = digs_rm_globus(errorMessage, t->hostname, lockedFilepath);
			acquireTransactionListMutex();
		} else {//get
			//TODO remove file on local machine
		}
		logMessage(WARN, "local checksum %s", actualChecksum);
		logMessage(WARN, "remote checksum %s", t->checksum);
		strncpy(errorMessage,
				"The checksums of the local and remote files do not match.", 
				MAX_ERROR_MESSAGE_LENGTH);
		if (result == DIGS_SUCCESS) { // Make sure an error is returned.
			result = DIGS_INVALID_CHECKSUM;
		}
	}
	
	destroyFtpTransaction(t);
	releaseTransactionListMutex();
	globus_libc_free(lockedFilepath);
	return result;
}

/***********************************************************************
 * digs_error_code_t digs_cancelTransfer_globus(char *errorMessage, int handle)
 * 
 * Cancels and cleans up a transfer identified by a handle. Removes any
 * partially or completely transferred files.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 		the id of the transfer								I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_cancelTransfer_globus(char *errorMessage, int handle){
	ftpTransaction_t *t;
	errorMessage[0] = '\0';
	digs_error_code_t result= DIGS_UNKNOWN_ERROR;
	
	acquireTransactionListMutex();
	char *hostname;
	char *destFilepath;

	t = findTransaction(handle);

	if (t == NULL) {
		/* couldn't find it at all */
		releaseTransactionListMutex();
		strncpy(errorMessage, "Couldn't find the transaction.",
				MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_UNKNOWN_ERROR;
	}
	
	/* Cancel the transfer. */
	if (globus_ftp_client_abort(&t->handle) != GLOBUS_SUCCESS) {
		logMessage(WARN, "Error aborting FTP operation %s", t->opName);
	}

	/* Remove the file if it has been created. */
	char *lockedFilepath;
	if (safe_asprintf(&lockedFilepath, "%s-LOCKED", (t->destFilepath))<0) {
		errorExit("Out of memory in digs_cancelTransfer_globus");
	}
    logMessage(DEBUG, "lockedFilepath(%s)",lockedFilepath);

	result = DIGS_SUCCESS;
	
	hostname = safe_strdup(t->hostname);
	destFilepath = safe_strdup(t->destFilepath);

	int isWriting = t->writing;//The callback will destroy the transaction.
	t->waiting = 0;
	releaseTransactionListMutex();

	if (isWriting) {//put
		/* remove locked file */
		digs_rm_globus(errorMessage, hostname, lockedFilepath);
		/* remove unlocked file */
		digs_rm_globus(errorMessage, hostname, destFilepath);
		strncpy(errorMessage, "", MAX_ERROR_MESSAGE_LENGTH);
	} else {//get
		// remove files in cancelled get.
		remove(lockedFilepath);
		remove(destFilepath);
	}
	
	globus_libc_free(lockedFilepath);
	globus_libc_free(hostname);
	globus_libc_free(destFilepath);
	return result;
}

/***********************************************************************
 * digs_error_code_t digs_mkdir_globus(char *errorMessage, const char *hostname,
		const char *filePath);
 * 
 * Creates a directory on a node.
 * Will return an error if the directory already exists.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePath 		the path of the directory to be created				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mkdir_globus(char *errorMessage, const char *hostname,
		const char *filePath){

	char *urlBuffer;
	globus_result_t err;
	ftpTransaction_t *t;
	errorMessage[0] = '\0';

	/* Construct URL for file */
	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath) < 0) {
		errorExit("Out of memory in digs_mkdir_globus");
	}
	
	acquireTransactionListMutex();
	t = newFtpTransaction("mkdir");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	t->buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!t->buffer) {
		globus_libc_free(urlBuffer);
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		return DIGS_UNKNOWN_ERROR;
	}

	logMessage(DEBUG, "make dir %s", urlBuffer);

	/* mkdir */
	err = globus_ftp_client_mkdir( &t->handle, urlBuffer, &t->attr,
			completeCallback, t);
	
	if (err != GLOBUS_SUCCESS) {
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	globus_libc_free(urlBuffer);
	
	int timeOut = getNodeFtpTimeout(hostname);
	releaseTransactionListMutex();
	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_mkdirtree_globus(char *errorMessage, 
 * const char *hostname, const char *filePath);
 * 
 * Creates a tree of directories at one go on a node 
 * (e.g. 'mkdir lnf:/ukqcd/DWF')
 * Will return an error if the directory already exists.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePath 		the path of the directory to be created				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mkdirtree_globus(char *errorMessage,
		const char *hostname, const char *filePath ) {

	int doesExist = 0;
	// Strip of last of path
	char *shorterPath;
	shorterPath = safe_strdup(filePath);
	removeLastPartOfPath(shorterPath);
	errorMessage[0] = '\0';

	// Check if the start of the path exists
	digs_error_code_t error = digs_doesExist_globus(errorMessage, shorterPath,
			hostname, &doesExist);
	if (!((error == DIGS_SUCCESS)||(error == DIGS_UNSPECIFIED_SERVER_ERROR))){
		globus_libc_free(shorterPath);
		return error;
	}
	/* If the directory above this doesn't exist then make it. */
	if (!((DIGS_SUCCESS == error) && doesExist == 1)) {
		//make the dir above this
		error = digs_mkdirtree_globus(errorMessage, hostname, shorterPath);
		if(error != DIGS_SUCCESS){
			globus_libc_free(shorterPath);
			return error;
		}
	}
	// mkdir
	globus_libc_free(shorterPath);
	error = digs_mkdir_globus(errorMessage, hostname, filePath);
	return error;
}

/***********************************************************************
 * digs_error_code_t digs_copy_globus(char *errorMessage, const char *hostname, 
 * const char *filePathFrom, const char *filePathTo);
 * 
 * Copies a file on an SE (hostname). 
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePathFrom	the path to get the file from						I	
 * 	 filePathTo		the path put the file to							I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_copy_globus(char *errorMessage, const char *hostname, 
  const char *filePathFrom, const char *filePathTo){
	errorMessage[0] = '\0';
	logMessage(DEBUG, "digs_copy_globus(%s, %s, %s)", hostname, filePathFrom,
			filePathTo);

	globus_result_t err;
	digs_error_code_t digsError;
	ftpTransaction_t *t;

	char *source_url;
	char *dest_url;

	int isDir = 0;
	digsError = digs_isDirectory_globus(errorMessage, filePathFrom, hostname,
			&isDir);
	if (digsError != DIGS_SUCCESS) {
		return digsError;
	}
	if (isDir) {
		return DIGS_FILE_IS_DIR;
	}

	if (safe_asprintf(&source_url, "gsiftp://%s%s", hostname, filePathFrom) < 0) {
		errorExit("Out of memory in digs_copy_globus");
	}
	if (safe_asprintf(&dest_url, "gsiftp://%s%s", hostname, filePathTo) < 0) {
		errorExit("Out of memory in digs_copy_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("move file");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(source_url);
		globus_libc_free(dest_url);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;
	//source_url = "gsiftp://qcdgrid4.epcc.ed.ac.uk/home/eilidh/testData/NEW/myDir-DIR-anotherDir-DIR-file1.txt"
	//source_url = safe_strdup("gsiftp://qcdgrid3.epcc.ed.ac.uk/home/eilidh/hopeCopyWorks");
	
	err = globus_ftp_client_third_party_transfer(handle, source_url,
			 &t->attr, dest_url, &t->attr2, NULL, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(source_url);
		globus_libc_free(dest_url);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(source_url);
	globus_libc_free(dest_url);

	int timeOut = getNodeFtpTimeout(hostname);

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_mv_globus(char *errorMessage, const char *hostname, 
 * const char *filePathFrom, const char *filePathTo);
 * 
 * Moves/Renames a file on an SE (hostname). 
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePathFrom	the path to get the file from						I	
 * 	 filePathTo		the path put the file to							I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mv_globus(char *errorMessage, const char *hostname, 
  const char *filePathFrom, const char *filePathTo){
	errorMessage[0] = '\0';
	logMessage(DEBUG, "digs_mv_globus(%s, %s, %s)", hostname, filePathFrom,
			filePathTo);

	globus_result_t err;
	digs_error_code_t digsError;
	ftpTransaction_t *t;

	char *source_url;
	char *dest_url;

	int isDir = 0;
	digsError = digs_isDirectory_globus(errorMessage, filePathFrom, hostname,
			&isDir);
	if (digsError != DIGS_SUCCESS) {
		return digsError;
	}
	if (isDir) {
		return DIGS_FILE_IS_DIR;
	}

	if (safe_asprintf(&source_url, "gsiftp://%s%s", hostname, filePathFrom) < 0) {
		errorExit("Out of memory in digs_mv_globus");
	}
	if (safe_asprintf(&dest_url, "gsiftp://%s%s", hostname, filePathTo) < 0) {
		errorExit("Out of memory in digs_mv_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("move file");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(source_url);
		globus_libc_free(dest_url);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;
	
	err = globus_ftp_client_move(handle, source_url, dest_url, &t->attr, completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(source_url);
		globus_libc_free(dest_url);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(source_url);
	globus_libc_free(dest_url);

	int timeOut = getNodeFtpTimeout(hostname);

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_rm_globus(char *errorMessage, const char *hostname, 
 * const char *filePath);
 * 
 * Deletes a file from a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePath 		the path of the directory to be removed				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rm_globus(char *errorMessage, const char *hostname,
		const char *filePath){
	errorMessage[0] = '\0';
	logMessage(DEBUG, "digs_rm_globus(%s, %s)", hostname,filePath);

	globus_result_t err;
	digs_error_code_t digsError;
	ftpTransaction_t *t;

	char *urlBuffer;

	int isDir = 0;
	digsError = digs_isDirectory_globus(errorMessage, filePath, hostname,
			&isDir);
	if (digsError != DIGS_SUCCESS) {
		return digsError;
	}
	if (isDir) {
		return DIGS_FILE_IS_DIR;
	}

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, filePath) < 0) {
		errorExit("Out of memory in digs_rm_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("remove file");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_delete(handle, urlBuffer, &t->attr,
			completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	int timeOut = getNodeFtpTimeout(hostname);

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_rmdir_globus(char *errorMessage, const char *hostname, 
 * const char *filePath);
 * 
 * Deletes a directory from a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 dirPath 		the path of the directory to be removed				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rmdir_globus(char *errorMessage, const char *hostname, 
  const char *dirPath){
	errorMessage[0] = '\0';
	logMessage(DEBUG, "digs_rmdir_globus(%s, %s)", hostname, dirPath);

	globus_result_t err;
	ftpTransaction_t *t;

	char *urlBuffer;

	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, dirPath) < 0) {
		errorExit("Out of memory in digs_rmdir_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("remove directory");
	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_rmdir(handle, urlBuffer, &t->attr,
			completeCallback, t);

	if (err != GLOBUS_SUCCESS) {

		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);

		/* Turn the error code into a Globus object */
		globus_object_t *errorObject;
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);

	int timeOut = getNodeFtpTimeout(hostname);

	if (!waitOnFtp(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {
		globus_object_t *error;
		error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	return DIGS_SUCCESS;
}

/***********************************************************************
 *digs_error_code_t getList(char *errorMessage,
 *	const char *hostname, const char *dir, char **oneDirlist)
 * 
 * Gets a listing of the files and dirs directly under the dir.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 dir			the directory to be listed							I
 *   list    		machine readable ls of the dir				 	    O		
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t getListFromGlobus(char *errorMessage,
		const char *hostname, const char *dir, char **oneDirlist) {

	char *urlBuffer; /* buffer holding the gsiftp URL to check   */
	ftpTransaction_t *t;
	globus_result_t err;
	float timeOut;
	
	if (safe_asprintf(&urlBuffer, "gsiftp://%s%s", hostname, dir)<0) {
		errorExit("Out of memory in digs_scanNode_globus");
	}

	acquireTransactionListMutex();
	t = newFtpTransaction("readToBuffer");

	if (!t) {
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		return DIGS_UNKNOWN_ERROR;
	}

	/* The Globus FTP handle for this transaction */
	globus_ftp_client_handle_t *handle= &t->handle;

	err = globus_ftp_client_machine_list(handle, urlBuffer, &t->attr,
			replicateCompleteCallback, t);

	if (err != GLOBUS_SUCCESS) {
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		globus_libc_free(urlBuffer);
		globus_object_t *errorObject;

		/* Turn the error code into a Globus object */
		errorObject = globus_error_get(err);
		return getErrorAndMessageFromGlobus(errorObject, errorMessage);
	}

	releaseTransactionListMutex();
	globus_libc_free(urlBuffer);
	
	t->buffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!t->buffer) {
		globus_libc_free(urlBuffer);
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		errorExit("Out of memory in digs_scanNode_globus");
	}
	t->bigBuffer = globus_libc_malloc(FTP_DATA_BUFFER_SIZE);
	if (!t->bigBuffer) {
		globus_libc_free(urlBuffer);
		destroyFtpTransaction(t);
		releaseTransactionListMutex();
		errorExit("Out of memory in digs_scanNode_globus");
	}
	t->bigBuffer[0]='\0';
		    
	releaseTransactionListMutex();
	
	/* and start receiving data */
	if (!startFtpReadToBuffer(t)) {
		releaseTransactionListMutex();
		globus_ftp_client_abort(&t->handle);
		return 0;
	}
	releaseTransactionListMutex();

	timeOut = getNodeCopyTimeout(hostname);

	/* Wait for it to complete */
	if (!waitOnFtpLong(t, timeOut)) {
		/* timed out */
		strncpy(errorMessage, TIMEOUT_MESSAGE, MAX_ERROR_MESSAGE_LENGTH);
		return DIGS_NO_RESPONSE;
	}

	acquireTransactionListMutex();
	if (!t->succeeded) {

		globus_object_t *error = (globus_object_t *)t->error;

		destroyFtpTransaction(t);
		releaseTransactionListMutex();

		return getErrorAndMessageFromGlobus(error, errorMessage);
	}

	*oneDirlist = safe_strdup(t->bigBuffer);
	
	destroyFtpTransaction(t);
	releaseTransactionListMutex();

	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_rmr_globus(char *errorMessage, const char *hostname, 
 * const char *filePath);
 * 
 * Deletes a directory with subdirectories (recursively) from a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 dirPath 		the path of the directory to be removed				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rmr_globus(char *errorMessage, const char *hostname,
		const char *dirPath) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	logMessage(DEBUG, "digs_rmr_globus(%s,%s)", hostname, dirPath);
	
	/* If it's a file instead of a dir, just delete it. */
	int isDir = 0;
	result = digs_isDirectory_globus(errorMessage, dirPath, hostname, &isDir);
	if (result != DIGS_SUCCESS) {
		return result;
	}	
	if(!isDir){
		return digs_rm_globus(errorMessage, hostname, dirPath);
	}

	char **oneDirList;
	oneDirList = globus_malloc(sizeof(char**));
	result = getListFromGlobus(errorMessage, hostname, dirPath, oneDirList);
	if (result != DIGS_SUCCESS) {
		return result;
	}

	char *row;
	char **tempList; // Just to help with parsing.
	int lengthOfTempList = 0;
	tempList = globus_malloc(sizeof(*tempList));
	row = strtok(*oneDirList, "\n");
	/* for each row */
	while (row) {
		/* Add item to temp list */
		lengthOfTempList++;
		tempList = globus_realloc(tempList,((lengthOfTempList)* sizeof(*tempList)));
		tempList[lengthOfTempList-1] = safe_strdup(row);

		row = strtok('\0', "\n");
	}

	char *type, *tmp;
	char *lastItem, *lastItem2 = NULL;
	char *parsedItem;
	int i;
	for (i=0; i<lengthOfTempList; i++) {
		/*getType of item */
	        tmp = safe_strdup(tempList[i]);
		type = parseMlst("Type", tmp);

		if (!strcmp(type, "file")) {
			/* If it's a file remove it*/
			parsedItem = strtok(tempList[i], "; ");
			while (parsedItem) {
				lastItem2 = parsedItem;
				parsedItem = strtok('\0', "; ");
			}
			lastItem = safe_strdup(lastItem2);
			lastItem[strlen(lastItem)-1]='\0'; //remove the line break after file name
			int stringLength = strlen(dirPath)+strlen(lastItem);
			char *filePath= globus_malloc((stringLength + 2)*sizeof(char));
			if (!filePath) {
				errorExit("Out of memory in digs_rmr_globus");
			}
			strncpy(filePath, dirPath, stringLength);
			strncat(filePath, "/", stringLength);
			strncat(filePath, lastItem, stringLength);

			result = digs_rm_globus(errorMessage, hostname, filePath);
			globus_free(lastItem);
			if (result != DIGS_SUCCESS) {
				return result;
			}

		} //else if type == dir
		else if (!strcmp(type, "dir")) {
			/* If it's a dir get the list of files below it. */
			parsedItem = strtok(tempList[i], "; ");
			while (parsedItem) {
				lastItem2 = parsedItem;
				parsedItem = strtok('\0', "; ");
			}
			lastItem = safe_strdup(lastItem2);
			lastItem[strlen(lastItem)-1]='\0'; //remove the line break after file name
			int stringLength = strlen(dirPath)+strlen(lastItem);
			char *filePath= globus_malloc((stringLength + 2)*sizeof(char));
			if (!filePath) {
				errorExit("Out of memory in digs_rmr_globus");
			}
			strncpy(filePath, dirPath, stringLength);
			strncat(filePath, "/", stringLength);
			strncat(filePath, lastItem, stringLength);
			logMessage(DEBUG, "****name of dir is (%s)", filePath);

			globus_free(lastItem);
			
			result = digs_rmr_globus(errorMessage, hostname,
					filePath);
			globus_free(filePath);
			if (result != DIGS_SUCCESS) {
				return result;
			}
		}
		globus_libc_free(tmp);
	}
	// tidy up
	globus_free(*oneDirList);
	globus_free(oneDirList);
	digs_free_string_array_globus(&tempList, &lengthOfTempList);

	//Should have emptied dir by now
	return digs_rmdir_globus(errorMessage, hostname, dirPath);
}

/***********************************************************************
 *digs_error_code_t digs_copyFromInbox_globus(char *errorMessage, 
 * const char *hostname, const char *lfn, const char *targetPath);
 * 
 * The function is effectively a wrapper to digs_mv operation.
 * 
 * Moves a file (identified by lfn) from the Inbox folder to
 * permanent storage on an SE (identified by hostname), with a 
 * specified destination file path. 
 * Not all SEs have Inboxes. If this function is called on an SE that 
 * does not have an Inbox, then an appropriate error is returned.
 * 
 * If a file with the chosen name already exists at the remote location, it 
 * will be overwriten without a warning.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 *   lfn    		Logical name for file on grid          			    I
 * 	 targetPath 	the full path to the SE location to put the 
 * 					file to.											I		
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_copyFromInbox_globus(char *errorMessage,
		const char *hostname, const char *lfn, const char *targetPath){

	char *from;
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
    char *flatFilename;
	errorMessage[0] = '\0';

	/* This storage element doesn't have an inbox. */
	char *inbox = getNodeInbox(hostname);
	if(inbox==NULL){
		return DIGS_NO_INBOX;
	}
	
	/* To avoid having to maintain a complex directory hierarchy in NEW
	 * as well as outside of it, we substitute the slashes in a new filename
	 * with '-DIR-'
	 */
	flatFilename = substituteSlashes(lfn);
	if (!flatFilename)
	{
	    errorExit("Out of memory in digs_copyFromInbox_globus");
	}

	if (safe_asprintf(&from, "%s/%s", inbox, flatFilename) < 0)
	{
	    errorExit("Out of memory in digs_copyFromInbox_globus");
	}
	globus_libc_free(flatFilename);
	result = digs_copy_globus(errorMessage, hostname, from, targetPath);
	globus_libc_free(from);
	
	return result;
}

/***********************************************************************
 * digs_error_code_t getListOfFilesBelowThisDir(char *errorMessage,
 *	 const char *hostname, path_s *topDir, char ***list, int *listLength,
 * 	 int allFiles)
 * 
 * Add the files under this directory to the list of files. Maintain list
 * of dirs that are still to be listed.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 topDir			the directory to be listed							I
 *   list    		An array of the filepaths below the top dir. 	    O	
 *   listLength   	The length of the array of filepaths			    O		
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t getListOfFilesBelowThisDir(char *errorMessage,
		const char *hostname, const char *topDir, char ***listOfFiles,
		int *listLength, int allFiles) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;	
	char **oneDirList;
	oneDirList = globus_malloc(sizeof(char**));

	logMessage(DEBUG, "digs_scanNode_globus(%s,%s)", hostname, topDir);
	result = getListFromGlobus(errorMessage, hostname, topDir, oneDirList);
	if (result != DIGS_SUCCESS){
		return result;
	}
	
	char *row;
	char **tempList; // Just to help with parsing.
	int lengthOfTempList = 0;
	tempList = globus_malloc(sizeof(*tempList));
	row = strtok(*oneDirList, "\n");
	/* for each row */
	while (row) {
		/* Add item to temp list */
		lengthOfTempList++;
		tempList = globus_realloc(tempList,((lengthOfTempList)* sizeof(*tempList)));
		tempList[lengthOfTempList-1] = safe_strdup(row);

		row = strtok('\0', "\n");
	}
	
	char *type, *tmp;
	char *lastItem, *lastItem2 = NULL;
	char *parsedItem;
	int i;
	for (i=0; i<lengthOfTempList; i++) {
		/*getType of item */
	        tmp = safe_strdup(tempList[i]);
		type = parseMlst("Type", tmp);

		if (!strcmp(type, "file")) {
			/* If it's a file add to the list of files */
			parsedItem = strtok(tempList[i], "; ");
			while (parsedItem) {
				lastItem2 = parsedItem;
				parsedItem = strtok('\0', "; ");
			}
			lastItem = safe_strdup(lastItem2);
			lastItem[strlen(lastItem)-1]='\0'; //remove the line break after file name
			
			int isLockedFile = 0;
			char *lockedSuffix = "-LOCKED";
			if(strstr(lastItem, lockedSuffix)!=NULL){
				isLockedFile = 1;
			}
			
			if (allFiles || (!isLockedFile)) { //Filter out locked files
				int stringLength = strlen(topDir)+strlen(lastItem);
				char *filePath= globus_malloc((stringLength + 2)*sizeof(char));
				if (!filePath) {
					errorExit("Out of memory in scanNode");
				}
				strncpy(filePath, topDir, stringLength);
				strncat(filePath, "/", stringLength);
				strncat(filePath, lastItem, stringLength);

				(*listLength)++;
				(*listOfFiles) = globus_realloc((*listOfFiles), *listLength * sizeof(char**));
				(*listOfFiles)[*listLength-1] = filePath;
			}
			globus_free(lastItem);
		} 
		else if (!strcmp(type, "dir")) {
			/* If it's a dir get the list of files below it. */
			parsedItem = strtok(tempList[i], "; ");
			while (parsedItem) {
				lastItem2 = parsedItem;
				parsedItem = strtok('\0', "; ");
			}
			lastItem = safe_strdup(lastItem2);
			lastItem[strlen(lastItem)-1]='\0'; //remove the line break after file name

			int stringLength = strlen(topDir)+strlen(lastItem);
			char *filePath = globus_malloc((stringLength + 2)*sizeof(char));
			if (!filePath) {
				errorExit("Out of memory in scanNode");
			}
			strncpy(filePath, topDir, stringLength);
			strncat(filePath, "/", stringLength);
			strncat(filePath, lastItem, stringLength);
			
			globus_free(lastItem);
			result = getListOfFilesBelowThisDir(errorMessage, hostname,
					filePath, listOfFiles, listLength, allFiles);
			globus_free(filePath);
			if (result != DIGS_SUCCESS) {
				return result;
			}
		}	
		globus_libc_free(tmp);
	}

	// tidy up
	globus_free(*oneDirList);
	globus_free(oneDirList);
	digs_free_string_array_globus(&tempList, &lengthOfTempList);
	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_scanNode_globus(char *errorMessage, 
 * const char *hostname, char ***list, int *listLength, int allFiles);
 * 
 * Gets a recursive list of all the files below the inbox.
 * Setting the allFiles flag to zero will hide any temporary (locked) 
 * files.
 *
 * Returned an array of the file paths is stored in variable list. Note    
 * that list should be freed accordingly to the way it was allocated in 
 * the implementation! Use digs_free_string_array.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 *   list    		An array of the filepaths below the top dir. 	    O	
 *   listLength   	The length of the array of filepaths			    O	
 * 	 allFiles		Show all files or hide any temporary (locked files) I	
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_scanNode_globus(char *errorMessage,
		const char *hostname, char ***list, int *listLength, int allFiles) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;	
	errorMessage[0] = '\0';
	
	/* Setup list of files. */
	*listLength = 0;
	char **listOfFiles;
	listOfFiles = globus_malloc(sizeof(char**));
	*list = listOfFiles;
	

	char *topDir = safe_strdup(getNodePath(hostname));

	logMessage(DEBUG, "getNodePath (%s)",hostname);
	logMessage(DEBUG, "topDir is (%s)",topDir);
	
	int stringLength = strlen(topDir)+strlen("path1");
	char *dataPath= globus_malloc((stringLength +2)*sizeof(char));
	if (!dataPath) {
		errorExit("Out of memory in scanNode");
	}
	strncpy(dataPath, topDir, stringLength);
	strncat(dataPath, "/", stringLength);
	strncat(dataPath, "data", stringLength);
	
	char *dataDir;
	dataDir = safe_strdup(dataPath);

	int doesExist = 0;
	int i=1;
	do{
		result = getListOfFilesBelowThisDir(errorMessage, hostname, dataDir, list,
				listLength, allFiles);
		globus_free(dataDir);
		if (safe_asprintf(&dataDir, "%s%d", dataPath, i) < 0) {
			errorExit("Out of memory in digs_scanNode_globus");
		}

		i++;
		// Check if the next path exists
		result = digs_doesExist_globus(errorMessage, dataDir, hostname, &doesExist);
		if (!((result == DIGS_SUCCESS)||(result == DIGS_UNSPECIFIED_SERVER_ERROR))) {
			globus_free(dataDir);
			globus_free(topDir);
			return result;
		}
	}while((DIGS_SUCCESS == result) && (doesExist == 1));// next data dir exists.
	
	globus_free(topDir);
	return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_scanInbox_globus(char *errorMessage, 
 * const char *hostname, char ***list, int *listLength, int allFiles);
 * 
 * Gets a list of all the DiGS file locations in the inbox. Recursive directory
 * listing of all the files under inbox.
 * Setting the allFiles flag to zero will hide any temporary (locked) 
 * files.
 *
 * Returned an array of the file paths is stored in variable list. Note    
 * that list should be freed accordingly to the way it was allocated in 
 * the implementation! Use digs_free_string_array.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 *   list    		An array of the filepaths below the top dir. 	    O	
 *   listLength   	The length of the array of filepaths			    O	
 * 	 allFiles		Show all files or hide any temporary (locked files) I	
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_scanInbox_globus(char *errorMessage,
		const char *hostname, char ***list, int *listLength, int allFiles) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;	
	errorMessage[0] = '\0';
	
	/* Setup list of files. */
	*listLength = 0;
	char **listOfFiles;
	listOfFiles = globus_malloc(sizeof(char**));
	*list = listOfFiles;

	char *inbox = getNodeInbox(hostname);
	if(inbox==NULL){
		/* This storage element doesn't have an inbox. */
		return DIGS_NO_INBOX;
	}
	
	logMessage(DEBUG, "scan inbox (%s,%s)",hostname, inbox);

	return getListOfFilesBelowThisDir(errorMessage, hostname, inbox, list,
			listLength, allFiles);
}

/***********************************************************************
 * 	void digs_free_string_array_globus(char ***arrayOfStrings, int *listLength);
 *
 * 	Free an array of strings according to the correct implementation.
 * 
 *   Parameters:                                                 [I/O]	
 * 
 * 	 arrayOfStrings The array of strings to be freed	 		  I	
 *   listLength   	The length of the array of filepaths		  I/O
 ***********************************************************************/
void digs_free_string_array_globus(char ***arrayOfStrings, int *listLength) {
    int i;

    for (i = 0; i<(*listLength); i++) {
	globus_free((*arrayOfStrings)[i]);
	(*arrayOfStrings)[i] = NULL;
    }
    globus_free(*arrayOfStrings);
    *arrayOfStrings = NULL;
    *listLength = 0;
}

/***********************************************************************
 * void digs_housekeeping_globus(char *errorMessage, char *hostname);
 *
 * Perform periodic housekeeping tasks (currently just delete stray
 * -LOCKED files)
 * 
 * Parameters:                                                 [I/O]	
 * 
 *   errorMessage  Receives error message if an error occurs      O
 *   hostname      Name of host to perform housekeeping on      I
 *
 * Returns DIGS_SUCCESS on success or an error code on failure.
 ***********************************************************************/
digs_error_code_t digs_housekeeping_globus(char *errorMessage, char *hostname)
{
    char **list = NULL;
    int listLength = 0;
    char *topdir;
    int i;
    digs_error_code_t result;

    errorMessage[0] = '\0';

    logMessage(WARN, "Running housekeeping for %s", hostname);

    /*
     * get list of all files on node, including locked ones
     *
     * can't use scanNode here because we need to know exactly which data
     * directory each locked file is in. Recursively list the level above
     * the data directories
     */
    topdir = getNodePath(hostname);
    if (!topdir)
    {
	return DIGS_NO_SERVICE;
    }

    result = getListOfFilesBelowThisDir(errorMessage, hostname, topdir, &list,
					&listLength, 1);
    if (result != DIGS_SUCCESS)
    {
	return result;
    }

    /* check each file to see if it's a locked one */
    for (i = 0; i < listLength; i++)
    {
	/* only do files inside data directory */
	if (strstr(list[i], "/data") != NULL)
	{
	    int l = strlen(list[i]);
	    if (!strcmp(&list[i][l - 7], "-LOCKED"))
	    {
		time_t modtime;

		logMessage(WARN, "Found locked file %s", list[i]);

		/* found a locked file */
		/* get its modification time */
		result = digs_getModificationTime_globus(errorMessage,
							 list[i], hostname, &modtime);
		if (result != DIGS_SUCCESS)
		{
		    digs_free_string_array_globus(&list, &listLength);
		    return result;
		}

		/* see if it's old enough to safely delete */
		if (difftime(time(NULL), modtime) > 24.0 * 60.0 * 60.0)
		{
		    logMessage(WARN, "Removing %s", list[i]);
		    result = digs_rm_globus(errorMessage, hostname, list[i]);
		    if (result != DIGS_SUCCESS)
		    {
			digs_free_string_array_globus(&list, &listLength);
			return result;
		    }
		}
	    }
	}
    }

    digs_free_string_array_globus(&list, &listLength);
    return DIGS_SUCCESS;
}
