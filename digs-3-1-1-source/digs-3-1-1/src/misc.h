/***********************************************************************
*
*   Filename:   misc.h
*
*   Authors:    James Perry, Eilidh Grant      (jamesp, eilidh)   EPCC.
*
*   Purpose:    Low/middle level functions shared by other modules
*
*   Contents:   Function prototypes for this module
*             
*   Used in:    Called by various other modules
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003-2008 The University of Edinburgh
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
 * Miscellaneous functions are in this module, mostly related to file
 * manipulations
 */

#ifndef MISC_H
#define MISC_H
#define MAX_ERROR_MESSAGE_LENGTH 1000
#define CHECKSUM_LENGTH 32
#define MAX_PERMISSIONS_LENGTH 32

/* Need time_t definition */
#include <sys/stat.h>

#include <globus_common.h>

#include <globus_ftp_client.h>

/* Digs error codes.*/
typedef enum
{
  DIGS_SUCCESS,            /* Operation succeeded */
  DIGS_NO_SERVICE,         /* No instance of service found */
  DIGS_NO_RESPONSE,        /* Request to service timed out */

  DIGS_NO_SERVICE_PROXY,   /* No service proxy or expired */
  DIGS_NO_USER_PROXY,      /* No user proxy or expired */
  DIGS_AUTH_FAILED,        /* Authorisation denied */
  DIGS_NO_CONNECTION,      /* Unable to connect */

  DIGS_UNSPECIFIED_SERVER_ERROR, /* An unspecified error from the server. */
  DIGS_FILE_NOT_FOUND,     /* File not found */
  DIGS_INVALID_CHECKSUM,   /* Invalid checksum */
  DIGS_UNSUPPORTED_CHECKSUM_TYPE,   /* This type of checksum is not supported.*/
  DIGS_FILE_IS_DIR, 		/* Expected file but found directory. */

  DIGS_NO_INBOX,		/*This storage element does not have an inbox. */
  DIGS_UNKNOWN_ERROR        /* Operation failed for an unknown reason */
  
} digs_error_code_t;

/* The results structure. */
struct digs_result_t {
	
	/* The error code */
	digs_error_code_t error_code;

	/* The error description */
	char error_description[MAX_ERROR_MESSAGE_LENGTH];
	
};

/* The 5 levels of logging from DEBUG to FATAL. */
typedef enum
{
	DEBUG = 1,
	INFO = 2,
	WARN = 3,
	ERROR = 4,
	FATAL = 5
} log_level;

/* The types of checksum. */
typedef enum
{
  DIGS_MD5_CHECKSUM,         /* Message-Digest algorithm 5 */
  DIGS_CRC_CHECKSUM         /* Cyclic Redundancy Check */
} digs_checksum_type_t;

/* The possible statuses of transfers */
typedef enum
{
  DIGS_TRANSFER_IN_PREPARATION,	/* Transfer is being prepared */ 
  DIGS_TRANSFER_PREPARATION_COMPLETE,	/* Transfer has been prepared */ 
  DIGS_TRANSFER_IN_PROGRESS,	/* Transfer is progressing */
  DIGS_TRANSFER_DONE,		/* Transfer completed successfully */
  DIGS_TRANSFER_FAILED,		/* Transfer failed */
  DIGS_TRANSFER_CLEANUP		/* Post-transfer cleanup */
} digs_transfer_status_t;

int safe_asprintf(char **ptr, const char *templ, ...);
int safe_getline (char **lineptr, int *n, FILE *stream);
char *safe_strdup(const char *str);

int activateGlobusModules();
void deactivateGlobusModules();

void errorExit(char *msg);

/* group map file management */
int userInGroup(char *dn, char *group);
int getUserGroups(char *dn, int *numGroups, char ***groups);
void dumpGroupMapFile();
int loadGroupMapFile(char *filename);


/***********************************************************************
 *   void logMessage(log_level level, char *msg, ...)
 *
 *   Logs a message to stderr
 *    
 *   Parameters:                                [I/O]
 *
 *     level severity level (see above)          I
 *     msg   message format string               I
 *     ...   parameters for message              I
 *    
 *   Returns: (void)
 ***********************************************************************/ 
void logMessage(log_level level, char *msg, ...);

/***********************************************************************
 *   void removeCrlf(char *str)
 *
 *   Utility function to remove CRLF characters from the end of a string. 
 *   Don't call if there could be CRLFs in the middle as then the end of 
 *   the string will be chopped off
 *
 *   Parameters:                                                    [I/O]
 *
 *     str  Pointer to string to remove CRLF from                    I/O
 *   
 *   Returns: (void)
 ***********************************************************************/
void removeCrlf(char *str);

/***********************************************************************
 *   char *digsErrorToString(digs_error_code_t digsErrorCode)
 *
 *   Return the error message for the given digs error code.
 *    
 *   Parameters:                                                 [I/O]
 *
 *     digsErrorCode   The error code to get the message of.      I
 *    
 *   Returns: A string representation of the digs error code.
 ***********************************************************************/
char *digsErrorToString(digs_error_code_t digsErrorCode);

/*
 * Returns the length of a local file in bytes, given either a full or a
 * relative path to it. Returns -1 on error
 */
long long getFileLength(const char *filename);

/*
 * The main initialisation function. Should be called first by everything.
 */
int qcdgridInit(int secondaryOK);

/*
 * The main shutdown function. Should be called last by everything.
 */
void qcdgridShutdown();

/*
 * Sends a text message to the main grid node
 */
int sendMessageToMainNode(char *msg);

/*
 * Sends an e-mail message
 */
void sendEmail(char *address, char *subject, char *message);

/***********************************************************************
*   void convertToLowerCase(char string[])
*    
*   Converts all of the upper case letters in a character array into 
* 	lower case letters.
*    
*   Parameters:                                       		[I/O]
*
*   string  A char array    								I/O
* 
 ***********************************************************************/
void convertToLowerCase(char string[]);

/*
 * Generates an MD5 checksum of a file
 */
void computeMd5Checksum(char *filename, unsigned char checksum[16]);

/***********************************************************************
*  void getChecksum(const char *filePath, char **returnChecksum)
*    
*  Get the checksum of the local file.
*    
*   Parameters:                                        [I/O]
*
*   filePath Full 	name of file to get checksum for    	I
*   returnChecksum 	the checksum						 	O
 ***********************************************************************/
void getChecksum(const char *filePath, char **returnChecksum);


/*
 * Checks if a checksum of a copy of a file matches entry in RLS
 */
int verifyMD5SumWithRLS(char *node, char *lfn, char *pfn);

/*
 * Checks if a group and permissions of a copy of a file matche entry in RLS
 * if not, they get changed on the file
 */
int verifyGroupAndPermissionsWithRLS(char *node, char *lfn, char *pfn);

/*
 * Error display routines
 */
void printErrorObject(globus_object_t *errorObject, char *whichCall);
void printError(globus_result_t errorCode, char *whichCall);

/*
 * Returns a text representation of the time, for log messages
 */
char *textTime();

/*
 * Ensures that all the directories in a path exist on the local machine
 */
int makeLocalPathValid(char *path);

/*
 * Chooses a disk to store data in multidisk systems
 */
char *chooseDataDisk(char *host);

/*
 * Checks whether user or host certificate is going to expire soon and
 * prints warning messages if so
 */
void checkCertificateExpiry();

/*
 * Obtains the GSI identity from the security context. Normaly it
 * would be an identity field from the proxy certificate used.
 */
int getUserIdentity(char ** identity);

/* 
 * Reads the /etc/grid-security/grid-mapfile for a group a 
 * particular user, represented by his DN, belongs to.
 */
int getUserGroup(char *DN, char **group);

/*
 *  Replaces some characters with the others in a given string
 */
char *substituteChars(const char *src, const char *from, const char *to);

/*
 *  Changes permissions on remote file(s)
 */
int chmodRemotely(char *permissions, char *pfn, char *node);
int chownRemotely(char *owner, char *group, char *pfn, char *node);
int getGroupID(char *name);

int canAddFile(char *user);

int isValidChecksum(char *chksum);

int forEachFileInDirectory(char *ldn, int (*callback)(char *lfn, void *param),
			   void *cbparam);


int copyToLocal(char *remoteHost, char *remoteFile, char *localFile);

int copyFromLocal(char *localFile, char *remoteHost, char *remoteFile);


#endif
