/***********************************************************************
*
*   Filename:   misc.c
*
*   Authors:    James Perry, Radoslaw Ostrowski, Eilidh Grant
*               (jamesp, radek, eilidh)  EPCC.
*
*   Purpose:    Low/middle level functions shared by other modules
*
*   Contents:   Initialisation and shutdown, miscellaneous local file
*               manipulations, communication with central control thread
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

#define __USE_POSIX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Need this to support files larger than 2GB */
#define __USE_LARGEFILE64

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include <globus_ftp_client.h>
#include <globus_gram_client.h>

#include <globus_gass_server_ez.h>

#include <globus_rls_client.h>

#include <globus_gass_copy.h>
#include <globus_io.h>

#include <globus_error.h>
#include <globus_error_hierarchy.h>
#include <globus_error_string.h>
#include <globus_io_error_hierarchy.h>
#include <globus_object.h>

#include "misc.h"
#include "node.h"
#include "job.h"
#include "replica.h"
#include "config.h"

#include "md5.h"

#ifdef WITH_VOMS
#include "voms_apic.h"
#endif

/* sometimes we seem to be missing this */
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

/* 28 days */
#define CERTIFICATE_EXPIRY_THRESHOLD 2419200.0

#define SECONDS_PER_DAY 86400.0

/*
 * The port number used for QCDgrid communications
 */
int qcdgridPort_;

/*
 * I hate to have to do this, but we need our own versions of the GNU
 * functions used elsewhere if they're not defined
 */
#ifndef __GNU_LIBRARY__

#include <stdarg.h>

int asprintf (char **ptr, const char *template, ...)
{
    va_list ap;
    char *buffer;
    int size;
    int sizeneeded;

    va_start(ap, template);

    size = 1024;
    buffer = globus_libc_malloc(size);
    if (!buffer) return -1;

    sizeneeded = vsnprintf(buffer, size, template, ap);
    while (sizeneeded>size)
    {
	globus_libc_free(buffer);
	size = sizeneeded + 1024;
	buffer = globus_libc_malloc(size);
	if (!buffer) return -1;
	
	sizeneeded = vsnprintf(buffer, size, template, ap);
    }


    *ptr = buffer;
    va_end(ap);
    return 0;
}

ssize_t getline (char **lineptr, size_t *n, FILE *stream)
{
    /*
     * This isn't as robust as the GNU version (it will only work for lines
     * up to 1024 characters long) but should be good enough for most
     * cases
     */
    if ((*n==0)||(*lineptr==NULL))
    {
	*n=1024;
	*lineptr=globus_libc_malloc(1024);
    }
    if (!fgets(*lineptr, *n, stream))
    {
	return -1;
    }
    else
    {
	return 0;
    }
}

#endif

/*==============================================================================
 *
 * Thread safe versions of some libc functions
 *
 *============================================================================*/
int safe_asprintf(char **ptr, const char *template, ...)
{
    va_list ap;
    char *buffer;
    int size;
    int sizeneeded;

    va_start(ap, template);

    size = 1024;
    buffer = globus_libc_malloc(size);
    if (!buffer) return -1;

    sizeneeded = vsnprintf(buffer, size, template, ap);
    while (sizeneeded > size)
    {
	globus_libc_free(buffer);
	size = sizeneeded + 1024;
	buffer = globus_libc_malloc(size);
	if (!buffer) return -1;
	
	sizeneeded = vsnprintf(buffer, size, template, ap);
    }


    *ptr = buffer;
    va_end(ap);
    return 0;
}

int safe_getline (char **lineptr, int *n, FILE *stream)
{
    int res;
    size_t sz;
    globus_libc_lock();
    sz = *n;
    res = getline(lineptr, &sz, stream);
    *n = (int) sz;
    globus_libc_unlock();
    return res;
}

char *safe_strdup(const char *str)
{
    int len;
    char *result;
    len = strlen(str);
    result = globus_libc_malloc(len + 1);
    if (!result)
    {
	return NULL;
    }
    strcpy(result, str);
    return result;
}

/*==============================================================================
 *
 * Group map file functions
 *
 *============================================================================*/
/* number of entries in the group map */
static int groupMapSize_ = 0;

/* group map entry structure for one user */
typedef struct {
  char *dn;
  int numGroups;
  char **groups;
} groupMap_t;

static groupMap_t *groupMap_ = NULL;

/***********************************************************************
*   void trimString(char *str)
*
*   Trims off any spaces (or tabs or newlines) from the start and end
*   of a string.
*    
*   Parameters:                                               [I/O]
*
*     str   string to trim (cannot be constant)                I O
*    
*   Returns: (void)
***********************************************************************/
void trimString(char *str)
{
  int i, realStart;

  /* count spaces on beginning */
  realStart = 0;
  while (isspace(str[realStart])) realStart++;
  
  /* strip spaces from beginning */
  i = 0;
  do {
    str[i] = str[i + realStart];
    i++;
  } while (str[i + realStart]);
  str[i] = str[i + realStart];

  /* strip spaces from end */
  i = strlen(str) - 1;
  while ((i >= 0) && (isspace(str[i]))) {
    str[i] = 0;
    i--;
  }
}

/***********************************************************************
*   int userInGroup(char *dn, char *group)
*
*   Checks whether or not a user is a member of a particular group. 
*    
*   Parameters:                                               [I/O]
*
*     dn         the user's certificate subject                I
*     group      the group to check for membership of          I
*    
*   Returns: 1 if user is member of group, 0 if not
***********************************************************************/
int userInGroup(char *dn, char *group)
{
  int i;
  int j;
  for (i = 0; i < groupMapSize_; i++) {
    if (!strcmp(dn, groupMap_[i].dn)) {
      break;
    }
  }

  if (i >= groupMapSize_) return 0;
  for (j = 0; j < groupMap_[i].numGroups; j++) {
    if (!strcmp(group, groupMap_[i].groups[j])) {
      return 1;
    }
  }
  return 0;
}

/***********************************************************************
*   int getUserGroups(char *dn, int *numGroups, char ***groups)
*
*   Gets a list of the groups which a user is a member of.
*    
*   Parameters:                                               [I/O]
*
*     dn         the user's certificate subject                I
*     numGroups  receives the number of groups in the list       O
*     groups     receives a pointer to the list (caller should   O
*                NOT free the list or any of the items)
*    
*   Returns: 1 on success, 0 if user not found
***********************************************************************/
int getUserGroups(char *dn, int *numGroups, char ***groups)
{
  int i;
  for (i = 0; i < groupMapSize_; i++) {
    if (!strcmp(dn, groupMap_[i].dn)) {
      break;
    }
  }
  if (i >= groupMapSize_) return 0;

  *numGroups = groupMap_[i].numGroups;
  *groups = groupMap_[i].groups;
  return 1;
}

/***********************************************************************
*   void dumpGroupMapFile()
*
*   Debugging function to dump to contents of the group map to stdout.
*    
*   Parameters:                                               [I/O]
*
*     (none)
*    
*   Returns: (void)
***********************************************************************/
void dumpGroupMapFile()
{
  int i, j;
  globus_libc_printf("Group map size: %d\n", groupMapSize_);
  for (i = 0; i < groupMapSize_; i++) {
    globus_libc_printf("DN: %s\n", groupMap_[i].dn);
    globus_libc_printf("Groups: ");
    for (j = 0; j < groupMap_[i].numGroups; j++) {
      globus_libc_printf("%s ", groupMap_[i].groups[j]);
    }
    globus_libc_printf("\n");
  }
}

/***********************************************************************
*   int loadGroupMapFile(char *filename)
* 
*   Loads the group map from a disk file. The format is simple: each
*   line represents one user and consists of the user's DN (which must
*   be in quotes if it contains spaces) followed by a single space,
*   followed by a comma separated list of groups of which the user is
*   a member.
*    
*   Parameters:                                               [I/O]
*
*     filename   path to the group map file on disk            I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int loadGroupMapFile(char *filename)
{
  FILE *f;
  char *linebuf = NULL;
  size_t bufsize = 0;
  int enddn, startgroups, grouplen;
  char *tmp;
  groupMap_t *g;

  f = fopen(filename, "r");
  if (!f) {
    logMessage(ERROR, "Unable to open group map file %s", filename);
    return 0;
  }

  do {
    if (safe_getline(&linebuf, &bufsize, f) < 0) break;

    groupMapSize_++;
    groupMap_ = globus_libc_realloc(groupMap_, groupMapSize_*sizeof(groupMap_t));
    if (!groupMap_) {
      errorExit("Out of memory loading group map file");
    }
    g = &groupMap_[groupMapSize_ - 1];
    g->dn = NULL;
    g->numGroups = 0;
    g->groups = NULL;

    /* parse this line */
    removeCrlf(linebuf);
    trimString(linebuf);
    if (linebuf[0] == '"') {
      /* DN in quotes */
      /* find end quote */
      tmp = strchr(linebuf+1, '"');
      if (!tmp) {
	logMessage(ERROR, "Invalid line %s in group map file", linebuf);
	continue;
      }
      enddn = tmp - linebuf;
      g->dn = globus_libc_malloc(enddn);
      if (!g->dn) {
	errorExit("Out of memory loading group map file");
      }
      strncpy(g->dn, linebuf+1, enddn-1);
      g->dn[enddn - 1] = 0;

      startgroups = enddn+2;
    }
    else {
      /* DN not in quotes */
      /* find space or tab at end */
      tmp = strchr(linebuf, ' ');
      if (!tmp) {
	tmp = strchr(linebuf, '\t');
	if (!tmp) {
	  logMessage(ERROR, "Invalid line %s in group map file", linebuf);
	  continue;
	}
      }
      enddn = tmp - linebuf;
      g->dn = globus_libc_malloc(enddn);
      if (!g->dn) {
	errorExit("Out of memory loading group map file");
      }
      strncpy(g->dn, linebuf, enddn);
      g->dn[enddn] = 0;
      trimString(g->dn);

      startgroups = enddn+1;
    }

    /* parse the groups */
    do {
      /* find next separator comma */
      tmp = strchr(&linebuf[startgroups], ',');
      if (tmp) {
	grouplen = (tmp - linebuf) - startgroups;
      }
      else {
	grouplen = strlen(linebuf) - startgroups;
      }

      g->numGroups++;
      g->groups = globus_libc_realloc(g->groups, g->numGroups * sizeof(char*));
      if (!g->groups) {
	errorExit("Out of memory loading group map file");
      }
      g->groups[g->numGroups - 1] = globus_libc_malloc(grouplen + 1);
      if (!g->groups[g->numGroups - 1]) {
	errorExit("Out of memory loading group map file");
      }
      strncpy(g->groups[g->numGroups - 1], &linebuf[startgroups], grouplen);
      g->groups[g->numGroups - 1][grouplen] = 0;

      trimString(g->groups[g->numGroups - 1]);

      startgroups = startgroups + grouplen + 1;
    } while (tmp);

  } while (!feof(f));
  fclose(f);
  globus_libc_free(linebuf);

  return 1;
}

/*==============================================================================
 *
 * Error/warning/logging functions
 *
 *============================================================================*/
/***********************************************************************
*   char *textTime()
* 
*   Returns a text representation of the current date and time, which
*   can be used for log messages etc.
*    
*   Parameters:                                               [I/O]
*
*     (none)
*    
*   Returns: pointer to text representation. This is statically
*            allocated and should not be altered or freed
***********************************************************************/
char *textTime()
{
    time_t t;
    char *txt, *txt2;

    /* Get the current time */
    t = time(NULL);
    
    /* Convert it to ASCII */
    txt = ctime(&t);

    /* Remove the annoying newline */
    txt2 = strchr(txt, '\n');
    if (txt2)
    {
	*txt2 = 0;
    }
    return txt;
}

/***********************************************************************
*   void printErrorObject(globus_object_t *errorObject, char *whichCall)
* 
*   Translates a Globus error object into a meaningful error output on
*   the stderr stream   
*    
*   Parameters:                                               [I/O]
*
*     errorObject  The Globus error object                     I
*     whichCall    The name of the function call which failed  I
*    
*   Returns: (void)
***********************************************************************/
void printErrorObject(globus_object_t *errorObject, char *whichCall)
{
    char *errorString;

    if (!errorObject)
    {
	errorString = "null error code returned";
    }
    else
    {
	/* Try and get a specific error string out of Globus */
	errorString = globus_object_printable_to_string((globus_object_t *)errorObject);
	if (!errorString)
	{
	    errorString = "unknown error";
	}
    }

    /*
     * Don't print a "does not exist" error if the call in question was an 
     * existence check
     */
    if ((!strcmp(whichCall, "existence check")) && (strlen(errorString) > 14) &&
	(!strcmp(&errorString[strlen(errorString) - 14], "does not exist")))
    {
	return;
    }

    /* Print message to error stream */
    logMessage(3, "Globus error in %s: %s", whichCall, errorString);
}

/***********************************************************************
*   void printError(globus_result_t errorCode, char *whichCall)
* 
*   Translates a Globus error code into a meaningful error output on
*   the stderr stream   
*    
*   Parameters:                                             [I/O]
*
*     errorCode  The result returned by a Globus function    I
*     whichCall  The name of the function call               I
*    
*   Returns: (void)
***********************************************************************/
void printError(globus_result_t errorCode, char *whichCall)
{
    globus_object_t *errorObject;

    /* Turn the error code into a Globus object */
    errorObject = globus_error_get(errorCode);

    /* and call the function which works on an object */
    printErrorObject(errorObject, whichCall);
}

/***********************************************************************
*   void errorExit(char *msg)
*
*   Exits with an error message. Used for fatal errors such as "out of
*   memory"
*    
*   Parameters:                                [I/O]
*
*     msg   message to print                    I
*    
*   Returns: (void)
***********************************************************************/
void errorExit(char *msg)
{
    globus_libc_fprintf(stderr, "%s\n", msg);
    exit(1);
}

/*
 * Log messages above this level will be printed, below will be
 * suppressed
 */
int logLevel_ = ERROR;

/***********************************************************************
*   void logMessage(log_level level, char *msg, ...)
*
*   Logs a message to stderr
*    
*   Parameters:                                [I/O]
*
*     level severity level, 1 to 5              I
*     msg   message format string               I
*     ...   parameters for message              I
*    
*   Returns: (void)
***********************************************************************/
void logMessage(log_level level, char *msg, ...)
{
    va_list ap;
    char *buffer;
    int size;
    int sizeneeded;

    va_start(ap, msg);

    if (level >= logLevel_)
    {
	size = 1024;
	buffer = globus_libc_malloc(size);
	if (!buffer)
	{
	    errorExit("Out of memory in logMessage");
	}
	
	sizeneeded = vsnprintf(buffer, size, msg, ap);
	while (sizeneeded > size)
	{
	    globus_libc_free(buffer);
	    size = sizeneeded + 1024;
	    buffer = globus_libc_malloc(size);
	    if (!buffer)
	    {
		errorExit("Out of memory in logMessage");
	    }
	    
	    sizeneeded = vsnprintf(buffer, size, msg, ap);
	}

	globus_libc_fprintf(stderr, "[%s] %s\n", textTime(), buffer);
	globus_libc_free(buffer);
    }

    va_end(ap);    
}

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
void removeCrlf(char *str)
{
    char *tmp;

    tmp=strchr(str,13);
    if (tmp) *tmp=0;
    tmp=strchr(str,10);
    if (tmp) *tmp=0;
}

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
char *digsErrorToString(digs_error_code_t digsErrorCode){
	  
	switch(digsErrorCode){
	case DIGS_SUCCESS:
		return "Operation succeeded.";
	case DIGS_NO_SERVICE:
		return "No instance of service found.";
	case DIGS_NO_RESPONSE:
		return "Request to service timed out.";
	case DIGS_NO_SERVICE_PROXY:
		return "No service proxy or expired.";
	case DIGS_NO_USER_PROXY:
		return "No user proxy or expired.";
	case DIGS_AUTH_FAILED:
		return "Authorisation denied.";
	case DIGS_FILE_NOT_FOUND:
		return "File not found, or no permission to read file.";
	case DIGS_INVALID_CHECKSUM:
		return "Invalid checksum.";
	case DIGS_NO_CONNECTION:
		return "Unable to connect. ";
	case DIGS_FILE_IS_DIR:
		return "Expected file but found directory.";
	case DIGS_UNKNOWN_ERROR:
		return "Operation failed for an unknown reason.";
	case DIGS_UNSPECIFIED_SERVER_ERROR:
		return "There was an unspecified error from the server. ";
	case DIGS_NO_INBOX:
		return "This storage element does not have an inbox. ";
	case DIGS_UNSUPPORTED_CHECKSUM_TYPE:
		return "This type of checksum is not supported. ";
	default:
		return "Unrecognised digsErrorCode.";
	}
}

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
void convertToLowerCase(char string[]){
    int i;
    for (i = 0; i < strlen(string); i++)
    {
	char c = string[i];
	if(isalpha(c) && isupper(c))
	{
	    string[i] = tolower(c);
	}
    }
}

/***********************************************************************
*   void sendEmail(char *address, char *subject, char *message)
*
*   Sends an e-mail with the specified subject and message to the
*   address given.    
*    
*   Parameters:                                [I/O]
*
*     address  e-mail address to send to        I
*     subject  subject line for e-mail          I
*     message  body of e-mail                   I
*    
*   Returns: (void)
***********************************************************************/
void sendEmail(char *address, char *subject, char *message)
{
    char *tmpFilename;
    int tmpFd;
    FILE *f;
    char *commandBuffer;

    if (safe_asprintf(&tmpFilename, "%s/qcdgridemailXXXXXX", tmpDir_) < 0)
    {
	errorExit("Out of memory in sendEmail");
    }

    /* create temporary file */
    tmpFd = mkstemp(tmpFilename);
    if (tmpFd < 0)
    {
	logMessage(3, "mkstemp failed in sendEmail");
	globus_libc_free(tmpFilename);
	return;
    }
    close(tmpFd);

    /* write e-mail body into it */
    f=fopen(tmpFilename, "w");
    globus_libc_fprintf(f, message);
    fclose(f);

    /* prepare command line */
    if (safe_asprintf(&commandBuffer, "mail -s \"%s\" %s < %s", subject, address,
		      tmpFilename) < 0)
    {
	unlink(tmpFilename);
	errorExit("Out of memory in sendEmail");
    }

    /* invoke mail command to actually send message */
    system(commandBuffer);
    globus_libc_free(commandBuffer);

    /* clean up temporary file */
    unlink(tmpFilename);
    globus_libc_free(tmpFilename);
}

/*==============================================================================
 *
 * File and directory functions
 *
 *============================================================================*/
/***********************************************************************
*   long long getFileLength(char *filename)
*    
*   Gets the length of a file on the local storage system
*    
*   Parameters:                                [I/O]
*
*     filename  Full pathname of file           I
*    
*   Returns: length of file in bytes, or -1 on error
***********************************************************************/
long long getFileLength(const char *filename)
{
    struct stat64 statbuf;

    if (stat64(filename, &statbuf) < 0)
    {
	logMessage(3, "stat64 failed in getFileLength");
	return -1;
    }

    return (long long) statbuf.st_size;
}

/***********************************************************************
*   int makeLocalPathValid(char *path)
*    
*   Creates directories on the local file system in the path given, if
*   they don't already exist
*    
*   Parameters:                                [I/O]
*
*     path  Path to validate/create             I
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int makeLocalPathValid(char *path)
{
    char *nextPathComponent;
    char *dirBuffer;
    int i;
    DIR *dirCheck;

    logMessage(1, "makeLocalPathValid(%s)", path);

    /* Start from beginning */
    i = 0;
    nextPathComponent = path;
    dirBuffer = globus_libc_malloc(strlen(path));
    if (!dirBuffer) errorExit("Out of memory in makeLocalPathValid");
    nextPathComponent++;

    /* Keep going as long as there are more slashes in the path - when we run 
     * out of slashes, we've checked all the directories */
    while (strchr(nextPathComponent, '/')) 
    {
      
	dirBuffer[i] = '/';
	i++;
    
	/* Copy the next dir name to our buffer */
	while ((*nextPathComponent)!='/') 
	{
	    dirBuffer[i]=*nextPathComponent;
	    i++;
	    nextPathComponent++;
	}
	dirBuffer[i]=0;

	/* Skip the slash */
	nextPathComponent++;

	/* Now check to see if the directory exists */
	dirCheck=opendir(dirBuffer);

      	if (!dirCheck) 
	{
	    /* Need to create it now */
	    if (!mkdir(dirBuffer, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)<0) 
	    {
		logMessage(3, "mkdir failed in makeLocalPathValid");
		globus_libc_free(dirBuffer);
		return 0;
	    }
	}
	else
	{
	    closedir(dirCheck);
	}
    }

    globus_libc_free(dirBuffer);
    return 1;
    
}

/*==============================================================================
 *
 * Startup and shutdown functions
 *
 *============================================================================*/
/***********************************************************************
*   void deactivateGlobusModules()
*    
*   Deactivates all the Globus modules used by QCDgrid. Should never be
*   called directly - is installed as an atexit function when the modules
*   are activated.
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
void deactivateGlobusModules()
{
    logMessage(1, "Deactivating Globus modules");
    globus_module_deactivate(GLOBUS_RLS_CLIENT_MODULE);
    globus_module_deactivate(GLOBUS_GASS_SERVER_EZ_MODULE);
    globus_module_deactivate(GLOBUS_IO_MODULE);
    globus_module_deactivate(GLOBUS_GRAM_CLIENT_MODULE);
    globus_module_deactivate(GLOBUS_GASS_COPY_MODULE);
    globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE);
    globus_module_deactivate(GLOBUS_COMMON_MODULE);
}


/***********************************************************************
*   int activateGlobusModules()
*    
*   Activates all the Globus modules used by QCDgrid
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int activateGlobusModules()
{
    logMessage(1, "Activating Globus modules");
    if (globus_module_activate(GLOBUS_COMMON_MODULE)!=GLOBUS_SUCCESS)
	return 0;
    if (globus_module_activate(GLOBUS_FTP_CLIENT_MODULE)!=GLOBUS_SUCCESS)
	return 0;
    if (globus_module_activate(GLOBUS_GASS_COPY_MODULE)!=GLOBUS_SUCCESS)
	return 0;
    if (globus_module_activate(GLOBUS_GRAM_CLIENT_MODULE)!=GLOBUS_SUCCESS)
	return 0;
    if (globus_module_activate(GLOBUS_IO_MODULE)!=GLOBUS_SUCCESS)
	return 0;
    if (globus_module_activate(GLOBUS_GASS_SERVER_EZ_MODULE)!=GLOBUS_SUCCESS)
	return 0;
    if (globus_module_activate(GLOBUS_RLS_CLIENT_MODULE)!=GLOBUS_SUCCESS)
	return 0;

    return 1;
}

/***********************************************************************
*   int qcdgridInit()
*    
*   Starts up the various parts of the QCDgrid software. Should be called 
*   first by all QCDgrid commands
*    
*   Parameters:                                                   [I/O]
*
*     secondaryOK  Set if it is acceptable to use the backup node  I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int qcdgridInit(int secondaryOK)
{
    char *ll;

    logMessage(1, "qcdgridInit(%d)", secondaryOK);

    ll = getenv("QCDGRID_LOG_LEVEL");
    if (ll)
    {
	logLevel_ = atoi(ll);
	if (logLevel_ < 1) logLevel_ = 1;
	if (logLevel_ > 5) logLevel_ = 5;
    }

    if (!activateGlobusModules()) 
    {
	logMessage(4, "Cannot activate required Globus modules - possibly no valid proxy");
	return 0;
    }

    if (!getNodeInfo(secondaryOK))
    {
	logMessage(4, "Cannot read node config");
	return 0;
    }

    qcdgridPort_ = getConfigIntValue("miscconf", "qcdgrid_port", 51000);

    if (!openReplicaCatalogue(getMainNodeName())) 
    {
	logMessage(4, "Cannot open replica catalogue");
	return 0;
    }

    return 1;
}

/***********************************************************************
*   void qcdgridShutdown()
*    
*   Closes/frees all the resources used by the QCDgrid software
*    
*   Parameters:                                                   [I/O]
*
*     None
*    
*   Returns: (void)
***********************************************************************/
void qcdgridShutdown()
{
    logMessage(1, "qcdgridShutdown");
    closeReplicaCatalogue();
    destroyNodeInfo();
    deactivateGlobusModules();
}

/*==============================================================================
 *
 * Message sending functions
 *
 *============================================================================*/
/***********************************************************************
*   globus_bool_t authCallback2(void *arg, globus_io_handle_t *handle,
*	 		        globus_result_t result, char *identity, 
*	    		        gss_ctx_id_t *context_handle)
*    
*   Authorisation callback required for sending a secure message to
*   the main node. The meaningful authorisation (i.e. 'is this person
*   allowed to delete files?') is done on the server side, this
*   callback is effectively asking 'is this machine allowed to receive
*   our message?'. We just say yes - there's no particular reason to
*   keep the message itself a secret.
*
*   Should never be called directly. Only called by the Globus socket
*   code.
*
*   Parameters:                                [I/O]
*
*     All are unused
*    
*   Returns: GLOBUS_TRUE, to indicate that authorisation should succeed
***********************************************************************/
static globus_bool_t authCallback2(void *arg, globus_io_handle_t *handle,
				   globus_result_t result, char *identity, 
				   gss_ctx_id_t *context_handle)
{
    return GLOBUS_TRUE;
}

/***********************************************************************
*   int sendMessageToMainNode(char *msg)
*    
*   Sends a text message to the control thread running on the main node
*   of the grid. Used for administration of nodes (adding/removing/
*   enabling/disabling etc.)
*    
*   Parameters:                                [I/O]
*
*     None
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int sendMessageToMainNode(char *msg)
{
    char *hostname;
    globus_io_handle_t sock;
    globus_io_attr_t attr;
    globus_size_t n;
    globus_result_t errorCode;
    globus_io_secure_authorization_data_t authData;
    char responseBuffer[33];
    
    logMessage(1, "sendMessageToMainNode(%s)", msg);

    hostname = getMainNodeName();

    if (globus_io_secure_authorization_data_initialize(&authData)
	!= GLOBUS_SUCCESS)
    {
	logMessage(3, "globus_io_secure_authorization_data_initialize failed in "
		   "sendMessageToMainNode");
	return 0;
    }

    if (globus_io_secure_authorization_data_set_callback(&authData,
							 (globus_io_secure_authorization_callback_t)authCallback2, NULL)
	!= GLOBUS_SUCCESS)
    {
	logMessage(3, "Error setting authorization callback in sendMessageToMainNode");
	return 0;
    }

    if (globus_io_tcpattr_init(&attr) != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error initialising TCP attribute structure in sendMessageToMainNode");
	return 0;
    }

    if (globus_io_attr_set_secure_authentication_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHENTICATION_MODE_GSSAPI, 
	    GSS_C_NO_CREDENTIAL) != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error setting authentication mode to GSSAPI in sendMessageToMainNode");
	return 0;
    }                                            

    if (globus_io_attr_set_secure_proxy_mode (&attr, 
					      GLOBUS_IO_SECURE_PROXY_MODE_MANY)
	!= GLOBUS_SUCCESS)
    {
	logMessage(3, "Error setting proxy mode to many in sendMessageToMainNode");
	return 0;
    }

    if (globus_io_attr_set_secure_authorization_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHORIZATION_MODE_CALLBACK, 
	    &authData) != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error setting authorization mode to callback in sendMessageToMainNode");
	return 0;
    }

    if (globus_io_attr_set_secure_channel_mode(
	    &attr, GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP) != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error setting channel mode to GSI wrap in sendMessageToMainNode");
	return 0;
    }

    /* Increase socket buffer sizes - defaults are too small */
    errorCode = globus_io_attr_set_socket_sndbuf(&attr, 1024);
    if (errorCode != GLOBUS_SUCCESS)
    {
	printError(errorCode, "globus_io_attr_set_socket_sndbuf");
	return 0;
    }

    errorCode = globus_io_attr_set_socket_rcvbuf(&attr, 1024);
    if (errorCode != GLOBUS_SUCCESS)
    {
	printError(errorCode, "globus_io_attr_set_socket_rcvbuf");
	return 0;
    }

    errorCode = globus_io_tcp_connect(hostname, qcdgridPort_, &attr, &sock);
    if (errorCode != GLOBUS_SUCCESS)
    {
	printError(errorCode, "globus_io_tcp_connect");
	return 0;
    }

    if (globus_io_write(&sock, (globus_byte_t *)msg, strlen(msg)+1, &n) != GLOBUS_SUCCESS)
    {
	logMessage(3, "Error writing to main node in sendMessageToMainNode");
	return 0;
    }

    errorCode = globus_io_read(&sock, (globus_byte_t *)responseBuffer, 32, 32, &n);

    while ((n == 0) && (errorCode == GLOBUS_SUCCESS))
    {
        errorCode = globus_io_read(&sock, (globus_byte_t *)responseBuffer, 32, 32, &n);
    }

    if (n)
    {
	responseBuffer[n] = 0;
	if (!strncmp(responseBuffer, "OK", 2))
	{
	    logMessage(1, "Command received by main node");
	}
	else
	{
	    logMessage(3, "Main node responded with an error: %s", responseBuffer);
	    globus_io_close(&sock);
	    return 0;
	}
    }
    else
    {
	printError(errorCode, "waiting for server's response");
    }

    globus_io_close(&sock);

    return 1;
}

/*==============================================================================
 *
 * Checksumming functions
 *
 *============================================================================*/
/***********************************************************************
*  void getChecksum(const char *filePath, char **returnChecksum)
*    
* Get the checksum of the local file.
*    
*   Parameters:                                        [I/O]
*
*   filePath Full 	name of file to get checksum for    	I
*   returnChecksum 	the checksum						 	O
 ***********************************************************************/
void getChecksum(const char *filePath, char **returnChecksum) {
	unsigned char checksum[16];
	char hexBuffer[40];
	int i;
	char *fileChecksum=  malloc(CHECKSUM_LENGTH * sizeof(char));

	computeMd5Checksum(filePath, checksum);

	/* Get it in the same form as the remote checksum for comparison */
	for (i = 0; i < 16; i++) {
	  sprintf(&hexBuffer[i*2], "%02X", checksum[i]);
	}
	strcpy(fileChecksum, hexBuffer);

	*returnChecksum = fileChecksum;
}

/***********************************************************************
*   void computeMd5Checksum(char *filename, unsigned char checksum[16])
*
*   Calculates the MD5 checksum of the specified local file    
*    
*   Parameters:                                [I/O]
*
*     filename  Name of file to work on         I
*     checksum  Checksum of the file              O
*    
*   Returns: (void)
***********************************************************************/
void computeMd5Checksum(char *filename, unsigned char checksum[16])
{
    md5_state_t md5State;
    long long fileSize;
    FILE *f;
    char buffer[1024];

    logMessage(1, "computeMd5Checksum(%s)", filename);

    fileSize = getFileLength(filename);

    md5_init(&md5State);

    f = fopen(filename, "rb");

    while(fileSize >= 1024)
    {
	fread(buffer, 1, 1024, f);
	md5_append(&md5State, (unsigned char *)buffer, 1024);
	fileSize -= 1024;
    }

    if (fileSize > 0)
    {
	fread(buffer, 1, fileSize, f);
	md5_append(&md5State, (unsigned char *)buffer, fileSize);
    }

    md5_finish(&md5State, checksum);
    
    fclose(f);
}

/***********************************************************************
*   int isValidChecksum(char *chksum)
*
*   Checks if the checksum string is valid (i.e. is a string of 32 hex
*   digits)
*    
*   Parameters:                                [I/O]
*
*     chksum  The checksum string to validate   I
*    
*   Returns: 1 if valid, 0 if not
***********************************************************************/
int isValidChecksum(char *chksum)
{
    int i;

    /* Remove any newline */
    for (i = 0; i < strlen(chksum); i++)
    {
	if ((chksum[i] == 10) || (chksum[i] == 13))
	{
	    chksum[i] = 0;
	}
    }

    /* Should be 16 bytes - 32 hex digits */
    if (strlen(chksum) != 32)
    {
	return 0;
    }

    for (i = 0; i < 32; i++)
    {
	if ((chksum[i] >= '0') && (chksum[i] <= '9'))
	{
	    continue;
	}
	if ((chksum[i] >= 'a') && (chksum[i] <= 'f'))
	{
	    continue;
	}
	if ((chksum[i] >= 'A') && (chksum[i] <= 'F'))
	{
	    continue;
	}
	return 0;
    }
    return 1;
}

/***********************************************************************
*   int verifyGroupAndPermissionsWithRLS(char *node, char *pfn)
*
*   Verifies (and changes if necessary) that remote copy of a file have
*   correct file permissions and owner and group
*    
*   Parameters:                                            [I/O]
*     node  FQDN of node holding copy of file               I
*     lfn   The logical name of the file                    I
*     pfn   Name of file on node                            I
*    
*   Returns: 1 if operation succedeed, 0 if error occurred
***********************************************************************/
int verifyGroupAndPermissionsWithRLS(char *node, char *lfn, char *pfn)
{
    char *RLSgroup;
    char *RLSpermissions;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    struct storageElement *se;
    digs_error_code_t result;
    char *perms;

    se = getNode(node);
    if (!se)
    {
	logMessage(5, "Error getting SE structure for node %s", node);
	return 0;
    }
    
    if(!getAttrValueFromRLS(lfn, "group", &RLSgroup))
    {
	logMessage(5, "Error obtaining the group from RLS for file %s", lfn);
	return 0;
    }
    
    if(!getAttrValueFromRLS(lfn, "permissions", &RLSpermissions))
    {
	logMessage(5, "Error obtaining the permissions from RLS for file %s", lfn);
	return 0;
    }  

    if (!strcmp(RLSpermissions, "private"))
    {
        perms = "0640";
    }
    else
    {
        perms = "0644";  
    }

    result = se->digs_setPermissions(errbuf, pfn, node, perms);
    if (result != DIGS_SUCCESS)
    {
	logMessage(5, "Error running digs_setPermissions on %s: %s (%s)",
		   node, digsErrorToString(result), errbuf);
	globus_libc_free(RLSgroup);
	globus_libc_free(RLSpermissions);
	return 0;
    }

    result = se->digs_setGroup(errbuf, pfn, node, RLSgroup);
    if (result != DIGS_SUCCESS)
    {
	logMessage(5, "Error running digs_setGroup on %s: %s (%s)",
		   node, digsErrorToString(result), errbuf);
	globus_libc_free(RLSgroup);
	globus_libc_free(RLSpermissions);
	return 0;
    }

    globus_libc_free(RLSgroup);
    globus_libc_free(RLSpermissions);
    
    return 1;

}

/***********************************************************************
*   int verifyMD5SumWithRLS(char *node, char *lfn, char *pfn)
*
*   Verifies that remote copy of a file have correct checksum
*    
*   Parameters:                                            [I/O]
*     node  FQDN of node holding copy of file               I
*     lfn   The logical file name                           I
*     pfn   Name of file on node                            I
*    
*   Returns: 1 if checksum matches, 0 if not, -1 if error occurred
***********************************************************************/
int verifyMD5SumWithRLS(char *node, char *lfn, char *pfn)
{
    char *remoteChecksum;
    char *RLSchecksum;

    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    struct storageElement *se;
    digs_error_code_t result;

    logMessage(1, "verifyMD5SumWithRLS(%s,%s,%s)", node, lfn, pfn);

    se = getNode(node);
    if (!se)
    {
	logMessage(5, "Error getting SE structure for node %s", node);
	return -1;
    }
    result = se->digs_getChecksum(errbuf, pfn, node, &remoteChecksum,
				  DIGS_MD5_CHECKSUM);
    if (result != DIGS_SUCCESS)
    {
	logMessage(5, "Error running digs_getChecksum on %s: %s (%s)",
		   node, digsErrorToString(result), errbuf);
	return -1;
    }

    // now get the md5sum from RLS and compare

    if(!getAttrValueFromRLS(lfn, "md5sum", &RLSchecksum))
    {
      logMessage(5, "Error obtaining the md5sum from RLS for file %s", lfn);
      return -1;
    }  

    if (!isValidChecksum(remoteChecksum))
    {
	logMessage(3, "Invalid remote checksum '%s' returned from (%s,%s)",
		   remoteChecksum, node, pfn);
	return -1;
    }
    if (!isValidChecksum(RLSchecksum))
    {
	logMessage(3, "Invalid RLS checksum '%s' returned from (%s,%s)",
		   RLSchecksum, node, pfn);
	return -1;
    }

    if (!strcasecmp(remoteChecksum, RLSchecksum))
    {
	logMessage(1, "Checksums match");
	se->digs_free_string(&remoteChecksum);
	globus_libc_free(RLSchecksum);
	return 1;
    }
    else
    {
	logMessage(1, "Checksums differ");
	se->digs_free_string(&remoteChecksum);
	globus_libc_free(RLSchecksum);
	return 0;
    }
}


/***********************************************************************
*   int chmodRemotely(char *permissions, char *pfn, char *node)
*
*   Executes digs-chmod on remote host
*    
*   Parameters:                                            [I/O]
*     node          FQDN of node holding the file           I
*     permissions   "private" or "public"                   I
*     pfn           Name of file on node                    I
*    
*   Returns: 1 if successful, 0 if error occurred
***********************************************************************/
int chmodRemotely(char *permissions, char *pfn, char *node)
{
    struct storageElement *se;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;
    char *perms = "0640";

    se = getNode(node);
    if (!se)
    {
	logMessage(ERROR, "Error getting node structure for %s in chmodRemotely",
		   node);
	return 0;
    }

    if (!strcmp(permissions, "public"))
    {
	perms = "0644";
    }

    result = se->digs_setPermissions(errbuf, pfn, node, perms);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error setting permissions of %s on %s: %s (%s)",
		   pfn, node, digsErrorToString(result), errbuf);
	return 0;
    }
    return 1;
}

/***********************************************************************
*   int chownRemotely(char *owner, char *group, char *pfn, char *node)
*
*   Executes digs-chown on remote host
*    
*   Parameters:                                            [I/O]
*     node  FQDN of node holding the file                   I
*     owner Desired owner of the file                       I
*     group Desired group of the file                       I
*     pfn   Name of file on node                            I
*    
*   Returns: 1 if successful, 0 if error occurred
***********************************************************************/
int chownRemotely(char *owner, char *group, char *pfn, char *node)
{
    struct storageElement *se;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];
    digs_error_code_t result;

    se = getNode(node);
    if (!se)
    {
	logMessage(ERROR, "Error getting node struct for %s in chownRemotely",
		   node);
	return 0;
    }

    result = se->digs_setGroup(errbuf, pfn, node, group);
    if (result != DIGS_SUCCESS)
    {
	logMessage(ERROR, "Error setting %s on %s to group %s: %s (%s)",
		   pfn, node, group, digsErrorToString(result), errbuf);
	return 0;
    }
    return 1;
}


/***********************************************************************
*   char *chooseDataDisk(char *host)
* 
*   Chooses which disk of a multidisk storage element to store a file
*   on. Currently just always chooses the one with most space free -
*   can't see any problem with doing it that way
*    
*   Parameters:                                             [I/O]
*
*       host    the hostname of the storage element          I
*    
*   Returns: data directory name with most space on it. Storage should
*            be freed by the caller
***********************************************************************/
char *chooseDataDisk(char *host)
{
    struct storageElement *se;
    long long *used;
    long long *freespace;
    int numDisks;
    int i;
    int bestidx = -1;
    long long bestval = 0;
    char *diskname;

    logMessage(1, "chooseDataDisk(%s)", host);

    /* get node structure which contains quotas for each disk */
    se = getNode(host);
    if (!se)
    {
	logMessage(ERROR, "Error getting node structure for %s in chooseDataDisk",
		   host);
	return NULL;
    }

    /* get file space used on each disk from RLS */
    used = getFilespaceUsedOnNode(host, &numDisks);
    if (!used)
    {
	logMessage(ERROR, "Error getting filespace used for %s in chooseDataDisk",
		   host);

	numDisks = se->numDisks;
	used = globus_libc_malloc(numDisks * sizeof(long long));
	for (i = 0; i < numDisks; i++) {
	  used[i] = 0LL;
	}
    }

    /* calculate free space on each disk */
    freespace = globus_libc_malloc(numDisks * sizeof(long long));
    if (!freespace)
    {
	globus_libc_free(used);
	logMessage(ERROR, "Out of memory in chooseDataDisk");
	return NULL;
    }
    for (i = 0; i < numDisks; i++)
    {
	freespace[i] = se->diskQuota[i] - used[i];
    }
    globus_libc_free(used);

    /* choose the disk with most free space */
    for (i = 0; i < numDisks; i++)
    {
	if (freespace[i] > bestval)
	{
	    bestidx = i;
	    bestval = freespace[i];
	}
    }
    globus_libc_free(freespace);

    /* return the disk name string */
    if (bestidx < 0)
    {
	diskname = NULL;
	logMessage(ERROR, "All disks on %s are full", host);
    }
    else if (bestidx == 0)
    {
	diskname = safe_strdup("data");
    }
    else
    {
	if (safe_asprintf(&diskname, "data%d", bestidx) < 0)
	{
	    logMessage(ERROR, "Out of memory in chooseDataDisk");
	    diskname = NULL;
	}
    }

    return diskname;
}

/***********************************************************************
*   time_t getCertificateExpiryTime(int host)
* 
*   Checks when the user or host certificate will expire
*    
*   Parameters:                                             [I/O]
*
*       host    0=check user cert, 1=check host cert         I
*    
*   Returns: Time at which the certificate will expire, -1 on failure
***********************************************************************/
static time_t getCertificateExpiryTime(int host)
{
    FILE *f;
    char buffer[80];
    char *outputFile;
    int fd;
    char *sysBuffer;
    struct tm expiry;
    time_t result;

    char month[10];
    int day;
    int year;
    char unused[20];
    int i;

    static char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    logMessage(1, "getCertificateExpiryTime(%d)", host);

    if (safe_asprintf(&outputFile, "%s/qcdgridcertinfXXXXXX", tmpDir_) < 0)
    {
	errorExit("Out of memory in getCertificateExpiryTime");
    }

    /*
     * Yet another case where popen would be nice, but it doesn't seem
     * to want to play ball (probably due to Globus threading)
     */
    fd = mkstemp(outputFile);
    if (fd < 0) 
    {
	globus_libc_free(outputFile);
	logMessage(3, "mkstemp failed in getCertificateExpiryTime");
	return (time_t) -1;
    }
    close(fd);

    if (!host)
    {
	if (safe_asprintf(&sysBuffer, "grid-cert-info -enddate > %s", outputFile) < 0)
	{
	    errorExit("Out of memory in getCertificateExpiryTime");
	}
    }
    else
    {
	/*
	 * First check that host cert actually exists - it might not on a client machine
	 */
	f = fopen("/etc/grid-security/hostcert.pem", "r");
	if (!f)
	{
	    logMessage(1, "Didn't find host certificate");
	    unlink(outputFile);
	    globus_libc_free(outputFile);
	    return (time_t) -1;
	}
	fclose(f);

	if (safe_asprintf(&sysBuffer, "grid-cert-info -file /etc/grid-security/hostcert.pem -enddate > %s",
			  outputFile) < 0)
	{
	    errorExit("Out of memory in getCertificateExpiryTime");
	}
    }

    if (system(sysBuffer) != 0)
    {
	/* if grid-cert-info returned non-zero, something went wrong */
	logMessage(3, "grid-cert-info failed in getCertificateExpiryTime");
	globus_libc_free(sysBuffer);
	unlink(outputFile);
	globus_libc_free(outputFile);
	return (time_t) -1;
    }
    globus_libc_free(sysBuffer);

    f=fopen(outputFile, "r");
    if (!f)
    {
	logMessage(3, "Error opening output file in getCertificateExpiryTime");
	unlink(outputFile);
	globus_libc_free(outputFile);
	return (time_t) -1;
    }
    
    /* read the single line containing the certificate expiry date */
    fgets(buffer, 80, f);
    fclose(f);

    unlink(outputFile);
    globus_libc_free(outputFile);

    sscanf(buffer, "%s %d %s %d", month, &day, unused, &year);

    for (i = 0; i < 12; i++)
    {
	if (!strcmp(months[i], month))
	{
	    expiry.tm_mon = i;
	    break;
	}
    }
    if (i == 12)
    {
	logMessage(3, "Month returned from grid-cert-info is invalid");
	return (time_t) -1;
    }

    /* don't bother with time, only date */
    expiry.tm_sec = 0;
    expiry.tm_min = 0;
    expiry.tm_hour = 0;
    expiry.tm_mday = day;
    expiry.tm_year = year - 1900;
    expiry.tm_wday = -1;
    expiry.tm_yday = -1;
    expiry.tm_isdst = -1;

    result = mktime(&expiry);
    return result;
}

/***********************************************************************
*   void checkCertificateExpiry()
* 
*   Checks when the user and host certificates will expire. Prints a
*   warning message if one will expire in the next 28 days.
*    
*   Parameters:                                             [I/O]
*
*       None
*    
*   Returns: (void)
***********************************************************************/
void checkCertificateExpiry()
{
    time_t now;
    time_t userexp, hostexp;
    float left;

    logMessage(1, "checkCertificateExpiry()");

    now = time(NULL);
    userexp = getCertificateExpiryTime(0);
    hostexp = getCertificateExpiryTime(1);

    if (userexp != -1)
    {
	left = difftime(userexp, now);
	if (left < CERTIFICATE_EXPIRY_THRESHOLD)
	{
	    logMessage(5, "WARNING: user certificate will expire in %d days",
		       (int)(left / SECONDS_PER_DAY));
	}
    }
    if (hostexp != -1)
    {
	left = difftime(hostexp, now);
	if (left < CERTIFICATE_EXPIRY_THRESHOLD)
	{
	    logMessage(5, "WARNING: host certificate will expire in %d days",
		       (int)(left / SECONDS_PER_DAY));
	}
    }
}

/***********************************************************************
*  char *substituteChars(const char *src, const char *from, const char *to)
* 
*   Replaces some characters with the others in a given string
*   The returned string needs to be freed!
*   
*
*   Parameters:                                             [I/O]
*
*       src     - pointer to the source string                I
*       from    - the characters to be replaced               I
*       to      - the characters to be used                   I
*
*   Returns: 
*       
*       pointer to string with replaced characters (needs to be freed)
*
*
*  This function was written by Dave Sinkula and found on the 
*  following website: http://www.daniweb.com/code/snippet262.html
*
***********************************************************************/
char *substituteChars(const char *src, const char *from, const char *to)
{

   /*
    * Find out the lengths of the source string, text to replace, and
    * the replacement text.
    */
   size_t size    = strlen(src) + 1;
   size_t fromlen = strlen(from);
   size_t tolen   = strlen(to);
   /*
    * Allocate the first chunk with enough for the original string.
    */
   char *value = globus_libc_malloc(size);
   /*
    * We need to return 'value', so let's make a copy to mess around with.
    */
   char *dst = value;
   /*
    * Before we begin, let's see if malloc was successful.
    */
   if ( value != NULL )
   {
      /*
       * Loop until no matches are found.
       */
      for ( ;; )
      {
         /*
          * Try to find the search text.
          */
         const char *match = strstr(src, from);
         if ( match != NULL )
         {
            /*
             * Found search text at location 'match'. :)
             * Find out how many characters to copy up to the 'match'.
             */
            size_t count = match - src;
            /*
             * We are going to realloc, and for that we will need a
             * temporary pointer for safe usage.
             */
            char *temp;
            /*
             * Calculate the total size the string will be after the
             * replacement is performed.
             */
            size += tolen - fromlen;
            /*
             * Attempt to realloc memory for the new size.
             */
            temp = globus_libc_realloc(value, size);
            if ( temp == NULL )
            {
               /*
                * Attempt to realloc failed. Free the previously malloc'd
                * memory and return with our tail between our legs. :(
                */
               globus_libc_free(value);
               return NULL;
            }
            /*
             * The call to realloc was successful. :) But we'll want to
             * return 'value' eventually, so let's point it to the memory
             * that we are now working with. And let's not forget to point
             * to the right location in the destination as well.
             */
            dst = temp + (dst - value);
            value = temp;
            /*
             * Copy from the source to the point where we matched. Then
             * move the source pointer ahead by the amount we copied. And
             * move the destination pointer ahead by the same amount.
             */
            globus_libc_memmove(dst, src, count);
            src += count;
            dst += count;
            /*
             * Now copy in the replacement text 'to' at the position of
             * the match. Adjust the source pointer by the text we replaced.
             * Adjust the destination pointer by the amount of replacement
             * text.
             */
            globus_libc_memmove(dst, to, tolen);
            src += fromlen;
            dst += tolen;
         }
         else /* No match found. */
         {
            /*
             * Copy any remaining part of the string. This includes the null
             * termination character.
             */
            strcpy(dst, src);
            break;
         }
      }
   }
   return value;
}

/***********************************************************************
*   int getUserIdentity()
* 
*   Obtains the GSI identity from the security context. Normaly it
*   would be an identity field from the proxy certificate used.
*    
*   Parameters:                                             [I/O]
*
*       identity - pointer to an identity variable             O
*    
*   Returns: 1 on success and 0 on error
***********************************************************************/
int getUserIdentity(char ** identity)
{
    OM_uint32       major_status = 0;
    OM_uint32       minor_status = 0;
    gss_cred_id_t   credential = GSS_C_NO_CREDENTIAL;
    gss_name_t	    client_name = GSS_C_NO_NAME;
    gss_buffer_desc name_buffer;       

    /* acquire the users credentials */
    major_status = gss_acquire_cred(&minor_status,   /* (out) minor status */
				    GSS_C_NO_NAME,   /* (in) desired name */
				    GSS_C_INDEFINITE,/* (in) desired time
							     valid */
				    GSS_C_NO_OID_SET,/* (in) desired mechs */
				    GSS_C_BOTH,      /* (in) cred usage
							     GSS_C_BOTH
							     GSS_C_INITIATE
							     GSS_C_ACCEPT */
				    &credential,     /* (out) cred handle */
				    NULL,	     /* (out) actual mechs */
				    NULL);	     /* (out) actual time
							      valid */

    if(major_status != GSS_S_COMPLETE)
    {
	logMessage(1, "Failed to acquire credentials");
	return 0;
    }

    /* extract the name associated with the creds */
    major_status = gss_inquire_cred(&minor_status, /* (out) minor status */
				    credential,    /* (in) cred handle */
				    &client_name,  /* (out) name in cred */
				    NULL,	   /* (out) lifetime */
				    NULL,	   /* (out) cred usage
							    GSS_C_BOTH
							    GSS_C_INITIATE
							    GSS_C_ACCEPT */
				    NULL);	   /* (out) mechanisms */

    /* authentication succeeded, figure out who it is */
    if (major_status == GSS_S_COMPLETE)
    {
	major_status = gss_display_name(&minor_status,
					client_name,
					&name_buffer,
					NULL);
	*identity = safe_strdup(name_buffer.value);
    }
    else
    {
	logMessage(1, "Failed to determine user DN");
	return 0;
    }

    return 1;
}

/***********************************************************************
*   char *getGroupFromVOMSProxy()
* 
*   Tries to retrieve the user's group from their VOMS proxy
*    
*   Parameters:                                             [I/O]
*
*     (none)
*    
*   Returns: group string (caller should free) on success, NULL on
*            failure
***********************************************************************/
char *getGroupFromVOMSProxy()
{
  char *group = NULL;

#ifdef WITH_VOMS
  struct vomsdata *vd;
  int err, res;

  vd = VOMS_Init("", "");
  if (vd == NULL) {
    return NULL;
  }

  res = VOMS_RetrieveFromProxy(RECURSE_NONE, vd, &err);
  if (res == 1) {
    group = safe_strdup(vd->data[0]->std[0]->group);
  }

  VOMS_Destroy(vd);
#endif

  return group;
}

/***********************************************************************
*   int getUserGroup(char * DN, char **group)
* 
*   Reads the /etc/grid-security/grid-mapfile for a group a 
*   particular user, represented by his DN, belongs to.
*   Returned group must be freed!
*    
*   Parameters:                                             [I/O]
*
*        DN      the identity of the user                     I
*        group   a group of particular use (eg. ukq)          O  
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
int getUserGroup(char *DN, char **group)
{

  //insert quotes around DN
  char * cite = "\"";
  char * DNcited;

  char **groups;
  int numGroups;
 
  // printf("DN: %s\n", DN);   
  int DNlength = strlen(DN);

  char *vomsgroup;
  char *envgroup = getenv("DIGS_GROUP");

  if (envgroup)
  {
      *group = safe_strdup(envgroup);
      return 1;
  }

  vomsgroup = getGroupFromVOMSProxy();
  if (vomsgroup)
  {
    *group = vomsgroup;
    return 1;
  }

  if (!getUserGroups(DN, &numGroups, &groups)) {
    *group = NULL;
    return 0;
  }

  *group = safe_strdup(groups[0]);

#if 0
  DNcited = (char *)globus_libc_calloc(DNlength + 3, sizeof(char));
  strcat(DNcited, cite);
  strcat(DNcited, DN);
  strcat(DNcited, cite);
  DNlength = strlen(DNcited);

  if (strcmp(DN,"") == 0) 
  {
    logMessage(1, "No group could be found for empty DN\n");

    globus_libc_free(DNcited);

    return 0;
  }

//read grid-map-file-mapping  
  FILE *f;
  char line[200];

  char *token;
  char *saveptr;

  f = fopen ("/etc/grid-security/grid-mapfile", "rt");  
/* open the file for reading */

  // find DN mapping group started with "."
  while(fgets(line, 100, f) != NULL)
  {
     // search for the line with the correct DN
     if (strstr(line, DNcited)){
       // "some DN" .grp
       //safe_strdup(getLastJobOutput());

       token = strtok_r(line, ".", &saveptr);
       // get what is after "." char
       token = strtok_r(NULL, ".", &saveptr);
       
       //remove end of line char
       token = strtok_r(token, "\n", &saveptr);
       logMessage(1, "token = %s", token);

       logMessage(1, "Checking if it is a pooled account mapping");
       if(token == NULL)
       {
	 logMessage(5, "Error user isn't mapped to the pooled account");
	 globus_libc_free(DNcited);
	 globus_libc_free(line);
	 return 0;	
       }

       *group = safe_strdup(token);
       break;
     }

  }
  fclose(f);  /* close the file prior to exiting the routine */

  globus_libc_free(DNcited);
  globus_libc_free(line);
#endif
  return 1;
}

/***********************************************************************
*   int canAddFile(char *user)
*
*   Check whether a certain user is allowed to add to the grid
*    
*   Parameters:                                         [I/O]
*
*     user     The user's certificate subject            I
*
*   Returns: 1 if the user is allowed to add the file, 0 if not
************************************************************************/
int canAddFile(char *user)
{
  /*
   * Currently only users belonging to group "ukq" 
   * are allowed to add the files to the grid.
   */
  char *group;

  if(!getUserGroup(user, &group))
  {
    logMessage(5, "Error obtaining group in canAddFile");
    //no need to free "group"
    return 0;
  }

  if(strcmp(group, "ukq") == 0)
  {
    globus_libc_free(group);
    return 1;
  }
  else
  {
      logMessage(1, "User '%s' belonging to group '%s' was trying to add to the grid", user, group);
      globus_libc_free(group);    
      return 0;
  }
}


/***********************************************************************
*   int getGroupID(char *name)
*
*   Returns a numeric representation of a given group name
*    
*   Parameters:                                         [I/O]
*
*     name     the name of the group                      I
*
*   Returns: 1 if the user is allowed to add the file, 0 if not
************************************************************************/
int getGroupID(char *name)
{
  /* 
   * TODO : should handle the error if the group doesn't exist
   */

  /* read /etc/group */
  FILE *f;
  char line[200];

  char *token = "-1";
  char *saveptr;

  f = fopen ("/etc/group", "rt");  
/* open the file for reading */

  while(fgets(line, 100, f) != NULL)
  {
     if (strstr(line, name)){

       token = strtok_r(line, ":", &saveptr);
       token = strtok_r(NULL, ":", &saveptr);

       /* third token should be the group id */
       token = strtok_r(NULL, ":", &saveptr);
       break;
     }
  }
/* close the file prior to exiting the routine */
  fclose(f);  

  return atoi(token);
}


/***********************************************************************
*   int forEachFileInDirectory(char *ldn,
*                              int (*callback)(char *lfn, void *param),
*			       void *cbparam)
*
*   Iterates over all the logical files in a logical directory and
*   calls the callback function for each one
*    
*   Parameters:                                         [I/O]
*
*     ldn       logical directory name                   I
*     callback  function to call for files in directory. I
*               Should return 1 to continue iteration,
*               0 to terminate straight away
*     cbparam   parameter to pass to callback            I
*
*   Returns: 1 if the iteration completed, 0 if it terminated early
************************************************************************/
int forEachFileInDirectory(char *ldn, int (*callback)(char *lfn, void *param),
			   void *cbparam)
{
    char *prefix;
    char *file;
    int pl;

    if (safe_asprintf(&prefix, "%s/", ldn) < 0)
    {
	logMessage(5, "Out of memory");
	return 0;
    }
    /* cope with ready-slashed directory name */
    if (prefix[strlen(prefix) - 2] == '/')
    {
	prefix[strlen(prefix) - 1] = 0;
    }

    pl = strlen(prefix);
    file = getFirstFile();
    while (file)
    {
	if (!strncmp(prefix, file, pl))
	{
	    if (!callback(file, cbparam))
	    {
		globus_libc_free(prefix);
		return 0;
	    }
	}

	file = getNextFile();
    }

    globus_libc_free(prefix);
    return 1;
}


/***********************************************************************
*   int copyToLocal(char *remoteHost, char *remoteFile, char *localFile)
*
*   Copies a file to the local file system from a remote host using
*   GridFTP
*    
*   Parameters:                                                     [I/O]
*
*     remoteHost  FQDN of host to copy to                            I
*     remoteFile  Full pathname to file on remote machine            I
*     localFile   Full pathname to file on local machine             I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
int copyToLocal(char *remoteHost, char *remoteFile, char *localFile)
{
  struct storageElement *se;
  int handle;
  time_t startTime, endTime;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  digs_error_code_t result;
  float timeout;
  int percentComplete;
  digs_transfer_status_t status;

  se = getNode(remoteHost);
  if (!se) {
    logMessage(ERROR, "Cannot find node %s", remoteHost);
    return 0;
  }

  result = se->digs_startGetTransfer(errbuf, remoteHost, remoteFile,
				     localFile, &handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error starting get transfer from %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  timeout = getNodeCopyTimeout(remoteHost);

  startTime = time(NULL);
  do {
    result = se->digs_monitorTransfer(errbuf, handle, &status, &percentComplete);
    if ((result != DIGS_SUCCESS) || (status == DIGS_TRANSFER_FAILED)) {
      logMessage(ERROR, "Error in get transfer from %s: %s (%s)", remoteHost,
		 digsErrorToString(result), errbuf);
      se->digs_endTransfer(errbuf, handle);
      return 0;
    }
    if (status == DIGS_TRANSFER_DONE) {
      break;
    }

    globus_libc_usleep(1000);

    endTime = time(NULL);
  } while (difftime(endTime, startTime) < timeout);

  result = se->digs_endTransfer(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error in end transfer from %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  return 1;
}


/***********************************************************************
*   int copyFromLocal(char *localFile, char *remoteHost, char *remoteFile)
*
*   Copies a file from the local file system to a remote host using
*   GridFTP, synchronously
*    
*   Parameters:                                                     [I/O]
*
*     localFile   Full pathname to file on local machine             I
*     remoteHost  FQDN of host to copy to                            I
*     remoteFile  Full pathname to file on remote machine            I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
int copyFromLocal(char *localFile, char *remoteHost, char *remoteFile)
{
  struct storageElement *se;
  int handle;
  time_t startTime, endTime;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  digs_error_code_t result;
  float timeout;
  int percentComplete;
  digs_transfer_status_t status;

  se = getNode(remoteHost);
  if (!se) {
    logMessage(ERROR, "Cannot find node %s", remoteHost);
    return 0;
  }

  result = se->digs_startPutTransfer(errbuf, remoteHost, localFile,
				     remoteFile, &handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error starting put transfer to %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  timeout = getNodeCopyTimeout(remoteHost);

  startTime = time(NULL);
  do {
    result = se->digs_monitorTransfer(errbuf, handle, &status, &percentComplete);
    if ((result != DIGS_SUCCESS) || (status == DIGS_TRANSFER_FAILED)) {
      logMessage(ERROR, "Error in put transfer to %s: %s (%s)", remoteHost,
		 digsErrorToString(result), errbuf);
      se->digs_endTransfer(errbuf, handle);
      return 0;
    }
    if (status == DIGS_TRANSFER_DONE) {
      break;
    }

    globus_libc_usleep(1000);

    endTime = time(NULL);
  } while (difftime(endTime, startTime) < timeout);

  result = se->digs_endTransfer(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    logMessage(ERROR, "Error in end transfer to %s: %s (%s)", remoteHost,
	       digsErrorToString(result), errbuf);
    return 0;
  }

  return 1;
}
