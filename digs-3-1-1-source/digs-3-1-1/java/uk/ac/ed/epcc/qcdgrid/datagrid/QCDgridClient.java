/***********************************************************************
*
*   Filename:   QCDgridClient.java
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Provides a Java API for QCDgrid client functionality
*
*   Contents:   QCDgridClient class, which provides methods for
*               accessing all QCDgrid client operations
*
*   Used in:    Java client tools
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
package uk.ac.ed.epcc.qcdgrid.datagrid;

import uk.ac.ed.epcc.qcdgrid.*;

public class QCDgridClient {

    /** existing client if there is one, null if not */
    private static QCDgridClient theClient;

    /** whether the client is using a secondary node */
    private static boolean usingSecondary;

    // Constructors - assume secondary node is OK if not told
    // otherwise
    private QCDgridClient() throws QCDgridException {
	this(true);
    }

    private QCDgridClient(boolean secondaryOK) throws QCDgridException {
	init(secondaryOK);
    }

    /**
     *  Because the C library behind this class uses lots of static storage and can
     *  only be instantiated once, we have to use a static instance of this class.
     *  So the constructors are private, and other classes wanting a QCDgridClient
     *  need to use this function to get one
     */
    public static QCDgridClient getClient(boolean secondaryOK) throws QCDgridException {

	// If no client yet, need to create one now
	if (theClient==null) {

	    // First try to use the primary node
	    usingSecondary=false;
	    try {
		theClient=new QCDgridClient(false);
	    }
	    catch (QCDgridException e) {

		// Using primary node failed. Try secondary
		usingSecondary=true;
		try {
		    theClient=new QCDgridClient(true);
		}
		catch (QCDgridException e2) {

		    // Using secondary node failed too. Nothing we can do
		    throw e2;
		}
	    }
	}
	if ((usingSecondary)&&(!secondaryOK)) {
	    throw new QCDgridException("Primary node inaccessible");
	}
	return theClient;
    }

    // Shutdown QCDgrid on destruction of object
    protected void finalize() {
	shutdown();
    }

    // Init and shutdown methods in C
    private native void init(boolean secondaryOK) throws QCDgridException;
    private native void shutdown();

    // Node manipulation methods
    public native void addNode(String node, String site, String path) throws QCDgridException;
    public native void removeNode(String node) throws QCDgridException;
    public native void disableNode(String node) throws QCDgridException;
    public native void enableNode(String node) throws QCDgridException;
    public native void retireNode(String node) throws QCDgridException;
    public native void unretireNode(String node) throws QCDgridException;

    // File retrieving
    public native void getFile(String lfn, String pfn) throws QCDgridException;
    public native void getDirectory(String lfn, String pfn) throws QCDgridException;

    // File storing
    //public native void putFile(String pfn, String lfn) throws QCDgridException;
    //public native void putDirectory(String pfn, String lfn) throws QCDgridException;

    //RADEK
    public native void putFile(String pfn, String lfn, String permissions) throws QCDgridException;
    public native void putDirectory(String pfn, String lfn, String permissions) throws QCDgridException;


    // File listing
    public native String[] list() throws QCDgridException;

    // Node management
    public native String[] listNodes() throws QCDgridException;
    public native String getNodeSite(String node) throws QCDgridException;
    public native String getMainNodeName() throws QCDgridException;
    public native String getMetadataCatalogueLocation() throws QCDgridException;

    // Constants: node states
    // If you change these, change the corresponding numbers in QCDgridClient.c too
    public static final int NODE_OK=0;
    public static final int NODE_DEAD=1;
    public static final int NODE_DISABLED=2;
    public static final int NODE_RETIRING=3;

    public native int getNodeStatus(String node) throws QCDgridException;

    // Constants: node types
    // If you change these, change the corresponding numbers in QCDgridClient.c too
    public static final int NORMAL_NODE=0;
    public static final int PRIMARY_NODE=1;
    public static final int SECONDARY_NODE=2;

    public native int getNodeType(String node) throws QCDgridException;

    // Deleting
    public native void deleteFile(String lfn) throws QCDgridException;
    public native void deleteDirectory(String lfn) throws QCDgridException;

    // Touching
    // The methods without the dest parameter choose a suitable destination
    // node automatically
    public native void touchFile(String lfn) throws QCDgridException;
    public native void touchDirectory(String lfn) throws QCDgridException;
    public native void touchFile(String lfn, String dest) throws QCDgridException;
    public native void touchDirectory(String lfn, String dest) throws QCDgridException;

    // Load the library containing our required native methods
    static {
	System.loadLibrary("QCDgridJavaClient");
	theClient = null;
    }
}
