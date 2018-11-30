/***********************************************************************
*
*   Filename:   QCDgridMessageSender.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implements interface between Globus I/O and QCDgrid
*               Java code
*
*   Contents:   QCDgridMessageSender native method implementations
*
*   Used in:    QCDgrid Java code
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

#include "uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridMessageSender.h"

/*
 * Macro for easily throwing a QCDgridException
 */
#define THROW_QCDGRID_EXCEPTION(env, str) (*env)->ThrowNew(env, (*env)->FindClass(env, "uk/ac/ed/epcc/qcdgrid/QCDgridException"), str);

/*
 * Utility function for converting a Java string object to a C string (char array)
 * Returned pointer points to malloced storage and should be freed by the caller
 */
static char *jstringToCstring(JNIEnv *env, jstring str)
{
    int len;
    char *cstr;

    len=(*env)->GetStringUTFLength(env, str);

    cstr=malloc(len+1);
    if (!cstr)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return NULL;
    }

    (*env)->GetStringUTFRegion(env, str, 0, len, cstr);
    return cstr;
}

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
globus_bool_t authCallback2(void *arg, globus_io_handle_t *handle,
			    globus_result_t result, char *identity, 
			    gss_ctx_id_t *context_handle)
{
    return GLOBUS_TRUE;
}

/*
 * Actually sends a message to the main node. Hostname, port number and message string are specified in
 * parameters. Main node's response is returned
 */
JNIEXPORT jstring JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridMessageSender_doSend(JNIEnv *env, jobject obj, jstring host, jstring msg, jint port)
{
    char *chost, *cmsg;
    jstring response;

    globus_io_handle_t sock;
    globus_io_attr_t attr;
    int n;
    globus_result_t errorCode;
    globus_io_secure_authorization_data_t authData;
    char responseBuffer[33];
    
    /* Convert Java args to C format */
    chost=jstringToCstring(env, host);
    cmsg=jstringToCstring(env, msg);
    if ((!chost)||(!cmsg))
    {
	return NULL;
    }

    if (globus_io_secure_authorization_data_initialize(&authData)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error initialising authorisation data structure");
	return NULL;
    }

    if (globus_io_secure_authorization_data_set_callback(&authData,
							 authCallback2, NULL)
	!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting authorization callback");
	return NULL;
    }

    if (globus_io_tcpattr_init(&attr)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error initialising TCP attribute structure");
	return NULL;
    }

    if (globus_io_attr_set_secure_authentication_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHENTICATION_MODE_GSSAPI, 
	    GSS_C_NO_CREDENTIAL)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting authentication mode to GSSAPI");
	return NULL;
    }                                            

    if (globus_io_attr_set_secure_proxy_mode (&attr, 
					      GLOBUS_IO_SECURE_PROXY_MODE_MANY)
	!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting proxy mode to many");
	return NULL;
    }

    if (globus_io_attr_set_secure_authorization_mode (
	    &attr, GLOBUS_IO_SECURE_AUTHORIZATION_MODE_CALLBACK, 
	    &authData)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting authorization mode to callback\n");
	return NULL;
    }

    if (globus_io_attr_set_secure_channel_mode(
	    &attr, GLOBUS_IO_SECURE_CHANNEL_MODE_GSI_WRAP)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting channel mode to GSI wrap");
	return NULL;
    }

    /* Increase socket buffer sizes - defaults are too small */
    errorCode=globus_io_attr_set_socket_sndbuf(&attr, 1024);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error increasing send socket buffer size");
	return NULL;
    }

    errorCode=globus_io_attr_set_socket_rcvbuf(&attr, 1024);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error increasing receive socket buffer size");
	return NULL;
    }

    errorCode=globus_io_tcp_connect(chost, port, &attr, &sock);
    if (errorCode!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error connecting to main node");
	return NULL;
    }

    if (globus_io_write(&sock, cmsg, strlen(cmsg)+1, &n)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error writing to main node");
	globus_io_close(&sock);
	return NULL;
    }

    errorCode=globus_io_read(&sock, responseBuffer, 32, 32, &n);

    while ((n==0)&(errorCode==GLOBUS_SUCCESS))
    {
	errorCode=globus_io_read(&sock, responseBuffer, 32, 32, &n);
    }

    if (n)
    {
	responseBuffer[n]=0;
    }
    else
    {
	THROW_QCDGRID_EXCEPTION(env, "Error waiting for main node's response");
	globus_io_close(&sock);
	return NULL;
    }

    globus_io_close(&sock);

    response=(*env)->NewStringUTF(env, responseBuffer);

    return response;
}

/*
 * Starts up the Globus I/O module needed by this code
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridMessageSender_activateGlobusIOModule(JNIEnv *env, jclass cls)
{
    globus_module_activate(GLOBUS_IO_MODULE);    
}
