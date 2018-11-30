/***********************************************************************
 *
 *   Filename:   ExistDownloadClient.java
 *
 *   Authors:    Daragh Byrne     (daragh)   EPCC.
 *
 *   Purpose:    Allows all database files to be downloaded from the root 
 *               collection of an exist database server.
 *
 *   Contents:   ExistDownloadClient class
 *
 *   Used in:    Utility
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2006 The University of Edinburgh
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

package uk.ac.ed.epcc.qcdgrid.metadata.catalogue.util;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

import java.io.*;

public class ExistDownloadClient {

  private XPathQueryService service_;
  private Collection col_;
  private String databaseUrl_;

  /**
   * Usage: java ExistDownloadClient &lt;databaseServer&gt; &lt;port&gt;
   * @param args
   */
  public static void main(String[] args) {
    if (args.length < 2) {
      printUsage();
      System.exit(0);
    }
    String server = args[0];
    int port = Integer.parseInt(args[1]);
    try {
      ExistDownloadClient cl = new ExistDownloadClient(server, port);
      cl.downloadAllData();
    } catch (Exception e) {
      System.out.println("A problem occurred:" + e.getMessage());
    }
  }

  public static void printUsage() {
    System.out
        .println("Usage: java ExistDownloadClient <databaseServer> <port>");
  }

  /**
   * Grabs the root collection, enumerates the contents and downloads each
   * resources. Resources are stored in files that are named after the XMLDB
   * resourceId.
   * @throws XMLDBException
   * @throws IOException
   */
  public void downloadAllData() throws XMLDBException, IOException {
    String[] resources = col_.listResources();
    for (int i = 0; i < resources.length; i++) {
      String resourceId = resources[i];
      System.out.println("Downloading: [" + i + "] " + resourceId);
      Resource r = col_.getResource(resourceId);
      String res = (String) r.getContent();
      FileWriter fw = new FileWriter(resourceId);
      fw.write(res);
      fw.close();
    }
  }

  
  /**
   * Initialises the XMLDB objects that are used to download the resources.
   * @param server
   * @param port
   * @throws XMLDBException
   */
  public ExistDownloadClient(String server, int port) throws XMLDBException {

    try {
      databaseUrl_ = "xmldb:exist://" + server + ":" + port + "/exist/xmlrpc/db/";
      col_ = newCollection();
      service_ = (XPathQueryService) col_
          .getService("XPathQueryService", "1.0");
      System.out.println("Connecting to: " + databaseUrl_);
      if (service_ == null) {
        System.out.println("Service null in ctor.");
      }
    } catch (ClassNotFoundException cnfe) {
      System.out
          .println("QCDgridExistDatabase: Could not find org.exist.xmldb.DatabaseImpl");
    } catch (InstantiationException ie) {
      System.out
          .println("QCDgridExistDatabase: Could not instantiate org.exist.xmldb.DatabaseImpl");
      System.out.println(ie.getStackTrace());
    } catch (IllegalAccessException iae) {
      System.out.println("QCDgridExistDatabase: IllegalAccessException");
      System.out.println(iae.getStackTrace());
    }

  }

  
  /**
   * 
   * @return
   * @throws ClassNotFoundException
   * @throws IllegalAccessException
   * @throws InstantiationException
   * @throws XMLDBException
   */
  private Collection newCollection() throws ClassNotFoundException,
      IllegalAccessException, InstantiationException, XMLDBException {
    String dr = "org.exist.xmldb.DatabaseImpl";
    Class c = Class.forName(dr);
    Database database = (Database) c.newInstance();
    DatabaseManager.registerDatabase(database);
    String connStr = this.databaseUrl_;
    return createCollection(connStr);

  }

  /**
   * Returns a new collection
   * 
   * @param connStr
   *          A connection string of the form
   *          xmldb:exist//localhost:port/exist/xmlrmb/db/scope
   */

  private Collection createCollection(String connStr) throws XMLDBException {
    // System.out.println( "Creating XMLDB: " + connStr );
    return DatabaseManager.getCollection(connStr);
  }

}
