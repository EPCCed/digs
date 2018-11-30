/***********************************************************************
 *
 *   Filename:   gridftp.h
 *
 *   Authors:    James Perry, Eilidh Grant  (jamesp, egrant1)   EPCC.
 *
 *   Purpose:    Provides a simple interface to Globus FTP functions to
 *               the other modules
 *
 *   Contents:   Function prototypes for this module
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
/*
 * The GridFTP module deals with copying files from one grid machine to 
 * another, and also other file operations such as getting file lengths,
 * creating directory structures on storage elements and deleting superfluous
 * replicas. It uses the Globus GASS copy and FTP client modules to achieve
 * this.
 */

#ifndef GRIDFTP_H_
#define GRIDFTP_H_
#include "misc.h"

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
digs_error_code_t digs_getLength_globus(char *errorMessage, const char *filePath,
		const char *hostname, long long int *fileLength);

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
		digs_checksum_type_t checksumType) ;

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
		const char *filePath, const char *hostname, int *isDirectory) ;

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
		const char *filePath, const char *hostname, int *doesExist);

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
digs_error_code_t digs_ping_globus(char *errorMessage, const char * hostname);


/***********************************************************************
 *   void digs_free_string_globus(char **string) 
 *
 * 	 Free string
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 string 		the string to be freed								I
 ***********************************************************************/
void digs_free_string_globus(char **string) ;
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
		const char *filePath, const char *hostname, char **ownerName);

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
		const char *filePath, const char *hostname, char **groupName);

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
		const char *filePath, const char * hostname, const char *groupName);

/***********************************************************************
 *  digs_error_code_t digs_getPermissions_globus(char *errorMessage,
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
digs_error_code_t digs_getPermissions_globus(char *errorMessage,
		const char *filePath, const char *hostname,  char **permissions);

/***********************************************************************
 * digs_error_code_t digs_setPermissions_globus(char *errorMessage, 
 * const char *filePath, const char *hostname, const char *permissions);
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
digs_error_code_t digs_setPermissions_globus(char *errorMessage,
		const char *filePath, const char *hostname, const char *permissions);

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
		time_t *modificationTime);

///***********************************************************************
//* partially implemented get_list.
// ***********************************************************************/
//void digs_getList_globus(struct digs_result_t *result,
//		const char *filePath, const char *hostname, time_t *fileTime,
//		digs_fileTimeType_t *fileTimeType);

/***********************************************************************
 *digs_error_code_t digs_startPutTransfer_globus(char *errorMessage,
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
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 SURL 			the remote location to put the file to				I	
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startPutTransfer_globus(char *errorMessage,
		const char *localPath, const char *hostname, const char *SURL, int *handle);

/***********************************************************************
 *digs_error_code_t digs_startCopyToInbox_globus(char *errorMessage,
 *		const char *hostname, const char *localPath, , const char *lfn,
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
		int *handle);

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
		digs_transfer_status_t *status, int *percentComplete);

/***********************************************************************
 * digs_error_code_t digs_endTransfer_globus(char *errorMessage, int handle)
 * 
 * Cleans up a transfer identified by a handle. Checks that the file 
 * has been transferred correctly (using checksum) and removes the 
 * LOCKED postfix if it has. If the transfer has not completed 
 * correctly, removes partially transferred files.
 *
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 		the id of the transfer								I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_endTransfer_globus(char *errorMessage, int handle);

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
digs_error_code_t digs_cancelTransfer_globus(char *errorMessage, int handle);

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
		const char *filePath);

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
		const char *hostname, const char *filePath );

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
		int *handle);

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
  const char *filePathFrom, const char *filePathTo);

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
		const char *filePath);

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
  const char *dirPath);

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
		const char *filePath);

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
 * 	 targetPath		the full path to the SE location to put the 
 * 					file to.											I		
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_copyFromInbox_globus(char *errorMessage,
		const char *hostname, const char *lfn, const char *targetPath);

/***********************************************************************
 * digs_error_code_t digs_scanNode_globus(char *errorMessage, 
 * const char *hostname, char ***list, int *listLength, int allFiles);
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
		const char *hostname, char ***list, int *listLength, int allFiles);

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
		const char *hostname, char ***list, int *listLength, int allFiles);

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
void digs_free_string_array_globus(char ***arrayOfStrings, int *listLength);

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
digs_error_code_t digs_housekeeping_globus(char *errorMessage, char *hostname);


#endif /*GRIDFTP_H_*/
