/***********************************************************************
*
*   Filename:   MdcSoapBindingImpl.java
*
*   Authors:    Daragh Byrne           (daragh)   EPCC.
*
*   Purpose:    The main implementation class for the Metadata Catalogue 
*               web service.
*
*   Content:    MdcSoapBindingImpl class
*
*   Used in:    Java client tools
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
package org.lqcd.ildg.mdc;

import java.util.ArrayList;
import java.util.Date;
import java.text.DateFormat;

import org.xmldb.api.base.XMLDBException;
import org.apache.log4j.Logger;
import org.apache.axis.MessageContext;
import org.lqcd.ildg.mdc.QueryResults;
import org.lqcd.ildg.mdc.QueryResultsParser;
import org.lqcd.ildg.mdc.ResultsParserException;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;


/**
 * This is the main implementation class for the metadata catalogue web service. 
 * It uses QCDgridExistDatabase to issue 
 * queries against the XML database. The raw results of the queries are processed 
 * into the form required to be returned by use of the QueryResultsParser.
 * @author The QCDgrid project team
 */

public class MdcSoapBindingImpl implements org.lqcd.ildg.mdc.MDCInterface {

  // The driver that executes all queries. Read-only at present, it is worth
  // condsidering the threading implications of this at some stage.
  private QCDgridExistDatabase driver_;

  // Types of query allowed, currently just Xpath
  private static ArrayList allowedQueryTypes_ = new ArrayList();
  
  private String databaseLocation_=null;
  
  static{
    allowedQueryTypes_.add("Xpath");
  }
 
  private static Logger logger_ = Logger.getLogger(MdcSoapBindingImpl.class);

  /**
   * Creates a connection to the XML database used.
   */
  public MdcSoapBindingImpl() {
    // TODO make url configurable via service parameters.
    //this.databaseLocation_ = "localhost:8080";
    MessageContext ctx = MessageContext.getCurrentContext();
    if(ctx == null){
      logger_.debug("Null message context.");
    }
    String dbLocation = (String)ctx.getProperty("databaseServer");
    String dbPort     = (String)ctx.getProperty("databasePort");
    String dbUser     = (String)ctx.getProperty("databaseUser");
    String dbPassword = (String)ctx.getProperty("databasePassword");
    
    databaseLocation_ = dbLocation + ":" + dbPort;
    logger_.debug("Using db: " + databaseLocation_ );
    try {
      logger_.debug("Created new MdcSoapBindingImpl");
      driver_ = new QCDgridExistDatabase();
      driver_.init(databaseLocation_, dbUser, dbPassword);
      logger_.debug("Driver created successfully: " + databaseLocation_);
    } catch (XMLDBException xdbe) {
      logger_.error("Error connecting to XMLDB: " + xdbe.getMessage());
    } 
  }


  /**
   * Executes a query against the ensemble documents in the database. Currently the
   * query is executed against ALL documents (config or ensemble) in the database.
   * The URIs associated with any matching ensembles are returned via an instance
   * of QueryResults.
   * @param queryFormat Xpath only just now.
   * @param queryString The query to execute
   * @param startIndex The first result to return (enables scrolling)
   * @param maxResults The maximum number of results to return. If -1, then all results
   * are returned.
   * @return
   * @throws java.rmi.RemoteException
   */
  public org.lqcd.ildg.mdc.QueryResults doEnsembleURIQuery(
      java.lang.String queryFormat, java.lang.String queryString,
      int startIndex, int maxResults) throws java.rmi.RemoteException {
    if(!queryFormatOK(queryFormat)){
      return createQueryInvalidResponse(startIndex, 0);
    }
    logger_.debug("Query issued: [" + queryString + "]");
     // Take the query and execute it against the configurations.
    uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults resultsFromDb = null;
    if(startIndex < 0){
      return createQueryInvalidResponse(startIndex, 0);
    }
    boolean queryFailed = false;    
    try{
      resultsFromDb = runEnsembleXPathQuery(queryString);
    }
    catch(XMLDBException xdbe){
      logger_.debug("XMLDBException when running ensemble lfn query: " + xdbe.getMessage());
      queryFailed = true;
    }
    if(queryFailed == true){
	// RHO: changed createQueryInvalidSyntaxResponse to createQueryInvalidResponse
	//      as for ILDG we need MDC_INVALID_REQUEST and not MDC_INVALID_SYNTAX
      return createQueryInvalidResponse( startIndex, 0 );
    }
    
    //OLD: Check if results have no data, if so respond with MDC_NO_DATA
    //RHO: Replaced createResponseZeroResults with createResponseZeroResultsSuccess
    //     so it complies with ILDG test suite (no results is still a success)
    if(resultsFromDb.count()==0){      
      return createResponseZeroResultsSuccess(startIndex, 0);
    }    
    QueryResultsParser parser = new QueryResultsParser(this.driver_);
    org.lqcd.ildg.mdc.QueryResults resultsToReturn = null; 
    try{
      logger_.debug("Parsing query results.");
      resultsToReturn = parser.prepareRawEnsembleURIResultsForReturn( resultsFromDb, 
          startIndex, 
          maxResults );
    }catch(ResultsParserException re){
      logger_.debug("Query results could not be parsed: " + re.getMessage());
      resultsToReturn = createQueryInvalidResponse(
          startIndex, resultsFromDb.count());
    }
    return resultsToReturn;
  }


  /**
   * Runs a query against the configuration documents. Currently the
   * query is executed against ALL documents (config or ensemble) in the database.
   * The URIs associated with any matching ensembles are returned via an instance
   * of QueryResults.
   * @param queryFormat Xpath only just now.
   * @param queryString The query to execute
   * @param startIndex The first result to return (enables scrolling)
   * @param maxResults The maximum number of results to return. If -1, then all results
   * are returned.
   * @return
   * @throws java.rmi.RemoteException
   */
  public org.lqcd.ildg.mdc.QueryResults doConfigurationLFNQuery(
      java.lang.String queryFormat, java.lang.String queryString,
      int startIndex, int maxResults) throws java.rmi.RemoteException {
    logger_.debug("Query issued:[" + queryFormat + "," + queryString + "]");
    if(!queryFormatOK(queryFormat)){
      return createQueryInvalidResponse(startIndex, 0);
    }
    uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults resultsFromDb = null;
    if(startIndex < 0){
      return createQueryInvalidResponse(startIndex, 0);
    }
    boolean queryFailed = false;    
    try{
      resultsFromDb = runConfigurationXPathQuery(queryString);
    }
    catch(XMLDBException xdbe){
      logger_.debug("XMLDBException when running configuration lfn query: " 
          + xdbe.getMessage());
      queryFailed = true;
    }

    if(queryFailed == true){
	// RHO: changed createQueryInvalidSyntaxResponse to createQueryInvalidResponse
	//      as for ILDG we need MDC_INVALID_REQUEST and not MDC_INVALID_SYNTAX
      return createQueryInvalidResponse( startIndex, 0 );
    }    
    //OLD: Check if results have no data, if so respond with MDC_NO_DATA
    //RHO: Replaced createResponseZeroResults with createResponseZeroResultsSuccess
    //     so it complies with ILDG test suite (no results is still a success)
    if(resultsFromDb.count()==0){
      return createResponseZeroResultsSuccess(startIndex, 0);
    }
    QueryResultsParser parser = new QueryResultsParser(this.driver_);
    org.lqcd.ildg.mdc.QueryResults resultsToReturn = null; 
    try{
      logger_.debug("Parsing query results.");
      resultsToReturn = parser.prepareRawConfigLFNResultsForReturn( resultsFromDb, 
          startIndex, 
          maxResults );
    }catch(ResultsParserException re){
      logger_.debug("Query results could not be parsed.");
      resultsToReturn = createQueryInvalidResponse(
          startIndex, resultsFromDb.count());
    }
    return resultsToReturn;
  }

  
  /**
   * Allows an ensemble metadata document to be obtained from the database.
   * @param ensembleURI The URI of the ensemble document required.
   * @return
   * @throws java.rmi.RemoteException
   */
  public org.lqcd.ildg.mdc.MetadataResult getEnsembleMetadata(
      java.lang.String ensembleURI) throws java.rmi.RemoteException {
    org.lqcd.ildg.mdc.MetadataResult res = new MetadataResult();

    uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults qr = null;
    qr = getEnsembleDocumentUsingSpecifiedIdentifyingTagname("markovChainURI", ensembleURI);
    if(qr==null){
      logger_.debug("Problem occurred, returning MDC_FAILURE");
      res.setStatusCode("MDC_FAILURE");
      return res;
    }
    else if(qr.count()==0){    
      // Retry using old element name
      logger_.debug("Trying to match on markovChainLFN");
      qr = getEnsembleDocumentUsingSpecifiedIdentifyingTagname("markovChainLFN", ensembleURI);
    }
    //Check for second error
    if(qr==null){
      logger_.debug("Problem occurred, returning MDC_FAILURE");
      res.setStatusCode("MDC_FAILURE");
      return res;
    }
    else if(qr.count()==0){    
      logger_.debug("Setting status MDC_NO_DATA");
      res.setStatusCode("MDC_NO_DATA");
      res.setDocument(null);
    }
    else{
      String[] results = qr.getResults();      
      logger_.debug("Setting status MDC_SUCCESS");
      res.setStatusCode("MDC_SUCCESS");
      res.setDocument(results[0]);
    }
    return res;    
  }

  /**
   * Retrieves an ensemble metadata document from the database based on its ensemble URI. Attempts to 
   * match on markovChainURI and markovChainLFN, to cope with older versions of the schema.
   * @param uriElementName
   * @param ensembleURI
   * @return
   */
  private uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults 
    getEnsembleDocumentUsingSpecifiedIdentifyingTagname(String uriElementName, String ensembleURI){
    uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults qr = null;
    String query = "/markovChain["+uriElementName+"='"+ensembleURI+"']";
    //String query = "/markovChain[//"+uriElementName+"='"+ensembleURI+"']";
    logger_.debug("Getting Ensemble " + query);
    boolean problemOccurred = false;
    try{
      qr = runEnsembleXPathQuery(query);      
    } 
    catch(XMLDBException xdbe){
      logger_.debug("Problem occured running query: " + xdbe.getMessage());
      problemOccurred = true;
    }
    if(qr == null){
      logger_.debug("Null query results returned.");
      problemOccurred = true;
    }
    if(qr.count() > 1){
      logger_.debug("More than one document matches!");
      problemOccurred = true;
    } 
    if(problemOccurred){
      return null;
    }
    else {
      return qr;
    }
  }
  
  
  /**
   * Returns the metadata document associated with the data LFN supplied.
   * @param configurationLFN
   * @return
   * @throws java.rmi.RemoteException
   */
  public org.lqcd.ildg.mdc.MetadataResult getConfigurationMetadata(
      java.lang.String configurationLFN) throws java.rmi.RemoteException {
    String query = "/gaugeConfiguration/markovStep[dataLFN='"+configurationLFN+"']/..";
    //String query = "/gaugeConfiguration[//dataLFN='"+configurationLFN+"']";
    org.lqcd.ildg.mdc.MetadataResult res = new MetadataResult();
    logger_.debug("Getting Config " + query);
    boolean problemOccurred = false;
    uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults qr = null;
    try{
      qr = runConfigurationXPathQuery(query);      
    } 
    catch(XMLDBException xdbe){
      logger_.debug("Problem occured running query: " + xdbe.getMessage());
      problemOccurred = true;
    }
    if(qr == null){
      logger_.debug("Null query results returned.");
      problemOccurred = true;
    }
    if(qr.count() > 1){
      logger_.debug("More than one document matches!");
      problemOccurred = true;
    } 
    if(problemOccurred){
      logger_.debug("Problem occurred, returning MDC_FAILURE");
      res.setStatusCode("MDC_FAILURE");
      return res;
    }else{       
      String[] results = qr.getResults();    
      if(qr.count()==0){
        logger_.debug("Setting status MDC_NO_DATA");
        res.setStatusCode("MDC_NO_DATA");
        res.setDocument(null);
      }
      else{
        logger_.debug("Setting status MDC_SUCCESS");
        res.setStatusCode("MDC_SUCCESS");
        res.setDocument(results[0]);
      }
      return res;
    }
  }

  /**
   * Returns the metadata document associated with the ensemble URI supplied.
   * @return
   * @throws java.rmi.RemoteException
   */
  public org.lqcd.ildg.mdc.MDCinfo getMDCinfo() throws java.rmi.RemoteException {
    MDCinfo mdcInfo = new MDCinfo();
    mdcInfo.setVersion("1.0");
    mdcInfo.setGroupName("UKQCD");
    mdcInfo.setGroupURL("http://ukqcd.epcc.ed.ac.uk");
    String [] types = new String[allowedQueryTypes_.size()];
    for(int i=0; i<allowedQueryTypes_.size(); i++){
      types[i] = (String)allowedQueryTypes_.get(i);
    }
    mdcInfo.setQueryTypes(types);

    return mdcInfo;
  }
  
  
  /**
   * This method issues a query against the configuration metadata documents in the 
   * database. In this implementation, the query is actually run against the whole database.
   * At present, all documents are stored in the root collection of the database. There is 
   * the possibility that queries intended for configurations return results from ensembles,
   * and vice-versa. In future different collections may be used to prevent this. 
   * 
   * This method will make coping with such changes easier in the future.
   * @param query
   * @return
   */
  private uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults runConfigurationXPathQuery(
      String query ) throws XMLDBException{
    driver_.setNamespace("http://www.lqcd.org/ildg/QCDml/config1.3");

    return runXPathQuery(query);    
  }
  
  /**
   * This method issues a query against the ensmeble metadata documents in the 
   * database. In this implementation, the query is actually run against the whole database.
   * At present, all documents are stored in the root collection of the database. There is 
   * the possibility that queries intended for configurations return results from ensembles,
   * and vice-versa. In future different collections may be used to prevent this. 
   * 
   * This method will make coping with such changes easier in the future.
   * @param query
   * @return
   */
  private uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults runEnsembleXPathQuery(
      String query ) throws XMLDBException{
      
      driver_.setNamespace("http://www.lqcd.org/ildg/QCDml/ensemble1.4");

      return runXPathQuery(query);    
  }
  
  
  /**
   * Runs an arbitrary query against the entire database.
   * @param query
   * @return
   * @throws XMLDBException
   */
  private uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults runXPathQuery(String query)
  throws XMLDBException{
    return driver_.runQuery(query);
  }

  
  /**
   * Checks that queryFormat is contained in allowedQueryTypes_. Currently only
   * Xpath is allowed.
   * @param queryFormat
   */
  private boolean queryFormatOK(String queryFormat){
    if(!allowedQueryTypes_.contains(queryFormat)) {
      return false;
    }
    return true;
  }
  
  
  /**
   * Returns a new QueryResults object with the status field set to MDC_INVALID_REQUEST
   * and all other fields set to 0/null.
   */
  private org.lqcd.ildg.mdc.QueryResults createQueryInvalidResponse(
      int startIndex, int totalResults){
    org.lqcd.ildg.mdc.QueryResults res = new org.lqcd.ildg.mdc.QueryResults(
        "MDC_INVALID_REQUEST", //statusCode,
        timeAsString(), //queryTime,
        totalResults, //totalResults,
        startIndex, //startIndex,
        0, //numberOfResults,
        null //new String[]{null}// results
    );
    return res;
  }

  
  /**
   * Returns a new QueryResults object with the status field set to MDC_INVALID_SYNTAX
   * and all other fields set to 0/null.
   */
  private org.lqcd.ildg.mdc.QueryResults createQueryInvalidSyntaxResponse(
      int startIndex, int totalResults){
    org.lqcd.ildg.mdc.QueryResults res = new org.lqcd.ildg.mdc.QueryResults(
        "MDC_INVALID_REQUEST", //statusCode,
        timeAsString(), //queryTime,
        totalResults, //totalResults,
        startIndex, //startIndex,
        0, //numberOfResults,
        null //new String[]{null}// results
    );
    return res;
  }
  
  /**
   * Returns a new queryResults object with the status field set to MDC_NO_DATA
   * and all other fields set to 0/null.
   */
  private org.lqcd.ildg.mdc.QueryResults createResponseZeroResults(
      int startIndex, int totalResults){
    org.lqcd.ildg.mdc.QueryResults res = new org.lqcd.ildg.mdc.QueryResults(
        "MDC_NO_DATA", //statusCode,
        timeAsString(), //queryTime,
        totalResults, //totalResults,
        startIndex, //startIndex,
        0, //numberOfResults,
        null //new String[]{null}// results
    );
    return res;
  }

  /**
   * RHO: Returns a new queryResults object with the status field set to MDC_SUCCESS
   * and all other fields set to 0/null.
   * As zero results is considered as success too.
   */
  private org.lqcd.ildg.mdc.QueryResults createResponseZeroResultsSuccess(
      int startIndex, int totalResults){
    org.lqcd.ildg.mdc.QueryResults res = new org.lqcd.ildg.mdc.QueryResults(
        "MDC_SUCCESS", //statusCode,
        timeAsString(), //queryTime,
        totalResults, //totalResults,
        startIndex, //startIndex,
        0, //numberOfResults,
        null //new String[]{null}// results
    );
    return res;
  }
  
  /**
   * Returns the date as an appropriately formatted string. For the QueryResults
   * queryTime field.
   * @return
   */
  private String timeAsString(){
    Date now = new Date();
    DateFormat df = DateFormat.getInstance();
    String s = df.format(now);
    return s;
  }
  
}
