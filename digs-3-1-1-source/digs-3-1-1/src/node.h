/***********************************************************************
*
*   Filename:   node.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Manages information about nodes on the QCDgrid
*
*   Contents:   Function prototypes for node module
*
*   Used in:    Called by other modules to get/set information about
*               grid nodes
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
/*
 * Node management module - deals with maintaining lists of nodes and
 * supplying information on them to other modules. It gets the name of the
 * main node from the local 'nodes.conf' file, then transfers an up-to-date
 * list of the other nodes from there. 'nodeprefs.conf' (another local file)
 * lists the nodes in order of preference for this machine.
 */

#ifndef NODE_H
#define NODE_H

#include "misc.h"
#include "hashtable.h"

/* Types of storage elements.*/
typedef enum {SRM, GLOBUS, OMERO_SE, INVALID_SE_TYPE} storageElementTypes;

/* The storage element interface. */
struct storageElement{
	/*Full name of the storage element. */
	char *name;
	storageElementTypes storageElementType;
    char *site;
    /*Optional path to the inbox. If not defined assume that node has no inbox*/
    char *inbox; 
    char *path;
    long long freeSpace;
    long long *diskQuota;
    int numDisks;
    char *extraRsl;
    char *extraJssContact;

    /*
     * All the timeouts are measured in seconds. ftpTimeout is used for
     * 'short' ftp operations, like making directories or checking
     * existence. copyTimeout is used when copying data files, which
     * may take considerably longer. Defaults are 30 seconds for
     * jobTimeout and ftpTimeout, and 600 seconds for copyTimeout
     */
    float jobTimeout;
    float ftpTimeout;
    float copyTimeout;

    int gpfs;

	/***********************************************************************
	 *   digs_error_code_t  (*digs_getLength)(char *errorMessage, 
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
	digs_error_code_t  (*digs_getLength)(char *errorMessage, const char *filePath,
			const char *hostname, long long int *fileLength);

	/***********************************************************************
	 * 
	 *digs_error_code_t (*digs_getChecksum) (char *errorMessage, 
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

	digs_error_code_t (*digs_getChecksum)(char *errorMessage,
			const char *filePath, const char *hostname, char **fileChecksum,
			digs_checksum_type_t checksumType);

	/***********************************************************************
	 * digs_error_code_t digs_doesExist (char *errorMessage,
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
	digs_error_code_t (*digs_doesExist)(char *errorMessage,
			const char *filePath, const char *hostname, int *doesExist);
	
	/***********************************************************************
	 * digs_error_code_t digs_isDirectory (char *errorMessage,
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
	digs_error_code_t (*digs_isDirectory)(char *errorMessage,
			const char *filePath, const char *hostname, int *isDirectory);

	/***********************************************************************
	 *   void digs_free_string(char **string) 
	 *
	 * 	 Free string
	 * 
	 *   Parameters:                                                	 [I/O]
	 *
	 * 	 string 		the string to be freed								I
	 ***********************************************************************/
	void (*digs_free_string)(char **string);

	/***********************************************************************
	 *  digs_error_code_t (*digs_getOwner)(char *errorMessage, 
	 * const char *filePath, const char *hostname, char **ownerName);
	 * 
	 * Gets the owner of the file.
	 *
	 * Returned owner is stored in variable owner.
	 * Note that owner should be freed 
	 * accordingly to the way it was allocated in the implementation.
	 * Use digs_free_string to free it.
	 * 
	 *   Parameters:                                                	 [I/O]
	 *
	 * 	 errorMessage	an error description string	(expects to have
	 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
	 * 	 filePath 		the full path to the file							I
	 *   hostname  		the FQDN of the host to contact          			I
	 * 	 ownerName		the owner of the file is stored in variable 
	 * 					ownerName. 											O
	 *    
	 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
	 ***********************************************************************/
	digs_error_code_t (*digs_getOwner)(char *errorMessage,
			const char *filePath, const char *hostname, char **ownerName);
	
	/***********************************************************************
	 *  digs_error_code_t (*digs_getGroup)(char *errorMessage, 
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
	digs_error_code_t (*digs_getGroup)(char *errorMessage,
			const char *filePath, const char *hostname, char **groupName);

	/***********************************************************************
	 *  digs_error_code_t (*digs_setGroup)(char *errorMessage, 
	 * const char *filePath, const char *hostname, char *groupName);
	 * 
	 * Sets up/changes a group of a file/directory on a node (chgrp).
	 * 
	 * Globus implementation relies on chgrp command existing on the node.
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
	digs_error_code_t (*digs_setGroup)(char *errorMessage, const char *filePath,
			const char * hostname, const char *groupName);


	/***********************************************************************
	 *  digs_error_code_t (*digs_getPermissions)(char *errorMessage, 
	 *		const char *filePath, const char *hostname, char **permissions);
	 * 
	 * Gets the permissions of a file on a node.
	 *
	 * Returned permissions are stored in variable permissions, in a numberic
	 * representation. For example 754 would mean:	
	 * owner: read, write and execute permissions,
	 * group: read and execute permissions,
	 * others: only read permissions. 
	 * 
	 * Note that permissions should be freed 
	 * according to the way it was allocated in the implementation.
	 * Use digs_free_string to free it.
	 * 
	 *   Parameters:                                                	 [I/O]
	 *
	 * 	 errorMessage	an error description string	(expects to have
	 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
	 * 	 filePath 		the full path to the file							I
	 *   hostname  		the FQDN of the host to contact          			I
	 * 	 permissions	the permissions of the file/dir is stored in 
	 * 					variable permissions in decimal fomat.				0									
	 *    
	 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
	 ***********************************************************************/
	digs_error_code_t (*digs_getPermissions)(char *errorMessage, 
			const char *filePath, const char *hostname, char **permissions);

	/***********************************************************************
	 *  digs_error_code_t (*digs_getModificationTime)(char *errorMessage, 
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
	digs_error_code_t (*digs_getModificationTime)(char *errorMessage,
			const char *filePath, const char *hostname,
			time_t *modificationTime);
	

	/***********************************************************************
	 *digs_error_code_t (*digs_startPutTransfer)(char *errorMessage,
	 *		const char *localPath, const char *hostname, const char *SURL,
	 *		int *handle);
	 *
	 * Start a put operation (push) of a local file to a remote hostname with a 
	 * specified remote location (SURL). A handle is returned to uniquely 
	 * identify this transfer.
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
	 * 	 SURL 			the remote location to put the file to				I	
	 * 	 handle 		the id of the transfer								O					
	 *    
	 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
	 ***********************************************************************/
	digs_error_code_t (*digs_startPutTransfer)(char *errorMessage,
			const char *hostname, const char *localPath, const char *SURL,
			int *handle);

	/***********************************************************************
	 *digs_error_code_t (*digs_startCopyToInbox)(char *errorMessage,
	 *		const char *hostname, const char *localPath, const char *lfn, 
	 * 		int *handle)
	 * 
	 * This method is effectively a wrapper around digs_startPutTransfer()
	 * that firstly resolves the target SURL based on the specific 
	 * configuration of the SE (identified by its hostname). Not all
	 * SEs have Inboxes. If this function is called on an SE that does not
	 * have an Inbox, then an appropriate error is returned.
	 *
	 * A handle is returned to uniquely identify this transfer.
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
	 * 	 localPath 		the full path to the local location to get 
	 * 					the file from										I
	 * 	 handle 		the id of the transfer								O					
	 *    
	 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
	 ***********************************************************************/
	digs_error_code_t (*digs_startCopyToInbox)(char *errorMessage,
			const char *hostname, const char *localPath, const char *lfn,
			int *handle);

	/***********************************************************************
	 *digs_error_code_t (*digs_monitorTransfer) (char *errorMessage, int handle,
	 * digs_transferStatus_t *status, int *percentComplete);
	 * 
	 * Checks a progress of a transfer, identified by a handle, by
	 * returning its status. Also returns the percentage of the transfer that has 
	 * been completed.
	 * 
	 *   Parameters:                                                	 [I/O]
	 *
	 * 	 errorMessage		an error description string	(expects to have
	 * 						MAX_ERROR_MESSAGE_LENGTH assigned already)		O
	 * 	 handle 			the id of the transfer							I
	 * 	 status				the status of the transfer						O	
	 * 	 percentComplete	percentage of the transfer that has been 
	 * 						completed. In this implementation goes from 
	 * 						0 directly to 100								O					
	 *    
	 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
	 ***********************************************************************/
	digs_error_code_t (*digs_monitorTransfer)(char *errorMessage, int handle, 
			digs_transfer_status_t *status, int *percentComplete);

	/***********************************************************************
	 * digs_error_code_t (*digs_endTransfer)(char *errorMessage, int handle)
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
	digs_error_code_t (*digs_endTransfer)(char *errorMessage, int handle);

	/***********************************************************************
	 * digs_error_code_t (*digs_cancelTransfer)(char *errorMessage, int handle)
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
	digs_error_code_t (*digs_cancelTransfer)(char *errorMessage, int handle);

	/***********************************************************************
	 *digs_error_code_t (*digs_startGetTransfer)(char *errorMessage,
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
	digs_error_code_t (*digs_startGetTransfer)(char *errorMessage,
			const char *hostname, const char *SURL, const char *localPath,
			int *handle);
	
	/***********************************************************************
	 * digs_error_code_t (*digs_mkdir)(char *errorMessage, const char *hostname,
	 * 		const char *filePath);
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
	digs_error_code_t (*digs_mkdir)(char *errorMessage, const char *hostname,
			const char *filePath);
	
	/***********************************************************************
	 * digs_error_code_t (*digs_mkdirtree)(char *errorMessage, 
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
	digs_error_code_t (*digs_mkdirtree)(char *errorMessage,
			const char *hostname, const char *filePath);
	
	/***********************************************************************
	 * digs_error_code_t (*digs_mv)(char *errorMessage, const char *hostname, 
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
	digs_error_code_t (*digs_mv)(char *errorMessage, const char *hostname, 
	  const char *filePathFrom, const char *filePathTo);

	/***********************************************************************
	 * digs_error_code_t (*digs_rm)(char *errorMessage, const char *hostname, 
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
	digs_error_code_t (*digs_rm)(char *errorMessage, const char *hostname,
			const char *filePath);

	/***********************************************************************
	 * digs_error_code_t (*digs_rmdir)(char *errorMessage, const char *hostname, 
	 * const char *filePath);
	 * 
	 * Deletes an empty directory from a node.
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
	digs_error_code_t (*digs_rmdir)(char *errorMessage, const char *hostname, 
	  const char *dirPath);

	/***********************************************************************
	 * digs_error_code_t (*digs_rmr)(char *errorMessage, const char *hostname, 
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
	digs_error_code_t (*digs_rmr)(char *errorMessage, const char *hostname,
			const char *filePath);

	/***********************************************************************
	 *digs_error_code_t (*digs_copyFromInbox)(char *errorMessage, 
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
	digs_error_code_t (*digs_copyFromInbox)(char *errorMessage,
			const char *hostname, const char *lfn, const char *targetPath);

	/***********************************************************************
	 * digs_error_code_t (*digs_scanNode)(char *errorMessage,
	 * const char *hostname, char ***list, int *listLength, int allFiles);
	 * 
	 * Gets a list of all the DiGS file locations. Recursive directory
	 * listing of all the DiGS data files
	 * Setting the allFiles flag to zero will hide any temporary (locked) 
	 * files.
	 * 
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
	digs_error_code_t (*digs_scanNode)(char *errorMessage,
			const char *hostname, char ***list, int *listLength, int allFiles);

	/***********************************************************************
	 * digs_error_code_t  (*digs_scanInbox)(char *errorMessage, 
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
	digs_error_code_t (*digs_scanInbox)(char *errorMessage,
			const char *hostname, char ***list, int *listLength, int allFiles);
	
	/***********************************************************************
	 * 	void (*digs_free_string_array)(char ***arrayOfStrings);
	 *
	 * 	Free an array of strings according to the correct implementation.
	 * 
	 *   Parameters:                                                 [I/O]	
	 * 
	 * 	 arrayOfStrings The array of strings to be freed	 		  I	
	 *   listLength   	The length of the array of filepaths		  I/O
	 ***********************************************************************/
	void (*digs_free_string_array)(char ***arrayOfStrings, int *listLength);
	
	/***********************************************************************
	 * digs_error_code_t (*digs_setPermissions)(char *errorMessage, 
	 * const char *filePath,  const char *hostname, const char *permissions);
	 * 
	 * Sets up/changes permissions of a file/directory on a node. 
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
	digs_error_code_t (*digs_setPermissions)(char *errorMessage, 
			const char *filePath, const char *hostname, const char *permissions);

	/***********************************************************************
	 * digs_error_code_t (*(digs_ping)(char *errorMessage, const char * hostname);
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
	digs_error_code_t (*digs_ping)(char *errorMessage, const char * hostname);


        /***********************************************************************
	 * void (*digs_housekeeping)(char *errorMessage, char *hostname);
	 *
	 * Perform periodic housekeeping tasks
	 * 
	 * Parameters:                                                 [I/O]	
	 * 
	 *   errorMessage  Receives error message if an error occurs      O
	 *   hostname      Name of host to perform housekeeping on      I
	 *
	 * Returns DIGS_SUCCESS on success or an error code on failure.
	 ***********************************************************************/
        digs_error_code_t (*digs_housekeeping)(char *errorMessage, char *hostname);

        qcdgrid_hash_table_t *properties;
}storageElement_t;

extern char *tmpDir_;

/*
 * Structure to represent a list of nodes. They are indices into the
 * "master list" of grid nodes in node.c
 */
typedef struct nodeList_s
{
    /* number of nodes in the list */
    int count;

    /* current size of list allocation */
    int alloced;

    /* array of node indices */
    int *nodes;

} nodeList_t;

/*==============================================================================
 *
 * Node list manipulation functions
 *
 *=============================================================================*/
nodeList_t *readNodeList(char *filename);
void destroyNodeList(nodeList_t *nl);
void addNodeToList(nodeList_t *nl, int idx);
void removeNodeFromList(nodeList_t *nl, int idx);
int isNodeOnList(nodeList_t *nl, int idx);
void updateNodeListIndices(nodeList_t *nl, int idx);
void writeNodeList(nodeList_t *nl, char *filename);

/*
 * The main node startup function. Gets info about the local host, and
 * reads the config files from the central node.
 *
 * Returns 1 for success, 0 for failure
 */
int getNodeInfo(int secondaryOK);

/***********************************************************************
*  int getNodeInfoFromLocalFile(const char *dir)
* TODO EG This is here to get the tests to run.
*
*   Main node startup function. Reads main node list. Works out local hostname 
* and path to QCDgrid software.
*
*   Parameters:                                                    [I/O]
*
*     dir  The directory where the .conf files are				      I
*   
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int getNodeInfoFromLocalFile(const char *dir);
/*
 * Should be called at shutdown to free the storage used in this module
 */
void destroyNodeInfo();

/***********************************************************************
*  struct storageElement *getNode(char *nodeName)
*
*   Gets the storage element with the given name.
*    
*   Parameters:                                                    [I/O]
*
*     nodeName   Node's FQDN                                            I
*   
*   Returns: storage element, or NULL on error
***********************************************************************/
struct storageElement *getNode(char *nodeName);

/***********************************************************************
*   char *getNodeInbox(char *hostname)
*
*   Gets the inbox of a node or NULL if none specified.
*    
*   Parameters:                                                    [I/O]
*
*     hostname   Node's FQDN                                          I
*   
*   Returns: site name, or NULL on error
***********************************************************************/
char *getNodeInbox(const char *hostname);

/*
 * Returns 1 if node supports adding new files via a scan.
 */
int nodeSupportsAddScan(char *nodeName);

/*
 * Returns the name of the central node running the control thread and 
 * replica catalogue
 */
char *getMainNodeName();

/*
 * Returns the hostname (fully qualified) of the local machine. Required once
 * in a while because copying to a gsiftp:// URL on the local machine seems 
 * to cause problems sometimes, so we need to make a special case and copy to
 * a file:// URL instead
 */
char *getHostname();

/*
 * Gets the name of a suitable node for the primary copy of a file of the
 * specified size. Returns the highest preference node which has enough space
 * left
 */
char **getSuitableNodeForPrimary(long long size);

/*
 * Gets the name of a suitable node for the mirror copy of a file of the
 * specified size - this time the preference list is not used, the only
 * requirement is that the mirror node is at a different site from the
 * primary
 */
char *getSuitableNodeForMirror(char *file, long long size);

/* 
 * These two functions translate between a node's name and its position in
 * the node table
 */
char *getNodeName(int i);
int nodeIndexFromName(const char *node);

/*
 * Returns the number of nodes on the grid. Used in conjunction with
 * get_node_name() above, it is used by the control thread to iterate over 
 * all the nodes, checking their disk space remaining and whether they are 
 * still responding
 */
int getNumNodes();

/*
 * Sets the disk space remaining field of an entry in the node table. Should
 * only be called by the control thread
 */
void setNodeDiskSpace(int node, long long df);

/*
 * Returns the disk space free, in kilobytes, on the node specified
 */
long long getNodeDiskSpace(char *node);

/*
 * Updates the disk space values cached in the local program from the main
 * node
 */
void updateNodeDiskSpace();

/*
 * Removes a node from the table. Should be called only by the control thread
 */
void removeNode(int node);

/*
 * Adds a new node to the table. Should be called only by the control thread
 */
void addNode(char *name, char *site, long long df, char *path);

/*
 * Writes the node table back to disk. Should only be called by the control
 * thread
 */
void writeNodeTable();

/*
 * Gets the path at which the QCDgrid software is installed on the local
 * machine
 */
char *getQcdgridPath();

/*
 * Gets the path at which the QCDgrid software is installed on a remote
 * machine
 */
char *getNodePath(const char *node);

/*
 * Gets the site at which a node is physically located
 */
char *getNodeSite(char *node);

/*
 * Gets the extra RSL needed to submit a job to a node
 */
char *getNodeExtraRsl(char *node);

/*
 * Gets the extra JSS contact string needed to submit a job to a node
 */
char *getNodeExtraJssContact(char *node);

/*
 * Retrieves timeout values for the various Globus operations on a node
 */
float getNodeCopyTimeout(char *node);
float getNodeFtpTimeout(char *node);
float getNodeJobTimeout(char *node);

/*
 * Returns 1 if the node is a GPFS system
 */
int getNodeGpfs(char *node);

/*
 *   Returns the total disk space quota for all of the disks on this node.
 */
long long getNodeTotalDiskQuota(char *node);

/*
 * Checks if the specified node is on the dead list. Returns 1 if it is,
 * else 0
 */
int isNodeDead(char *node);

/*
 * Checks if the specified node is on the disabled list. Returns 1 if it is,
 * else 0
 */
int isNodeDisabled(char *node);

/*
 * Checks if the specified node is on the retiring list. Returns 1 if it is,
 * else 0
 */
int isNodeRetiring(char *node);

/*
 * Adds a node to the dead list. Should only be called by the control thread
 */
void addToDeadList(char *node);

/*
 * Adds a node to the disabled list. Should only be called by the control 
 * thread
 */
void addToDisabledList(char *node);

/*
 * Adds a node to the retiring list. Should only be called by the central
 * thread
 */
void addToRetiringList(char *node);

/*
 * Removes a node from the dead list. Should only be called by the control
 * thread
 */
void removeFromDeadList(char *node);

/*
 * Removes a node from the disabled list. Should only be called by the 
 * control thread
 */
void removeFromDisabledList(char *node);

/*
 * Removes a node from the retiring list. Should only be called by the
 * control thread
 */
void removeFromRetiringList(char *node);

/*
 * Writes the dead node list back to disk. Should only be called by the
 * control thread
 */
int writeDeadList();

/*
 * Writes the disabled node list back to disk. Should only be called by the
 * control thread
 */
int writeDisabledList();

/*
 * Writes the retiring node list back to disk. Should only be called by the
 * control thread
 */
int writeRetiringList();

/***********************************************************************
*   char *getTemporaryFile()
*
*   Creates a uniquely named file in the temporary directory and returns
*   its name. File should be deleted and name should be freed by the
*   caller after use.
*
*   Parameters:                                                    [I/O]
*
*     (none)
*   
*   Returns: filename pointer, or NULL on failure
***********************************************************************/

/*
 * Can't call before tmpDir_ is initialised!
 */
char *getTemporaryFile();

void initSEtoGlobus(struct storageElement *se);
void initSEtoSRM(struct storageElement *se);

char *getNodeProperty(const char *node, char *prop);

#endif
