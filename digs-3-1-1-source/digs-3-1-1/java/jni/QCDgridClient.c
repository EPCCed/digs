/***********************************************************************
*
*   Filename:   QCDgridClient.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    To interface the QCDgrid Java client API with the C
*               functions beneath it
*
*   Contents:   Implementations of QCDgridClient class's native methods
*
*   Used in:    QCDgrid Java API
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

#include "qcdgrid-client.h"

#include "uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient.h"

/*
 * Macro for easily throwing a QCDgridException
 */
#define THROW_QCDGRID_EXCEPTION(env, str) (*env)->ThrowNew(env, (*env)->FindClass(env, "uk/ac/ed/epcc/qcdgrid/QCDgridException"), str);

/*
 * Utility function for converting a Java string object to a C string (char array)
 * Returned pointer points to malloced storage and should be freed by the caller
 */
char *jstringToCstring(JNIEnv *env, jstring str)
{
    int len;
    char *cstr;

    len=(*env)->GetStringUTFLength(env, str);

    cstr=malloc(len+1);
    if (!cstr)
    {
	return NULL;
    }

    (*env)->GetStringUTFRegion(env, str, 0, len, cstr);
    cstr[len] = 0;
    return cstr;
}

/*
 * Init and shutdown functions; private and called only by the QCDgridClient's constructor
 * and finalize method respectively
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_init(JNIEnv *env, jobject obj, jboolean secondaryOK)
{
    if (qcdgridInit(secondaryOK)==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error loading grid config");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_shutdown(JNIEnv *env, jobject obj)
{
    qcdgridShutdown();
}

/*
 * Node manipulation functions
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_addNode(JNIEnv *env, jobject obj, jstring node, jstring site, jstring path)
{
    char *cnode, *csite, *cpath;
    int result;

    cnode=jstringToCstring(env, node);
    csite=jstringToCstring(env, site);
    cpath=jstringToCstring(env, path);

    if ((!cnode)||(!csite)||(!cpath))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridAddNode(cnode, csite, cpath);
    free(cnode);
    free(csite);
    free(cpath);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error adding node to grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_removeNode(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode;
    int result;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridRemoveNode(cnode);
    free(cnode);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error removing node from grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_disableNode(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode;
    int result;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridDisableNode(cnode);
    free(cnode);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error disabling node");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_enableNode(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode;
    int result;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridEnableNode(cnode);
    free(cnode);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error enabling node");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_retireNode(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode;
    int result;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridRetireNode(cnode);
    free(cnode);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error retiring node");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_unretireNode(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode;
    int result;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridUnretireNode(cnode);
    free(cnode);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error Unretiring node");
    }
}

/*
 * File getting functions
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getFile(JNIEnv *env, jobject obj, jstring lfn, jstring pfn)
{
    char *clfn, *cpfn;
    int result;

    clfn=jstringToCstring(env, lfn);
    cpfn=jstringToCstring(env, pfn);
    if ((!clfn)||(!cpfn))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridGetFile(clfn, cpfn);
    free(clfn);
    free(cpfn);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error getting file from grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getDirectory(JNIEnv *env, jobject obj, jstring lfn, jstring pfn)
{
    char *clfn, *cpfn;
    int result;

    clfn=jstringToCstring(env, lfn);
    cpfn=jstringToCstring(env, pfn);
    if ((!clfn)||(!cpfn))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridGetDirectory(clfn, cpfn);
    free(clfn);
    free(cpfn);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error getting directory from grid");
    }
}

/*
 * Putting functions
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_putFile(JNIEnv *env, jobject obj, jstring pfn, jstring lfn, jstring permissions)
{
    char *clfn, *cpfn, *cpermissions;
    int result;

    clfn=jstringToCstring(env, lfn);
    cpfn=jstringToCstring(env, pfn);
    cpermissions=jstringToCstring(env, permissions);
    if ((!clfn)||(!cpfn)||(!cpermissions))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridPutFile(cpfn, clfn, cpermissions);
    free(clfn);
    free(cpfn);
    free(cpermissions);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error putting file on grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_putDirectory(JNIEnv *env, jobject obj, jstring pfn, jstring lfn, jstring permissions)
{
    char *clfn, *cpfn, *cpermissions;
    int result;

    clfn=jstringToCstring(env, lfn);
    cpfn=jstringToCstring(env, pfn);
    cpermissions=jstringToCstring(env, permissions);

    if ((!clfn)||(!cpfn))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridPutDirectory(cpfn, clfn, cpermissions);
    free(clfn);
    free(cpfn);
    free(cpermissions);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error putting directory on grid");
    }
}

/*
 * List function: returns an array of Strings, which are the grid logical filenames
 */
JNIEXPORT jobjectArray JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_list(JNIEnv *env, jobject obj)
{
    char **fileList;
    jclass stringClass;
    jobjectArray jFileList;
    jstring str;
    int numFiles;
    int i;

    /*
     * Get the file list from C API
     */
    fileList=qcdgridList();

    if (fileList==NULL)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error listing files on grid");
	return NULL;
    }

    /*
     * Count the files in it
     */
    numFiles=0;
    while(fileList[numFiles]!=NULL)
    {
	numFiles++;
    }
    
    /*
     * Create a Java array corresponding to file list
     */
    stringClass=(*env)->FindClass(env, "java/lang/String");

    str=(*env)->NewStringUTF(env, "default");

    jFileList=(*env)->NewObjectArray(env, numFiles, stringClass, str);

    /*
     * Add each filename to it in turn
     */
    for (i=0; i<numFiles; i++)
    {
	str=(*env)->NewStringUTF(env, fileList[i]);
	(*env)->SetObjectArrayElement(env, jFileList, i, str);
    }

    qcdgridDestroyList(fileList);
    return jFileList;
}

/*
 * Node functions
 */
int getNumNodes();
char *getNodeName(int i);
char *getNodeSite(char *node);

JNIEXPORT jobjectArray JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_listNodes(JNIEnv *env, jobject obj)
{
    int numNodes = getNumNodes();
    jobjectArray jNodeList;
    int i;
    jclass stringClass;
    jstring str;

    /*
     * Create a Java array corresponding to node list
     */
    stringClass=(*env)->FindClass(env, "java/lang/String");

    str=(*env)->NewStringUTF(env, "default");

    jNodeList=(*env)->NewObjectArray(env, numNodes, stringClass, str);

    /*
     * Add each filename to it in turn
     */
    for (i=0; i<numNodes; i++)
    {
	str=(*env)->NewStringUTF(env, getNodeName(i));
	(*env)->SetObjectArrayElement(env, jNodeList, i, str);
    }

    return jNodeList;
}

JNIEXPORT jstring JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getNodeSite(JNIEnv *env, jobject obj, jstring node)
{
    char *csite;
    jstring str;
    char *cnode;

    cnode=jstringToCstring(env, node);

    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    csite = getNodeSite(cnode);

    free(cnode);

    str=(*env)->NewStringUTF(env, csite);
    return str;
}

char *getMainNodeName();
char *getMetadataCatalogueLocation();

JNIEXPORT jstring JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getMainNodeName(JNIEnv *env, jobject obj)
{
    char *mainnode;
    jstring str;

    mainnode=getMainNodeName();
    str=(*env)->NewStringUTF(env, mainnode);
    return str;
}

JNIEXPORT jstring JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getMetadataCatalogueLocation(JNIEnv *env, jobject obj)
{
    char *mdc;
    jstring str;

    mdc=getMetadataCatalogueLocation();
    str=(*env)->NewStringUTF(env, mdc);
    return str;
}


int isNodeDisabled(char *node);
int isNodeDead(char *node);
int isNodeRetiring(char *node);

JNIEXPORT jint JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getNodeStatus(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode;
    int nodeStatus=0;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return -1;
    }

    if (isNodeDisabled(cnode))
    {
	nodeStatus=2;
    }
    else if (isNodeDead(cnode))
    {
	nodeStatus=1;
    }
    else if (isNodeRetiring(cnode))
    {
	nodeStatus=3;
    }

    return nodeStatus;
}

char *getFirstConfigValue(char *id, char *name);
char *getNextConfigValue(char *id, char *name);

JNIEXPORT jint JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_getNodeType(JNIEnv *env, jobject obj, jstring node)
{
    char *cnode, *tmp;
    int nodeType=0;

    cnode=jstringToCstring(env, node);
    if (!cnode)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return -1;
    }

    tmp = getFirstConfigValue("mainnodeinfo", "node");
    if (!strcmp(tmp, cnode))
    {
	nodeType=1;
    }
    else
    {
	tmp=getFirstConfigValue("mainnodeinfo", "backup_node");
	while (tmp)
	{
	    if (!strcmp(tmp, cnode))
	    {
		nodeType=2;
	    }

	    tmp=getNextConfigValue("mainnodeinfo", "backup_node");
	}
    }

    return nodeType;
}


/*
 * Delete functions
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_deleteFile(JNIEnv *env, jobject obj, jstring lfn)
{
    char *clfn;
    int result;

    clfn=jstringToCstring(env, lfn);
    if (!clfn)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridDeleteFile(clfn);
    free(clfn);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error deleting file from grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_deleteDirectory(JNIEnv *env, jobject obj, jstring lfn)
{
    char *clfn;
    int result;

    clfn=jstringToCstring(env, lfn);
    if (!clfn)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridDeleteDirectory(clfn);
    free(clfn);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error deleting directory from grid");
    }
}

/*
 * Touch functions
 */
JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_touchFile__Ljava_lang_String_2(JNIEnv *env, jobject obj, jstring lfn)
{
    char *clfn;
    int result;

    clfn=jstringToCstring(env, lfn);
    if (!clfn)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridTouchFile(clfn, NULL);
    free(clfn);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error touching file on grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_touchDirectory__Ljava_lang_String_2(JNIEnv *env, jobject obj, jstring lfn)
{
    char *clfn;
    int result;

    clfn=jstringToCstring(env, lfn);
    if (!clfn)
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridTouchDirectory(clfn, NULL);
    free(clfn);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error touching directory on grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_touchFile__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv *env, jobject obj, jstring lfn, jstring dest)
{
    char *clfn, *cdest;
    int result;

    clfn=jstringToCstring(env, lfn);
    cdest=jstringToCstring(env, lfn);
    if ((!clfn)||(!cdest))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridTouchFile(clfn, cdest);
    free(clfn);
    free(cdest);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error touching file on grid");
    }
}

JNIEXPORT void JNICALL Java_uk_ac_ed_epcc_qcdgrid_datagrid_QCDgridClient_touchDirectory__Ljava_lang_String_2Ljava_lang_String_2(JNIEnv *env, jobject obj, jstring lfn, jstring dest)
{
    char *clfn, *cdest;
    int result;

    clfn=jstringToCstring(env, lfn);
    cdest=jstringToCstring(env, lfn);
    if ((!clfn)||(!cdest))
    {
	THROW_QCDGRID_EXCEPTION(env, "Out of memory");
	return;
    }

    result=qcdgridTouchDirectory(clfn, cdest);
    free(clfn);
    free(cdest);

    if (result==0)
    {
	THROW_QCDGRID_EXCEPTION(env, "Error touching directory on grid");
    }
}
