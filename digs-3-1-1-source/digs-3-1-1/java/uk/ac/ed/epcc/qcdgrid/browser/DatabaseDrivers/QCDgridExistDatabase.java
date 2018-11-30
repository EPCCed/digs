/*
 * Copyright (c) 2002, 2005 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

import org.apache.log4j.Logger;

import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MetadataDocumentRetriever;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QueryExecutionException;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.UserQueryRunner;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

import java.util.Vector;

/**
 * Title: QCDgridExistDatabase Description: A backend for interfacing with the
 * Exist XML database.
 * 
 * September 2005: Refactored and added some convenience methods for creating
 * new Collection objects.
 * 
 * June 2006: Renamed to QCDgridExistDatabase. This reflects the fact that this
 * class serves two purposes - running queries, and allowing access to application
 * specific information (e.g. query types, associated schemas etc).
 * 
 * Copyright: Copyright (c) 2003, 2005, 2006 Company: The University of Edinburgh
 * 
 * @author James Perry, Daragh Byrne
 * @version 1.2
 */
public class QCDgridExistDatabase extends DatabaseDriver 
  implements UserQueryRunner, MetadataDocumentRetriever {

  private static Logger logger_ = Logger.getLogger(QCDgridExistDatabase.class);

  /** Reference to the collection being manipulated */
  private Collection col;

  /** XPath query service of this database */
  private XPathQueryService service;

  /**
   * A bit of a hack: the QCDgrid result handler needs access to the Exist
   * driver and this was the only way I could think of doing it at the time
   */
  private static QCDgridExistDatabase driver_;

  private String scope;

  private String url;

  private String dbUser_ = "admin"; 
  private String dbPassword_ = "";

  /**
   * Namespace strings for the query types
   */
  private String ensembleNamespace = "http://www.lqcd.org/ildg/QCDml/ensemble1.4";
  private String configurationNamespace = "http://www.lqcd.org/ildg/QCDml/config1.3";
  private String correlatorNamespace = "";
  
  /**
   * Opens a connection to the database.
   * 
   * @param server
   *          hostname/port of database in form localhost:8080
   */
  public void init(String server, String username, String password) throws XMLDBException {
    try {
      dbUser_ = username;
      dbPassword_ = password;
      this.url = "xmldb:exist://" + server + "/exist/xmlrpc/db/";
      logger_.debug("Initialising database server at: " + this.url);
      col = newCollection();
      service = (XPathQueryService) col.getService("XPathQueryService", "1.0");
      driver_ = this;
      scope = "";
      if (service == null) {
        logger_.debug("Service null in ctor.");
      }
    } catch (ClassNotFoundException cnfe) {
      System.out
          .println("QCDgridExistDatabase: Could not find org.exist.xmldb.DatabaseImpl");
      logger_
          .debug("QCDgridExistDatabase: Could not find org.exist.xmldb.DatabaseImpl");
    } catch (InstantiationException ie) {
      System.out
          .println("QCDgridExistDatabase: Could not instantiate org.exist.xmldb.DatabaseImpl");
      System.out.println(ie.getStackTrace());
      logger_
          .debug("QCDgridExistDatabase: Could not instantiate org.exist.xmldb.DatabaseImpl");
    } catch (IllegalAccessException iae) {
      System.out.println("QCDgridExistDatabase: IllegalAccessException");
      System.out.println(iae.getStackTrace());
      logger_.debug("QCDgridExistDatabase: IllegalAccessException"
          + iae.getMessage());
    }
  }

  /**
   * Sets the database to "see" a restricted scope. This is done using the XMLDB
   * collections mechanism
   * 
   * @param sc
   *          string used as collection name. All non-alphanumeric characters
   *          are substituted with hyphens to make the actual name
   */
  public void setScope(String sc) throws Exception {

    /* substitute non-alphanumeric characters */
    StringBuffer sc2 = new StringBuffer(sc);
    for (int i = 0; i < sc2.length(); i++) {
      if (!Character.isLetterOrDigit(sc2.charAt(i))) {
        sc2.setCharAt(i, '-');
      }
    }
    String sc3 = sc2.toString();

    if (!sc3.equals("")) {
      sc3 = "/" + sc3;
    }

    col = newCollection(sc);
    service = (XPathQueryService) col.getService("XPathQueryService", "1.0");

    /* remember new scope if set successfully */
    scope = sc;
  }

  /**
   * @return the current database scope
   */
  public String getScope() {
    return scope;
  }

  /**
   * @return the exist database driver. Needed by the QCDgrid result handler
   */
  public static QCDgridExistDatabase getDriver() {
    return driver_;
  }

  /**
   * Gets the content of a document from the database
   * 
   * @param id
   *          the document identifier (usually its name)
   * @return the content of the document
   */
  public String getDocument(String id) throws QCDgridException {
    String retval = null;
    try {
      retval = getDocumentImpl(id);
    } catch (Exception e) {
      throw new QCDgridException(e.getMessage());
    }

   // System.out.println("Got document: " + retval);
    return retval;
    
  }


  
  /**
   * Runs an XPath query that is expected to return configuration related
   * metadata.
   * @param XPathQuery
   * @return
   */
  public QCDgridQueryResults runConfigurationQuery(String XPathQuery) throws QueryExecutionException{
    QCDgridQueryResults res;
    try{
      setNamespace(configurationNamespace);
      res = runQuery(XPathQuery, QueryType.CONFIGURATION_QUERY_TYPE);
    }
    catch(XMLDBException xdbe){
      throw new QueryExecutionException("Your query could not be executed for the following" +
          " reason: "
          +xdbe.getMessage());
    }
    return res;
  }
  
  /**
   * Runs an XPath query that is expected to return ensemble related
   * metadata.
   * @param XPathQuery
   * @return
   */
  public QCDgridQueryResults runEnsembleQuery(String XPathQuery) throws QueryExecutionException{
    QCDgridQueryResults res;
    try{
      setNamespace(ensembleNamespace);
      res = runQuery(XPathQuery, QueryType.ENSEMBLE_QUERY_TYPE);
    }
    catch(XMLDBException xdbe){
      throw new QueryExecutionException("Your query could not be executed for the following" +
          " reason: "
          +xdbe.getMessage());
    }
    return res;
  }
  
  /**
   * Runs an XPath query that is expected to return correlator related
   * metadata.
   * @param XPathQuery
   * @return
   */
  public QCDgridQueryResults runCorrelatorQuery(String XPathQuery) throws QueryExecutionException{
    QCDgridQueryResults res;
    try{
      setNamespace(correlatorNamespace);
      res = runQuery(XPathQuery, QueryType.CORRELATOR_QUERY_TYPE);
    }
    catch(XMLDBException xdbe){
      throw new QueryExecutionException("Your query could not be executed for the following" +
          " reason: "
          +xdbe.getMessage());
    }
    return res;
  }
  
  /**
   * Runs an XPath query on the database
   * 
   * @param query
   *          the query to execute
   * @return the results of the query
   */
  public QCDgridQueryResults runQuery(String query, String queryTypeInfo) throws XMLDBException {
    return runQueryImpl(query, queryTypeInfo);
  }

  public QCDgridQueryResults runQuery(String query) throws XMLDBException{
    return runQueryImpl(query, QueryType.QUERY_TYPE_UNKNOWN);
  }
  
  /**
   * Allows this to be mockable.
   * 
   * @param query
   *          the query to execute
   * @return query results.
   * @throws XMLDBException
   */
  protected QCDgridQueryResults runQueryImpl(String query, String queryTypeInfo) throws XMLDBException {
    // System.out.println("QCDgridExistDatabase::runQueryImpl: " + query );
    if(service==null)logger_.debug("SERVICENULL");
    ResourceSet resultSet = service.query(query);
    return new QCDgridQueryResults(resultSet, queryTypeInfo);
  }

 
  
  public String getConfigurationMetadataDocument(String id, String hostInfo) throws QCDgridException{
    return getDocument(id);
  }
  
  public String getEnsembleMetadataDocument(String id, String hostInfo) throws QCDgridException{
    return getDocument(id);
  }
  
  public String getCorrelatorMetadataDocument(String id, String hostInfo) throws QCDgridException{
    return getDocument(id);
  }


  public void setNamespace(String ns) throws XMLDBException {
    service.setNamespace("", ns);
  }

  public void setConfigurationNamespace(String ns) {
    configurationNamespace = ns;
  }

  public void setEnsembleNamespace(String ns) {
    ensembleNamespace = ns;
  }

  public void setCorrelatorNamespace(String ns) {
    correlatorNamespace = ns;
  }
  
  /**
   * @return the schema document
   */
  public String getSchema(String name) throws XMLDBException {
    Resource r = col.getResource(name);
    if (r == null) {
      return null;
    }
    return (String) r.getContent();
  }

  public String getSchema() throws Exception {
    return getSchema("schema.xml");
  }

  /**
   * @return a vector of supported query notations - only XPath just now
   */
  public Vector getQueryNotations() throws Exception {
    Vector v = new Vector();
    v.add(new String("http://www.w3.org/TR/1999/REC-xpath-19991116"));
    return v;
  }


  /**
   * Allows access to the collection associated with this driver.
   */
  public Collection getCollection() {
    return col;
  }

  /**
   * Allows access to the Query Service associated with this driver.
   */
  public XPathQueryService getXPathQueryService() {
    return service;
  }
  
  
  /**
   * Returns a new Collection object
   */
  protected Collection newCollection() throws ClassNotFoundException,
      IllegalAccessException, InstantiationException, XMLDBException {
    return newCollection(null);
  }

  /**
   * Returns a scoped collection object
   */
  protected Collection newCollection(String additionalScope)
      throws ClassNotFoundException, IllegalAccessException,
      InstantiationException, XMLDBException {
    String dr = "org.exist.xmldb.DatabaseImpl";
    Class c = Class.forName(dr);
    Database database = (Database) c.newInstance();
    DatabaseManager.registerDatabase(database);
    String connStr = this.url;
    if (additionalScope != null) {
      connStr += additionalScope;
    }
    return createCollection(connStr);

  }

  /**
   * Returns a new collection
   * 
   * @param connStr
   *          A connection string of the form
   *          xmldb:exist//localhost:port/exist/xmlrmb/db/scope
   */

  protected Collection createCollection(String connStr) throws XMLDBException {
    // System.out.println( "Creating XMLDB: " + connStr );
    return DatabaseManager.getCollection(connStr, dbUser_, dbPassword_);
  }
  
  
  protected String getDocumentImpl(String id) throws Exception {
    Resource r = col.getResource(id);
    if (r == null) {
      //logger_.debug("Resource is null");
      return null;
    }
    return (String) r.getContent();
  }
  
  
}
