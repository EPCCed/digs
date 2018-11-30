/***********************************************************************
*
*   Filename:   MultipleWebServiceQueryRunner.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    Implementation of the UserQueryRunner interface that uses 
*               the metadata catalogue web service to run user queries. 
*               Issues the query against multiple web services and 
*               aggregates the results. Allows for asynchronous execution.
*
*   Contents:   MultipleWebServiceQueryRunner class.
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


package uk.ac.ed.epcc.qcdgrid.metadata.catalogue;

import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MetadataCatalogueWebServiceQuery;

import java.util.ArrayList;
import java.util.List;
import java.util.Date;
import javax.xml.rpc.ServiceException;

/**
 * This is the MultipleWebServiceQuery runner class. It takes a list of web
 * services that are expected to conform to the ILDG metadata catalogue web
 * service specification. The query passed to the runBlahQuery methods is run
 * against each web service. The results are aggregaged and returned in an
 * instance of ILDGQueryResult. Each query is issued concurrently so this class
 * is threaded.
 * 
 * Concurrency requirements: The process should be haltable by the caller so
 * needs to be run in another thread.
 * 
 * @author daragh
 * 
 */
public class MultipleWebServiceQueryRunner implements UserQueryRunner {

  private List serviceEndpoints_;

  private List errors_ = new ArrayList();

  private List runningQueries_ = new ArrayList();

  private boolean stopped_ = false;
  
  private boolean hasRun_ = false;

  private static final long MDC_TIMEOUT = 120000;

  /**
   * Sets up the list of hosts that the runner should attempt to run the query
   * against.
   * 
   * @param hosts
   */
  public MultipleWebServiceQueryRunner(List serviceEndpoints) {
    if (serviceEndpoints == null) {
      throw new IllegalArgumentException(
          "List of service endpoints should not be null.");
    }
    serviceEndpoints_ = serviceEndpoints;
    hasRun_ = false;
  }

  /**
   * Runs a configuration query and waits for it to complete. Uses multiple
   * MetadataCatalogueWebServiceQuery objects to launch the queries.
   */
  public QCDgridQueryResults runConfigurationQuery(String XPathQuery)
      throws QueryExecutionException {
    setQueriesRunning(XPathQuery, QueryType.CONFIGURATION_QUERY_TYPE);
    ILDGQueryResults res;
    if(!stopped_){
      res = aggregateResults(QueryType.CONFIGURATION_QUERY_TYPE);
    }
    else{
      res = null;
    }
    return res;
  }

  /**
   * Runs an ensemble query and waits for it to complete. Uses multiple
   * MetadataCatalogueWebServiceQuery objects to launch the queries.
   */
  public QCDgridQueryResults runEnsembleQuery(String XPathQuery)
      throws QueryExecutionException {
    setQueriesRunning(XPathQuery, QueryType.ENSEMBLE_QUERY_TYPE);
    ILDGQueryResults res;
    if(!stopped_){
      res = aggregateResults(QueryType.ENSEMBLE_QUERY_TYPE);
    }
    else{
      System.out.println("returning null results");
      res = null;
    }
    return res;
  }

  /**
   * Not supported yet
   */
  public QCDgridQueryResults runCorrelatorQuery(String XPathQuery)
      throws QueryExecutionException {
    return null;
  }

  /**
   * Stops all running queries. Unused at the moment.
   */
  public void stop() {
    stopped_ = true;
  }

  /**
   * Returns the list of service endpoints that are being queries.
   * @return
   */
  public List getEndpoints() {
    return serviceEndpoints_;
  }

  
  /**
   * If any of the queries reported errors, this is true.
   */
  public boolean hasErrors() {
    if (errors_.size() > 0) {
      return true;
    }
    return false;
  }

  /**
   * Returns a list of errors encountered when running the queries.
   * @return
   */
  public List getErrors() {
    return errors_;
  }

  
  /**
   * 
   *
   */
  private void stopAllQueries() {
    for (int i = 0; i < runningQueries_.size(); i++) {
      MetadataCatalogueWebServiceQuery q = (MetadataCatalogueWebServiceQuery) runningQueries_
          .get(i);
      q.stop();
    }
  }


  private void timeOutQueries() {
    for (int i = 0; i < runningQueries_.size(); i++) {
      MetadataCatalogueWebServiceQuery q = (MetadataCatalogueWebServiceQuery) runningQueries_
          .get(i);
      if (!q.isFinished()) {
	  q.stop();
	  System.err.println("Warning: service "+q.getEndpoint()+" timed out");
      }
    }    
  }
  
  /**
   * Adds an error to the list.
   * @param error
   */
  private void setError(String error) {
    errors_.add(error);
  }

  
  /**
   * Checks each query in turn to see if they are all finished.
   * @return
   */
  private boolean allQueriesFinished() {
    boolean finished = true;
    for (int i = 0; i < runningQueries_.size(); i++) {
      MetadataCatalogueWebServiceQuery q = (MetadataCatalogueWebServiceQuery) runningQueries_
          .get(i);
      if (!q.isFinished()) {
        finished = false;
      }
    }
    return finished;
  }

  
  /**
   * Groups the results of all subqueries together.
   */
  private ILDGQueryResults aggregateResults(String queryType) {
    ILDGQueryResults res = new ILDGQueryResults(
        queryType);
    for (int i = 0; i < runningQueries_.size(); i++) {
      MetadataCatalogueWebServiceQuery q = (MetadataCatalogueWebServiceQuery) runningQueries_
          .get(i);
      ResultInfo[] results = q.getResults(); 
      if(results != null){
        res.addResults(results);
      }
    }
    return res;
  }

  
  /**
   * Creates a MetadataCatalogueWebServiceQuery object for each query an
   * runs it in its own thread.
   * @param XPathQuery
   * @param queryType
   */
  private void setQueriesRunning(String XPathQuery, String queryType)
    throws QueryExecutionException {
    if(hasRun_==true){
      throw new QueryExecutionException("This has already been used to run a query, "
          + "please use a new runner object");
    }
    hasRun_ = true;
    for (int i = 0; i < serviceEndpoints_.size(); i++) {
      try {
        MetadataCatalogueWebServiceQuery query = new MetadataCatalogueWebServiceQuery(
            (String) serviceEndpoints_.get(i), XPathQuery, queryType);
        Thread runner = new Thread(query);
        runningQueries_.add(query);
        ////System.out.println("Added query " + i + " to list.");
        runner.start();
      } catch (ServiceException se) {
        setError("Service exception caught: " + se.getMessage());
      }
    }

    Date startDate = new Date();
    long startTime = startDate.getTime();
    while (true) {
      if (stopped_) {
        // stop all queries
        stopAllQueries();
        return;
      }
      if (allQueriesFinished()) {
        ////System.out.println("All subqueries are finished.");
        aggregateErrors();
        break;
      }
      Thread.yield();

      Date endDate = new Date();
      long endTime = endDate.getTime();

      if ((endTime - startTime) >= MDC_TIMEOUT) {
	  timeOutQueries();
	  return;
      }
    }
  }

  
  /**
   * Aggregates the errors that have occurred during this run.
   */
  private void aggregateErrors(){
    for(int i=0; i<runningQueries_.size(); i++){
      List errors = ((MetadataCatalogueWebServiceQuery)runningQueries_.get(i))
        .getErrorList();
      if(errors != null){
        for(int j=0; j<errors.size();j++){
          Object error = errors.get(j);
          String errorMsg = null;
          if(error instanceof Exception){
            errorMsg = "Exception occurred running webservice query: \n";
            errorMsg += ((Exception)error).getMessage();
          }
          else{
            errorMsg = error.toString();
          }
        }
      }
    }
  }
}
