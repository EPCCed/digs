/***********************************************************************
*
*   Filename:   QCDgridReplicaCatalogue.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Wraps the Globus replica catalogue with JNI calls,
*               allowing it to be accessed from Java
*
*   Contents:   Implementations of native methods of
*               QCDgridReplicaCatalogue class
*
*   Used in:    Java QCDgrid software
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

#include <globus_replica_catalog.h>
#include <ldap.h>

#include "uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue.h"

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

/*
 * Utility to get the handle pointer from its dodgy home
 */
static globus_replica_catalog_collection_handle_t *getHandle(JNIEnv *env, jobject obj)
{
    int h;
    jfieldID jfid;

    jfid=(*env)->GetFieldID(env, (*env)->GetObjectClass(env, obj), "globusRcHandlePointer", "I");
    h=(*env)->GetIntField(env, obj, jfid);

    return (globus_replica_catalog_collection_handle_t *)h;
}

JNIEXPORT jint JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_open(JNIEnv *env, jobject obj, jstring url, jstring name, jstring password)
{
    globus_replica_catalog_collection_handleattr_t  rcHandleAttrs;
    globus_replica_catalog_collection_handle_t      *rcHandle;

    char *curl;
    char *cname;
    char *cpassword;

    // FIXME: get this out of here
    if (globus_module_activate(GLOBUS_REPLICA_CATALOG_MODULE)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error activating RC module");
	return 0;
    }

    curl=jstringToCstring(env, url);
    cname=jstringToCstring(env, name);
    cpassword=jstringToCstring(env, password);
    if ((!curl)||(!cname)||(!cpassword))
    {
	return 0;
    }

    if (globus_replica_catalog_collection_handleattr_init( &rcHandleAttrs )!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error initialising handle attributes");
	free(curl);
	free(cname);
	free(cpassword);
	return 0;
    }

    if (globus_replica_catalog_collection_handleattr_set_authentication_mode(&rcHandleAttrs,
	GLOBUS_REPLICA_CATALOG_AUTHENTICATION_MODE_CLEARTEXT, cname, cpassword )!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting authentication mode for replica catalogue to clear text");
	free(curl);
	free(cname);
	free(cpassword);
	return 0;
    }

    rcHandle=malloc(sizeof(globus_replica_catalog_collection_handle_t));
    if (!rcHandle)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory allocating RC handle");
	free(curl);
	free(cname);
	free(cpassword);
	return 0;	
    }

    if (globus_replica_catalog_collection_open( rcHandle, &rcHandleAttrs, curl)!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error opening replica catalogue");
	free(curl);
	free(cname);
	free(cpassword);
	free(rcHandle);
	return 0;
    }

    free(curl);
    free(cname);
    free(cpassword);
    return (int)rcHandle;
}

JNIEXPORT jint JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_openReadOnly(JNIEnv *env, jobject obj, jstring url)
{
    globus_replica_catalog_collection_handleattr_t  rcHandleAttrs;
    globus_replica_catalog_collection_handle_t      *rcHandle;

    char *curl;

    // FIXME: get this out of here
    if (globus_module_activate(GLOBUS_REPLICA_CATALOG_MODULE)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error activating RC module");
	return 0;
    }

    curl=jstringToCstring(env, url);
    if (!curl)
    {
	return 0;
    }

    if (globus_replica_catalog_collection_handleattr_init( &rcHandleAttrs )!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error initialising handle attributes");
	free(curl);
	return 0;
    }

    if (globus_replica_catalog_collection_handleattr_set_authentication_mode(&rcHandleAttrs,
	GLOBUS_REPLICA_CATALOG_AUTHENTICATION_MODE_CLEARTEXT, NULL, NULL )!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error setting authentication mode for replica catalogue to clear text");
	free(curl);
	return 0;
    }

    rcHandle=malloc(sizeof(globus_replica_catalog_collection_handle_t));
    if (!rcHandle)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory allocating RC handle");
	free(curl);
	return 0;	
    }

    if (globus_replica_catalog_collection_open( rcHandle, &rcHandleAttrs, curl)!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error opening replica catalogue");
	free(curl);
	free(rcHandle);
	return 0;
    }

    free(curl);
    return (int)rcHandle;
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_close(JNIEnv *env, jobject obj)
{
    globus_replica_catalog_collection_close(getHandle(env, obj));
}

JNIEXPORT jobjectArray JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_listFiles(JNIEnv *env, jobject obj)
{
    jobjectArray fileList;
    globus_replica_catalog_collection_handle_t *handle;
    char **filenames;
    int numFiles;
    int i;

    handle=getHandle(env, obj);

    if (globus_replica_catalog_collection_list_filenames(handle, &filenames)
	!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error listing files in RC");
	return NULL;
    }

    if (!filenames)
    {
	numFiles=0;
    }
    else
    {
	/*
	 * Count the files
	 */
	numFiles=0;
	while (filenames[numFiles]) 
	{
	    numFiles++;
	}
    }

    fileList=(*env)->NewObjectArray(env, numFiles, (*env)->FindClass(env, "java/lang/String"), NULL);

    for (i=0; i<numFiles; i++)
    {
	(*env)->SetObjectArrayElement(env, fileList, i, (*env)->NewStringUTF(env, filenames[i]));
    }

    if (filenames)
    {
	ldap_value_free(filenames);
    }

    return fileList;
}

JNIEXPORT jobjectArray JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_listLocationFiles(JNIEnv *env, jobject obj, jstring node)
{
    jobjectArray fileList;
    globus_replica_catalog_collection_handle_t *handle;
    char **filenames;
    int numFiles;
    int i;
    char *cnode;

    cnode=jstringToCstring(env, node);
    if (cnode==NULL)
    {
	return NULL;
    }

    handle=getHandle(env, obj);

    if (globus_replica_catalog_location_list_filenames(handle, cnode, &filenames)
	!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error listing files in RC location");
	free(cnode);
	return NULL;
    }
    free(cnode);

    if (!filenames)
    {
	numFiles=0;
    }
    else
    {
	/*
	 * Count the files
	 */
	numFiles=0;
	while (filenames[numFiles]) 
	{
	    numFiles++;
	}
    }

    fileList=(*env)->NewObjectArray(env, numFiles, (*env)->FindClass(env, "java/lang/String"), NULL);

    for (i=0; i<numFiles; i++)
    {
	(*env)->SetObjectArrayElement(env, fileList, i, (*env)->NewStringUTF(env, filenames[i]));
    }

    if (filenames)
    {
	ldap_value_free(filenames);
    }

    return fileList;    
}

JNIEXPORT jobjectArray JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_listFileLocations(JNIEnv *env, jobject obj, jstring lfn)
{
    char *filenames[2];
    struct globus_replica_catalog_entry_set_s locations;
    char *name;
    int i;
    globus_replica_catalog_collection_handle_t *handle;
    int numLocations;
    jobjectArray locList;

    filenames[0]=jstringToCstring(env, lfn);
    filenames[1]=NULL;
    if (!filenames[0])
    {
	return NULL;
    }

    handle=getHandle(env, obj);

    if (globus_replica_catalog_entry_set_init(&locations)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error initialising entry set");
	free(filenames[0]);
	return NULL;
    }

    if (globus_replica_catalog_collection_find_locations(handle, 
							 filenames,
							 GLOBUS_FALSE, NULL,
							 &locations)
	!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error finding file locations");
	free(filenames[0]);
	return NULL;
    }
    free(filenames[0]);
		
    globus_replica_catalog_entry_set_first(&locations);

    if (globus_replica_catalog_entry_set_count(&locations, &numLocations)!=
	GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error counting entries");
	globus_replica_catalog_entry_set_destroy(&locations);
	return NULL;
    }

    locList=(*env)->NewObjectArray(env, numLocations, (*env)->FindClass(env, "java/lang/String"), NULL);

    i=0;
    while (globus_replica_catalog_entry_set_more(&locations)==GLOBUS_TRUE) 
    {
    
	globus_replica_catalog_entry_set_get_name(&locations, &name);

	/* Don't return a location on a node that's inaccessible
	if ((!isNodeDead(name))&&(!isNodeDisabled(name))) 
	{
	*/
	(*env)->SetObjectArrayElement(env, locList, i, (*env)->NewStringUTF(env, name));
	i++;
	/*
	}
	*/

	globus_libc_free(name);
	globus_replica_catalog_entry_set_next(&locations);
    }

    globus_replica_catalog_entry_set_destroy(&locations);

    return locList;
}

JNIEXPORT jboolean JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_locationExists(JNIEnv *env, jobject obj, jstring location)
{
    struct globus_replica_catalog_entry_set_s locations;
    char *clocation;
    char *name;
    globus_replica_catalog_collection_handle_t *handle;

    handle=getHandle(env, obj);

    clocation=jstringToCstring(env, location);
    if (!clocation)
    {
	return JNI_FALSE;
    }

    if (globus_replica_catalog_entry_set_init(&locations)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error initialising entry set");
	free(clocation);
	return JNI_FALSE;
    }

    if (globus_replica_catalog_collection_list_locations(handle, NULL, 
							 &locations)!=
	GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error listing locations in RC");
	free(clocation);
	return JNI_FALSE;
    }

    globus_replica_catalog_entry_set_first(&locations);

    while (globus_replica_catalog_entry_set_more(&locations)==GLOBUS_TRUE) 
    {
    
	globus_replica_catalog_entry_set_get_name(&locations, &name);

	if (!strcmp(clocation, name)) 
	{
	    globus_replica_catalog_entry_set_destroy(&locations);
	    globus_libc_free(name);
	    free(clocation);
	    return JNI_TRUE;
	}
	globus_libc_free(name);
	
	globus_replica_catalog_entry_set_next(&locations);
    }

    globus_replica_catalog_entry_set_destroy(&locations);
    free(clocation);
    return JNI_FALSE;    
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_addLocation(JNIEnv *env, jobject obj, jstring location)
{
    globus_replica_catalog_collection_handle_t *handle;
    char *node;

    node=jstringToCstring(env, location);
    if (!node)
    {
	return;
    }

    if (globus_replica_catalog_location_create(handle, node, node, NULL, 
					       NULL)!=GLOBUS_SUCCESS)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error creating new RC");
    }
    free(node);
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_removeLocation(JNIEnv *env, jobject obj, jstring location)
{
    globus_replica_catalog_collection_handle_t *handle;
    char *clocation;

    clocation=jstringToCstring(env, location);
    if (!clocation)
    {
	return;
    }

    handle=getHandle(env, obj);
    if (globus_replica_catalog_location_delete(handle, clocation)
	!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error deleting location from RC");
    }
    free(clocation);
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_addFileToCollection(JNIEnv *env, jobject obj, jstring lfn)
{
    char *filenames[2];
    globus_replica_catalog_collection_handle_t *handle;

    filenames[0]=jstringToCstring(env, lfn);
    filenames[1]=NULL;
    if (!filenames[0])
    {
	return;
    }

    handle=getHandle(env, obj);

    if (globus_replica_catalog_collection_add_filenames(handle, 
							filenames,
							GLOBUS_FALSE)
	!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error adding file to RC collection");
    }

    free(filenames[0]);
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_addFileToLocation(JNIEnv *env, jobject obj, jstring location, jstring lfn)
{
    char *filenames[2];
    globus_replica_catalog_collection_handle_t *handle;
    char *node;

    node=jstringToCstring(env, location);
    filenames[0]=jstringToCstring(env, lfn);
    filenames[1]=NULL;
    if ((!filenames[0])||(!node))
    {
	return;
    }

    handle=getHandle(env, obj);

    /* Now add to actual location */
    if (globus_replica_catalog_location_add_filenames(handle, node,
						      filenames, 
						      GLOBUS_FALSE)
	!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error adding file to location");
    }

    free(node);
    free(filenames[0]);
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_removeFileFromCollection(JNIEnv *env, jobject obj, jstring lfn)
{
    char *filenames[2];
    globus_replica_catalog_collection_handle_t *handle;

    filenames[0]=jstringToCstring(env, lfn);
    filenames[1]=NULL;
    if (!filenames[0])
    {
	return;
    }

    handle=getHandle(env, obj);

    if (globus_replica_catalog_collection_delete_filenames(handle, 
							   filenames)
	!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error deleting file from collection");
    }

    free(filenames[0]);
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridReplicaCatalogue_removeFileFromLocation(JNIEnv *env, jobject obj, jstring location, jstring lfn)
{
    char *filenames[2];
    globus_replica_catalog_collection_handle_t *handle;
    char *node;

    node=jstringToCstring(env, location);
    filenames[0]=jstringToCstring(env, lfn);
    filenames[1]=NULL;
    if ((!filenames[0])||(!node))
    {
	return;
    }

    handle=getHandle(env, obj);

    /* Now add to actual location */
    if (globus_replica_catalog_location_delete_filenames(handle, node,
							 filenames)
	!=GLOBUS_SUCCESS) 
    {
	THROW_QCDGRID_EXCEPTION(env, "Error removing file from location");
    }

    free(node);
    free(filenames[0]);
}


