/***********************************************************************
*
*   Filename:   background-msg.c
*
*   Authors:    James Perry, Radoslaw Ostrowski (jamesp, radek)   EPCC.
*
*   Purpose:    Message handling for control thread
*
*   Contents:   Message functions and data structures
*
*   Used in:    Background process on main node of QCDgrid
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003-2007 The University of Edinburgh
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

#include <globus_io.h>
#include <globus_common.h>

#include "node.h"
#include "replica.h"

#include "background-delete.h"
#include "background-new.h"
#include "background-permissions.h"
#include "background-modify.h"

void touchDirectory(char *destination, char *dir);
int iLikeThisFile(char *destination, char *file);

int canSetReplCountFile(char *lfn, char *user);
int canSetReplCountDir(char *lfn, char *user);
int setReplCountFile(char *lfn, int count);
int setReplCountDir(char *lfn, int count);

/*
 * Administrators list in background.c
 */
extern char **administrators_;
extern int numAdministrators_;

/*
 * The port on which to listen for messages. Stored in misc.c which needs
 * to know where to send messages.
 */
extern int qcdgridPort_;

/*
 * Enumeration of message types
 */
enum { MT_ADD, MT_TOUCHDIR, MT_TOUCH, MT_CHECK, MT_REMOVE, MT_DISABLE,
       MT_ENABLE, MT_DELETE, MT_RMDIR, MT_RETIRE, MT_UNRETIRE, MT_PING,
       MT_LOCK, MT_UNLOCK, MT_LOCKDIR, MT_UNLOCKDIR, MT_REPLCOUNT,
       MT_REPLCOUNTDIR, MT_MODIFY
};

/*
 * Structure containing one received message
 */
typedef struct qcdgridMessage_s {

    int type;
    int numParams;
    char **params;
    char *sender;
    struct qcdgridMessage_s *next;

} qcdgridMessage_t;

/*
 * Set if the sender of the message currently being processed has
 * administrator privileges
 */
static int privileged_;

/*
 * The message sender's certificate subject
 */
static char *messageSenderID_;

/*=====================================================================
 *
 * Message authorisation functions. Each one takes a received message
 * and, using the privileged_ and messageSenderID_ values, returns 1
 * if the message is allowed or 0 if it is not.
 *
 *===================================================================*/
static int authAdminOnly(qcdgridMessage_t *msg)
{
    return privileged_;
}

static int authUKQCDGroupOnly(qcdgridMessage_t *msg)
{
    char *group;
    char *authGroup="ukq";

    if (privileged_) return 1;

    if(getUserGroup(messageSenderID_, &group) == 0)
    {
      logMessage(5, "Error obtaining user group");
      return privileged_;
    }

    /* user belongs to ukq group */
    if (strcmp(group, authGroup) == 0)
    {
	globus_libc_free(group);
	return 1;
    }
    else
    {
	globus_libc_free(group);
	return privileged_;
    }
}

static int authPutFile(qcdgridMessage_t *msg)
{
  if (privileged_) return 1;

  /* check group map for whether user's allowed to put files into the group they specified */
  if (!userInGroup(messageSenderID_, msg->params[1])) {
    return 0;
  }

  /* still only allow UKQCD group to put files for now */
  if (!authUKQCDGroupOnly(msg)) return 0;
  return 1;
}

static int authAlways(qcdgridMessage_t *msg)
{
    return 1;
}

static int authDeleteFile(qcdgridMessage_t *msg)
{
    return privileged_ || canDeleteFile(msg->params[0], messageSenderID_);
}

static int authDeleteDir(qcdgridMessage_t *msg)
{
    return privileged_ || canDeleteDirectory(msg->params[0], messageSenderID_);
}

static int authLockFile(qcdgridMessage_t *msg)
{
    return privileged_ || canLockFile(msg->params[0], messageSenderID_);
}

static int authUnlockFile(qcdgridMessage_t *msg)
{
    return privileged_ || canUnlockFile(msg->params[0], messageSenderID_);
}

static int authLockDirectory(qcdgridMessage_t *msg)
{
    return privileged_ || canLockDirectory(msg->params[0], messageSenderID_);
}

static int authUnlockDirectory(qcdgridMessage_t *msg)
{
    return privileged_ || canUnlockDirectory(msg->params[0], messageSenderID_);
}

static int authReplCountFile(qcdgridMessage_t *msg)
{
    return privileged_ || canSetReplCountFile(msg->params[0], messageSenderID_);
}

static int authReplCountDirectory(qcdgridMessage_t *msg)
{
    return privileged_ || canSetReplCountDir(msg->params[0], messageSenderID_);
}

static int authModify(qcdgridMessage_t *msg)
{
    return privileged_ || canModify(msg->params[0], messageSenderID_);
}

/*=====================================================================
 *
 * Message handler functions
 *
 *===================================================================*/
static void handleMessageAdd(qcdgridMessage_t *msg)
{
    logMessage(5, "Adding node %s", msg->params[0]);
    addNode(msg->params[0], msg->params[1], (long long) 0, msg->params[2]);
}

static void handleMessageTouchdir(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[1]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[1]);
	return;
    }
    logMessage(5, "Touching directory %s on %s", msg->params[0], msg->params[1]);
    touchDirectory(msg->params[1], msg->params[0]);
}

static void handleMessageTouch(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[1]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[1]);
	return;
    }
    logMessage(5, "Touching file %s on %s", msg->params[0], msg->params[1]);
    iLikeThisFile(msg->params[1], msg->params[0]);
}

static void handleMessageCheck(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[0]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[0]);
	return;
    }
    addToCheckList(msg->params[0]);
}

static void handleMessageRemove(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[0]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[0]);
	return;
    }
    logMessage(5, "Removing node %s", msg->params[0]);
    removeFromDisabledList(msg->params[0]);
    removeFromDeadList(msg->params[0]);
    removeFromRetiringList(msg->params[0]);
    removeNode(nodeIndexFromName(msg->params[0]));
    
    /* remove its replica catalog entries */
    deleteEntireLocation(msg->params[0]);
}

static void handleMessageDisable(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[0]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[0]);
	return;
    }
    logMessage(5, "Disabling node %s", msg->params[0]);
    addToDisabledList(msg->params[0]);
}

static void handleMessageEnable(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[0]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[0]);
	return;
    }
    logMessage(5, "Enabling node %s", msg->params[0]);
    removeFromDisabledList(msg->params[0]);
}

static void handleMessageDelete(qcdgridMessage_t *msg)
{
    logMessage(5, "Deleting file %s", msg->params[0]);
    deleteLogicalFile(msg->params[0]);
}

static void handleMessageRmdir(qcdgridMessage_t *msg)
{
    logMessage(5, "Deleting directory %s", msg->params[0]);
    deleteDirectory(msg->params[0]);
}

static void handleMessageRetire(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[0]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[0]);
	return;
    }
    logMessage(5, "Retiring node %s", msg->params[0]);
    addToRetiringList(msg->params[0]);
}

static void handleMessageUnretire(qcdgridMessage_t *msg)
{
    if (nodeIndexFromName(msg->params[0]) < 0)
    {
	logMessage(5, "processMessages: node %s doesn't exist", msg->params[0]);
	return;
    }
    logMessage(5, "Unretiring node %s", msg->params[0]);
    removeFromRetiringList(msg->params[0]);
}

static void handleMessagePing(qcdgridMessage_t *msg)
{

}

static void handleMessagePutFile(qcdgridMessage_t *msg)
{
  logMessage(5, "Putting file on a grid: %s", msg->params[0]);
  addPendingAdd(msg->numParams, msg->params);
}

static void handleMessageChmod(qcdgridMessage_t *msg)
{
  logMessage(5, "Request to change permissions of file[s]: %s", msg->params[2]);
  addPendingPermissions(msg->numParams, msg->params);
}

static void handleMessageLock(qcdgridMessage_t *msg)
{
    logMessage(5, "Locking file %s", msg->params[0]);
    if (!lockFile(msg->params[0], msg->sender))
    {
	logMessage(5, "Error locking file %s", msg->params[0]);
    }
}

static void handleMessageUnlock(qcdgridMessage_t *msg)
{
    logMessage(5, "Unlocking file %s", msg->params[0]);
    if (!unlockFile(msg->params[0], msg->sender))
    {
	logMessage(5, "Error unlocking file %s", msg->params[0]);
    }
}

static void handleMessageLockdir(qcdgridMessage_t *msg)
{
    logMessage(5, "Locking directory %s", msg->params[0]);
    if (!lockDirectory(msg->params[0], msg->sender))
    {
	logMessage(5, "Error locking directory %s", msg->params[0]);
    }
}

static void handleMessageUnlockdir(qcdgridMessage_t *msg)
{
    logMessage(5, "Unlocking directory %s", msg->params[0]);
    if (!unlockDirectory(msg->params[0], msg->sender))
    {
	logMessage(5, "Error unlocking directory %s", msg->params[0]);
    }
}

static void handleMessageReplcount(qcdgridMessage_t *msg)
{
  logMessage(5, "Setting replication count for %s", msg->params[0]);
  if (!setReplCountFile(msg->params[0], atoi(msg->params[1]))) {
    logMessage(5, "Error setting replication count for %s", msg->params[0]);
  }
}

static void handleMessageReplcountdir(qcdgridMessage_t *msg)
{
  logMessage(5, "Setting replication count for directory %s", msg->params[0]);
  if (!setReplCountDir(msg->params[0], atoi(msg->params[1]))) {
    logMessage(5, "Error setting replication count for directory %s", msg->params[0]);
  }
}

static void handleMessageModify(qcdgridMessage_t *msg)
{
    logMessage(5, "Modifying file %s", msg->params[0]);
    if (!addFileModification(msg->params[0], msg->params[1], msg->params[2],
			     msg->params[3], msg->params[4])) {
	logMessage(5, "Error modifying file %s", msg->params[0]);
    }
}

/*
 * All the possible message types are in this list
 */
static struct {

    char *name;
    int minParams;
    int maxParams;
    int (*authorise)(qcdgridMessage_t *msg);
    void (*handler)(qcdgridMessage_t *msg);
    
} messageTypes[] =
{
    { "add", 3, 3, authAdminOnly, handleMessageAdd },
    { "touchdir", 2, 2, authAlways, handleMessageTouchdir },
    { "touch", 2, 2, authAlways, handleMessageTouch },
    { "check", 1, 1, authAlways, handleMessageCheck },
    { "remove", 1, 1, authAdminOnly, handleMessageRemove },
    { "disable", 1, 1, authAdminOnly, handleMessageDisable },
    { "enable", 1, 1, authAdminOnly, handleMessageEnable },
    { "delete", 1, 1, authDeleteFile, handleMessageDelete },
    { "rmdir", 1, 1, authDeleteDir, handleMessageRmdir },
    { "retire", 1, 1, authAdminOnly, handleMessageRetire },
    { "unretire", 1, 1, authAdminOnly, handleMessageUnretire },
    { "ping", 0, 0, authAlways, handleMessagePing },
//RADEK changed to authUKQCDGroupOnly from authAlways
    { "putFile", 7, 7, authPutFile, handleMessagePutFile },
    { "chmod", 4, 4, authUKQCDGroupOnly, handleMessageChmod },
    { "lock", 1, 1, authLockFile, handleMessageLock },
    { "unlock", 1, 1, authUnlockFile, handleMessageUnlock },
    { "lockdir", 1, 1, authLockDirectory, handleMessageLockdir },
    { "unlockdir", 1, 1, authUnlockDirectory, handleMessageUnlockdir },
    { "replcount", 2, 2, authReplCountFile, handleMessageReplcount },
    { "replcountdir", 2, 2, authReplCountDirectory, handleMessageReplcountdir },
    { "modify", 5, 5, authModify, handleMessageModify },
    { NULL, -1, -1, NULL, NULL }
};

/*
 * This linked list is a queue of received messages. The listener
 * callback function adds them to the end of the queue, the
 * processMessages function called from the main loop pops them
 * off the front.
 */
static qcdgridMessage_t *msgListHead_ = NULL;

/*
 * Mutex protects the message queue from concurrent accesses
 */
static globus_mutex_t msgListLock_;

/***********************************************************************
*   void freeMessage(qcdgridMessage_t *msg)
*    
*   Frees all the memory used by a message structure
*    
*   Parameters:                                [I/O]
*
*     msg   the message to free                 I
*
*   Returns: (void)
************************************************************************/
static void freeMessage(qcdgridMessage_t *msg)
{
    int i;
    if (msg->params)
    {
	for (i = 0; i < msg->numParams; i++)
	{
	    globus_libc_free(msg->params[i]);
	}
	globus_libc_free(msg->params);
    }
    if (msg->sender)
    {
	globus_libc_free(msg->sender);
    }
    globus_libc_free(msg);
}

/***********************************************************************
*   qcdgridMessage_t *parseMessage(char *str)
*    
*   Parses a string representation of a message into a message structure
*   Arguments should be seperated by a single space 
*    
*   Parameters:                                [I/O]
*
*     str   the string to parse                 I
*
*   Returns: structure containing the message, or NULL on failure
************************************************************************/
static qcdgridMessage_t *parseMessage(char *str)
{
    qcdgridMessage_t *msg;
    char *str2;
    char *nextSpc;
    char *paramStart;
    int i;

    logMessage(1, "parseMessage(%s)", str);

    msg = globus_libc_malloc(sizeof(qcdgridMessage_t));
    str2 = safe_strdup(str);
    if ((!msg) || (!str2))
    {
	errorExit("Out of memory in parseMessage");
    }

    msg->next = NULL;

    /*
     * Parse the parameters out of the string
     */
    msg->numParams = 0;
    msg->params = NULL;
    nextSpc = strchr(str2, ' ');
    while (nextSpc)
    {
	msg->numParams++;
	nextSpc = strchr(nextSpc + 1, ' ');
    }

    if (msg->numParams > 0)
    {
	msg->params = globus_libc_malloc(sizeof(char*) * msg->numParams);
	if (!msg->params)
	{
	    errorExit("Out of memory in parseMessage");
	}

	nextSpc = strchr(str2, ' ');
	i = 0;
	while (i < msg->numParams)
	{
	    paramStart = nextSpc + 1;
	    nextSpc = strchr(paramStart, ' ');
	    if (nextSpc)
	    {
		*nextSpc = 0;
	    }
	    
	    msg->params[i] = safe_strdup(paramStart);
	    if (!msg->params[i])
	    {
		errorExit("Out of memory in parseMessage");
	    }

	    i++;
	}
    }

    /* NULL terminate message name */
    nextSpc = strchr(str2, ' ');
    if (nextSpc)
    {
	*nextSpc = 0;
    }

    /*
     * Look for a matching message type
     */
    msg->type = -1;
    i = 0;
    while (messageTypes[i].name)
    {
	if (!strcmp(str2, messageTypes[i].name))
	{
	    if ((msg->numParams >= messageTypes[i].minParams) &&
		(msg->numParams <= messageTypes[i].maxParams))
	    {
		msg->type = i;
	    }
	}
	i++;
    }

    if (msg->type < 0)
    {
	logMessage(5, "No matching type for message %s with %d params", str2, msg->numParams);
	freeMessage(msg);
	globus_libc_free(str2);
	return NULL;
    }

    globus_libc_free(str2);

    msg->sender = NULL;
    return msg;
}

#if 0
/***********************************************************************
*   void dumpMessage(qcdgridMessage_t *msg)
*    
*   Debugging function: dumps message contents to stdout
*    
*   Parameters:                                [I/O]
*
*     msg   the message to dump                 I
*
*   Returns: (void)
************************************************************************/
static void dumpMessage(qcdgridMessage_t *msg)
{
    int i;
    globus_libc_printf("Type: %d\n", msg->type);
    globus_libc_printf("Params:\n");
    for (i = 0; i < msg->numParams; i++)
    {
	globus_libc_printf("  %s\n", msg->params[i]);
    }
}
#endif

/***********************************************************************
*   void addMessageToQueue(qcdgridMessage_t *msg)
*    
*   Adds a new message to the end of the message queue
*    
*   Parameters:                                [I/O]
*
*     msg   the message to queue                I
*
*   Returns: (void)
************************************************************************/
static void addMessageToQueue(qcdgridMessage_t *msg)
{
    qcdgridMessage_t *msg2;

    msg->next = NULL;

    globus_mutex_lock(&msgListLock_);
    msg2 = msgListHead_;
    if (!msg2)
    {
	msgListHead_ = msg;
    }
    else
    {
	while (msg2->next)
	{
	    msg2 = msg2->next;
	}
	msg2->next = msg;
    }
    globus_mutex_unlock(&msgListLock_);
}

/***********************************************************************
*   qcdgridMessage_t *getMessageFromQueue()
*    
*   Pops a message from the front of the message queue
*    
*   Parameters:                                [I/O]
*
*     (none)
*
*   Returns: the first message from the queue, or NULL if queue was empty
************************************************************************/
static qcdgridMessage_t *getMessageFromQueue()
{
    qcdgridMessage_t *msg;

    globus_mutex_lock(&msgListLock_);

    msg = msgListHead_;
    if (msg)
    {
	msgListHead_ = msg->next;
    }

    globus_mutex_unlock(&msgListLock_);

    return msg;
}

/***********************************************************************
*   globus_bool_t authCallback(void *arg, globus_io_handle_t *handle,
*                              globus_result_t result, char *identity, 
*                              gss_ctx_id_t *context_handle)
*    
*   Authorisation callback used to establish whether the sender of the
*   message is allowed administrator privileges or not.
*    
*   Parameters:                                             [I/O]
*
*     identity  pointer to the sender's certificate subject  I
*
*   Returns: GLOBUS_TRUE
************************************************************************/
static globus_bool_t authCallback(void *arg, globus_io_handle_t *handle,
				  globus_result_t result, char *identity, 
				  gss_ctx_id_t *context_handle)
{
    int i;

    privileged_ = 0;

    for (i = 0; i < numAdministrators_; i++)
    {
	if (!strcmp(identity, administrators_[i]))
	{
	    privileged_ = 1;
	}
    }

    messageSenderID_ = safe_strdup(identity);

    return GLOBUS_TRUE;
}

/* socket on which we listen for messages */
static globus_io_handle_t sock_;

/***********************************************************************
*   void listenCallback(void *arg, globus_io_handle_t *handle,
*                       globus_result_t result)
*    
*   Called when a connection comes in on our Globus I/O socket
*    
*   Parameters:                                [I/O]
*
*     arg      not used
*     handle   handle of connection             I
*     result   not used
*
*   Returns: (void)
************************************************************************/
static void listenCallback(void *arg, globus_io_handle_t *handle,
			   globus_result_t result)
{
    globus_io_handle_t sock2;
    globus_result_t errorCode;
    char msgBuffer[2048];
    globus_size_t n, o;

    qcdgridMessage_t *msg;

    logMessage(1, "listenCallback()");

    /* accept the connection */
    errorCode = globus_io_tcp_accept(&sock_, GLOBUS_NULL, &sock2);
    if (errorCode == GLOBUS_IO_ERROR_TYPE_AUTHENTICATION_FAILED)
    {
	logMessage(5, "Authentication failed on message socket");
	if (globus_io_tcp_register_listen(&sock_, listenCallback, NULL) != GLOBUS_SUCCESS)
	{
	    logMessage(5, "Error listening on Globus socket");
	}
	globus_libc_free(messageSenderID_);
	return;
    }
	
    if (errorCode == GLOBUS_IO_ERROR_TYPE_SYSTEM_FAILURE)
    {
	logMessage(5, "System error accepting message connection");
    }
    if (errorCode != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error accepting connection on Globus socket");
	if (globus_io_tcp_register_listen(&sock_, listenCallback, NULL) != GLOBUS_SUCCESS)
	{
	    logMessage(5, "Error listening on Globus socket");
	}
	globus_libc_free(messageSenderID_);
	return;
    }

    /* read the message */
    errorCode = globus_io_read(&sock2, (globus_byte_t *)msgBuffer, 2046, 1, &n);
    /*
     * globus_io_read seems to be returning undocumented error values at
     * random, even when the read succeeds (for all intents and purposes).
      */
    if (errorCode != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error reading from Globus socket");
	globus_io_close(&sock2);

	if (globus_io_tcp_register_listen(&sock_, listenCallback, NULL) != GLOBUS_SUCCESS)
	{
	    logMessage(5, "Error listening on Globus socket");
	}
	globus_libc_free(messageSenderID_);
	return;
    }
    msgBuffer[n] = 0;

    logMessage(5, "Received message: %s", msgBuffer);

    /* parse the message */
    msg = parseMessage(msgBuffer);
    if (!msg)
    {
	logMessage(5, "Parsing message failed");

	globus_io_close(&sock2);
	if (globus_io_tcp_register_listen(&sock_, listenCallback, NULL) != GLOBUS_SUCCESS)
	{
	    logMessage(5, "Error listening on Globus socket");
	}
	globus_libc_free(messageSenderID_);
	return;
    }

    msg->sender = safe_strdup(messageSenderID_);

    /* find out if user is authorised and send response */
    if (messageTypes[msg->type].authorise(msg))
    {
        errorCode = globus_io_write(&sock2, (globus_byte_t *)"OK                              ", 32, &o);
	if (errorCode != GLOBUS_SUCCESS)
	{
	    logMessage(5, "Error writing to socket");
	}
	if (o != 32)
	{
	    logMessage(5, "Wrong no. of bytes written - %d", (int) o);
	}

	logMessage(5, "Message authorisation succeeded, adding to queue");
	/* authorisation succeeded, so queue message up */
	addMessageToQueue(msg);
    }
    else
    {
        errorCode = globus_io_write(&sock2, (globus_byte_t *)"Insufficient privileges         ", 32, &n);
	if (errorCode != GLOBUS_SUCCESS)
	{
	    logMessage(5, "Error writing to socket");
	}
	if (n != 32)
	{
	    logMessage(5, "Wrong no. of bytes written - %d", (int) n);
	}
	freeMessage(msg);
	logMessage(5, "Message authorisation failed");
    }

    globus_io_close(&sock2);
    globus_libc_free(messageSenderID_);

    /* receive next message */
    if (globus_io_tcp_register_listen(&sock_, listenCallback, NULL) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error listening on Globus socket");
    }
}

/***********************************************************************
*   int listenForMessages()
*    
*   Opens a TCP/IP socket and listens for messages coming in from
*   other nodes. Queues them up in a list for the main thread of the
*   background process.
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: 1 on success, 0 on failure
************************************************************************/
int listenForMessages()
{
    globus_io_attr_t attr;
    unsigned short port;
    globus_result_t errorCode;

    globus_io_secure_authorization_data_t authData;

    logMessage(1, "listenForMessages()");

    if (globus_mutex_init(&msgListLock_, NULL) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error initialising message list mutex");
	return 0;
    }

    port = qcdgridPort_;

    if (globus_io_secure_authorization_data_initialize(&authData)
	!= GLOBUS_SUCCESS)
    {
	logMessage(5, "Error initialising autorization data structure");
	return 0;
    }

    if (globus_io_secure_authorization_data_set_callback(&authData,
							 (globus_io_secure_authorization_callback_t) authCallback, NULL)
	!= GLOBUS_SUCCESS)
    {
	logMessage(5, "Error setting authorization callback");
	return 0;
    }

    if (globus_io_tcpattr_init(&attr) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error initialising TCP attribute structure");
	return 0;
    }

    if (globus_io_attr_set_secure_authentication_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHENTICATION_MODE_GSSAPI, 
	    GSS_C_NO_CREDENTIAL) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error setting authentication mode to GSSAPI");
	return 0;
    }

    if (globus_io_attr_set_secure_proxy_mode (&attr, 
					      GLOBUS_IO_SECURE_PROXY_MODE_MANY)
	!= GLOBUS_SUCCESS)
    {
	logMessage(5, "Error setting proxy mode to many");
	return 0;
    }

    if (globus_io_attr_set_secure_authorization_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHORIZATION_MODE_CALLBACK, 
	    &authData) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error setting authorization mode to callback");
	return 0;
    }

    if (globus_io_attr_set_secure_channel_mode(
	    &attr, GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error setting channel mode to GSI wrap");
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

    if (globus_io_tcp_create_listener(&port, -1, &attr, &sock_)
	!= GLOBUS_SUCCESS)
    {
	logMessage(5, "Error creating Globus listener");
	return 0;
    }

    if (globus_io_tcp_register_listen(&sock_, listenCallback, NULL) != GLOBUS_SUCCESS)
    {
	logMessage(5, "Error listening on Globus socket");
	return 0;
    }

    return 1;
}


/***********************************************************************
*   void processMessages()
*    
*   Called once every time round the control thread's main loop to read
*   messages that have been received from other nodes and act upon them
*    
*   Parameters:                                [I/O]
*
*     None
*
*   Returns: (void).
***********************************************************************/
void processMessages()
{
    qcdgridMessage_t *msg;

    /* process messages in queue until all done */
    msg = getMessageFromQueue();
    while (msg)
    {
	logMessage(5, "Got message type %d from queue", msg->type);
	messageTypes[msg->type].handler(msg);
	freeMessage(msg);
	msg = getMessageFromQueue();
    }
}
