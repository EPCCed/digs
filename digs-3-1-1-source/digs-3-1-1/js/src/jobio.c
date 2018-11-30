/***********************************************************************
*
*   Filename:   jobio.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Handling of I/O streams for interactive jobs
*
*   Contents:   Functions for communicating with job on server
*
*   Used in:    QCDgrid client tools for job submission
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <globus_io.h>

#include "qcdgrid-js.h"

/*
 * Set if we are currently connected
 */
static int connectedToHost_=0;

/*
 * Communications handle for connection
 */
static globus_io_handle_t jobIOSock_;

/***********************************************************************
*   globus_bool_t jioAuthCallback2(void *arg, globus_io_handle_t *handle,
*	 		          globus_result_t result, char *identity, 
*	    		          gss_ctx_id_t *context_handle)
*    
*   Authorisation callback required for sending a secure message back
*   to the submitter.
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
static globus_bool_t jioAuthCallback(void *arg, globus_io_handle_t *handle,
				    globus_result_t result, char *identity, 
				    gss_ctx_id_t *context_handle)
{
    /*
     * Always allow this - the meaningful authorisation takes place
     * on the server side
     */
    return GLOBUS_TRUE;
}

/*
 * Connect to system actually running job
 */
int connectToHost(char *hostname, int port)
{
    globus_io_attr_t attr;
    globus_result_t errorCode;
    globus_io_secure_authorization_data_t authData;

    if (globus_io_secure_authorization_data_initialize(&authData)
	!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error initialising autorization data structure\n");
	return 0;
    }

    if (globus_io_secure_authorization_data_set_callback(&authData,
							 jioAuthCallback, NULL)
	!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting authorization callback\n");
	return 0;
    }

    if (globus_io_tcpattr_init(&attr)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error initialising TCP attribute structure\n");
	return 0;
    }

    if (globus_io_attr_set_secure_authentication_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHENTICATION_MODE_GSSAPI, 
	    GSS_C_NO_CREDENTIAL)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting authentication mode to GSSAPI\n");
	return 0;
    }                                            

    if (globus_io_attr_set_secure_proxy_mode (&attr, 
					      GLOBUS_IO_SECURE_PROXY_MODE_MANY)
	!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting proxy mode to many\n");
	return 0;
    }

    if (globus_io_attr_set_secure_authorization_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHORIZATION_MODE_CALLBACK, 
	    &authData)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting authorization mode to callback\n");
	return 0;
    }

    if (globus_io_attr_set_secure_channel_mode(
	    &attr, GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting channel mode to GSI wrap\n");
	return 0;
    }

    /* Increase socket buffer sizes - defaults are too small */
    errorCode=globus_io_attr_set_socket_sndbuf(&attr, 1024);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting socket send buffer size\n");
	return 0;
    }

    errorCode=globus_io_attr_set_socket_rcvbuf(&attr, 1024);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error setting socket receive buffer size\n");
	return 0;
    }

    errorCode=globus_io_attr_set_tcp_nodelay(&attr, GLOBUS_TRUE);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	return 0;
    }

    errorCode=globus_io_tcp_connect(hostname, port, &attr, &jobIOSock_);
    if (errorCode!=GLOBUS_SUCCESS)
    {
/*	jwPrintError(errorCode, "globus_io_tcp_connect");*/
	fprintf(stderr, "Error connecting to %s:%d\n", hostname, port);
	return 0;
    }

    connectedToHost_=1;
    return 1;
}

/*
 * Reads new data from job stdout/stderr stream. Returns number of bytes.
 * Returns a dynamically allocated buffer containing the data in buffer param
 */
int getNewOutputData(char **buffer, int isStderr)
{
    unsigned char msgBuffer[4];
    int n;
    int len;
    globus_result_t retval;

    if (!connectedToHost_)
    {
	fprintf(stderr, "Get output attempted when not connected\n");
	return -1;
    }

    if (!isStderr)
    {
	msgBuffer[0]=QJS_MSG_STDOUT;
    }
    else
    {
	msgBuffer[0]=QJS_MSG_STDERR;
    }

    if (globus_io_write(&jobIOSock_, msgBuffer, 1, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error sending message to host\n");
	return -1;
    }

    if (retval=globus_io_read(&jobIOSock_, msgBuffer, 4, 4, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error reading response from host\n");
	printError(retval, "globus_io_read");
	return -1;
    }

    len=msgBuffer[0]|(msgBuffer[1]<<8)|(msgBuffer[2]<<16)|(msgBuffer[3]<<24);
//    printf("%d bytes on stdout/err\n", len);
    if (len==0)
    {
	return 0;
    }

    *buffer=malloc(len);
    if (!(*buffer))
    {
	fprintf(stderr, "Out of memory\n");
	return -1;
    }

    if (globus_io_read(&jobIOSock_, *buffer, len, len, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error reading data from host\n");
	free(*buffer);
	return -1;
    }

    return len;
}

int sendNewInputData(char *buffer, int len)
{
    unsigned char msgBuffer[5];
    int n;

    if (!connectedToHost_)
    {
	fprintf(stderr, "Send input attempted when not connected\n");
	return 0;
    }

    if (len==0)
    {
	return 1;
    }

    msgBuffer[0]=QJS_MSG_STDIN;
    msgBuffer[1]=len&0xff;
    msgBuffer[2]=(len>>8)&0xff;
    msgBuffer[3]=(len>>16)&0xff;
    msgBuffer[4]=(len>>24)&0xff;

    if (globus_io_write(&jobIOSock_, msgBuffer, 5, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error sending message to host\n");
	return 0;
    }

    if (globus_io_write(&jobIOSock_, buffer, len, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error sending data to host\n");
	return 0;
    }

    return 1;
}

int getJobState()
{
    unsigned char msgBuffer[1];
    int n;

    if (!connectedToHost_)
    {
	fprintf(stderr, "Get job state attempted when not connected\n");
	return -1;
    }

    msgBuffer[0]=QJS_MSG_STATE;
    if (globus_io_write(&jobIOSock_, msgBuffer, 1, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error sending message to host\n");
	return -1;
    }

    if (globus_io_read(&jobIOSock_, msgBuffer, 1, 1, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error reading response from host\n");
	return -1;
    }

    return msgBuffer[0];
}

char **getOutputFileNames()
{
    unsigned char msgBuffer[1000];
    char **result=NULL;
    int n, i=1;
    int len;

    if (!connectedToHost_)
    {
	fprintf(stderr, "Get output filenames attempted when not connected\n");
	return NULL;
    }

    msgBuffer[0]=QJS_MSG_OUTFILES;
    if (globus_io_write(&jobIOSock_, msgBuffer, 1, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error sending message to host\n");
	return NULL;
    }

    if (globus_io_read(&jobIOSock_, msgBuffer, 4, 4, &n)!=GLOBUS_SUCCESS)
    {
	fprintf(stderr, "Error reading response from host\n");
	return NULL;
    }

    len=msgBuffer[0]|(msgBuffer[1]<<8)|(msgBuffer[2]<<16)|(msgBuffer[3]<<24);

    /* in case there's no output files */
    result=malloc(sizeof(char*));
    
    while (len)
    {
	i++;
	result=realloc(result, i*sizeof(char*));
	if (!result)
	{
	    fprintf(stderr, "Out of memory\n");
	    return NULL;
	}
	
	result[i-2]=malloc(len+1);

	if (globus_io_read(&jobIOSock_, result[i-2], len, len, &n)!=GLOBUS_SUCCESS)
	{
	    fprintf(stderr, "Error reading response from host\n");
	    return NULL;
	}

	result[i-2][len]=0;

	if (globus_io_read(&jobIOSock_, msgBuffer, 4, 4, &n)!=GLOBUS_SUCCESS)
	{
	    fprintf(stderr, "Error reading response from host\n");
	    return NULL;
	}
	
	len=msgBuffer[0]|(msgBuffer[1]<<8)|(msgBuffer[2]<<16)|(msgBuffer[3]<<24);
    }

    result[i-1]=NULL;
    return result;
}
