/***********************************************************************
*
*   Filename:   MetadataCatalogueWebServiceQuery.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    A subclass of QCDgridQueryResults specifically for ILDG
*               query results.
*
*   Contents:   MetadataCatalogueWebServiceQuery class.
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

import java.rmi.RemoteException;
import java.util.List;
import java.util.ArrayList;
import javax.xml.rpc.ServiceException;

import org.lqcd.ildg.mdc.MetadataCatalogueServiceLocator;
import org.lqcd.ildg.mdc.QueryResults;
import org.lqcd.ildg.mdc.MDCInterface;


import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;

/**
 * Use this class whenever you want to issue an Xpath query against a metadata
 * catalogue web service.
 * 
 * The manner in which the query is executed is configurable in 
 * $BROWSER_HOME/browser.properties. If the queryrunner.maxresults property
 * is set to 0 or a negative number, a single query will be issued to the
 * catalogue. If the number is positive, multiple queries will be issued, 
 * each returning a maximum of that number of results. This may be
 * necessary in certain circumstances (for example, situations where
 * the server administrator has limited the number of results returned).
 * 
 * This class implements runnable, so the query can be issued in its own 
 * thread.
 * 
 * @author daragh
 *
 */

public class MetadataCatalogueWebServiceQuery implements Runnable{
  
  private String endPoint_;
  private String query_;
  private String queryType_;
  private int maximumQueryResults_;
  private ResultInfo [] results_;  
  private boolean running_ = false;
  private boolean stopped_ = false;
  private boolean finished_ = false;
  private MDCInterface webService_;
  private List errors_ = new ArrayList();
  

  /**
   * 
   * @param endPoint The web service endpoint
   * @param query The Xpath query to issue
   * @param queryType Whether this is an ensemble query or configuration 
   * query; see the QueryType class in browser/QueryHandler
   * @throws ServiceException
   */
  public MetadataCatalogueWebServiceQuery(
      String endPoint, 
      String query, 
      String queryType
      ) throws ServiceException {
    query_ = query;
    endPoint_ = endPoint;
    if(!queryType.equals(QueryType.ENSEMBLE_QUERY_TYPE)){
      if(!queryType.equals(QueryType.CONFIGURATION_QUERY_TYPE)){
        throw new IllegalArgumentException("Invalid query type specified, must be one of "
            + QueryType.ENSEMBLE_QUERY_TYPE + ", " + QueryType.CONFIGURATION_QUERY_TYPE);
      }
    }
    queryType_ = queryType;
    try{
      FeatureConfiguration fc = new FeatureConfiguration();
      maximumQueryResults_ = fc.getMDCWSQueryMaxResults();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      maximumQueryResults_ = -1;
    }
    // Zero does not make sense, assume that < 1 meant.
    if(maximumQueryResults_ == 0){
      maximumQueryResults_ = -1;
    }
    MetadataCatalogueServiceLocator locator 
      = new MetadataCatalogueServiceLocator();
    locator.setMetadataCatalogueEndpointAddress(endPoint_);
    webService_ = locator.getMetadataCatalogue();
    ////System.out.println("Created MDCWS query runner: " + endPoint + " " + query + " " + queryType);
  }
  
  /**
   * Is the query running?
   * @return
   */
  public boolean isRunning(){
    return running_;
  }
  
  
  /**
   * Has the query finished executing?
   * @return
   */
  public boolean isFinished(){
    return finished_;
  }
  
  /**
   * Stop this query running.
   */
  public void stop(){
    // stop running.
    stopped_ = true;
    running_ = false;
  }
  
  /**
   * Get the results of the query. Depends on doQuery having been run.
   * @return
   */
  public ResultInfo [] getResults(){    
    if(stopped_ == true){
      ////System.out.println("returning null");
      return null;
    }
    else{
      return results_;
    }
  }
  
  
  /**
   * Executes doQuery.
   */
  public void run(){
    running_ = true;
    doQuery();
  }
  
  public String getQuery(){
    return query_;
  }
  
  public String getEndpoint(){
    return endPoint_; 
  }
  
  public String getQueryType(){
    return queryType_;
  }
  
  
  /**
   * Returns the list of errors encountered if any, null if 
   * no errors were reported
   * @return
   */
  public List getErrorList(){
    if(errors_.size()>0){
      return errors_;
    }
    else{
      return null;
    }
  }
 
  
  /**
   * If this is less than 0, then all the query results will be obtained in one go.
   * Otherwise, the runner makes several batched subqueries and aggregates 
   * the results.
   * @return
   */
  public int getMaxResultsPerQuery(){
    return maximumQueryResults_;
  }

  
  /**
   * Was this stopped deliberately?
   * @return
   */
  public boolean wasStopped(){
    return stopped_;
  }
  
  
  /**
   * Get this ready for use after a failed run.
   *
   */
  public void reset(){
    stopped_ = false;
    running_ = false;
  }
  
  /**
   * Run the query as specified in the endPoint, query, queryType properties
   * of this object. Exact operation depends on the queryrunner.maxresults
   * propery in $BROWSER_HOME/browser.properties.
   */
  public void doQuery(){
    finished_ = false;
    if(getMaxResultsPerQuery() < 0 ){
      try{
        doQueryAllResults();
      }
      catch(RemoteException re){
        setError(re.getMessage());
      }
    }
    else{
      doBatchQuery();
    }
    finished_ = true;
  }
 
  /**
   * Add an error to the list of errors encounterd during this query.
   * @param err
   */
  private void setError(Object err){
    this.errors_.add(err);
  }
  
  
  /**
   * Execute the query asking the server for all results. Place them 
   * in the results_ array.
   */
  private void doQueryAllResults() throws RemoteException{
    QueryResults res = null;
    if(getQueryType().equals(QueryType.CONFIGURATION_QUERY_TYPE)){
      res = doSingleConfigurationQuery(0,-1);      
    }
    else if(getQueryType().equals(QueryType.ENSEMBLE_QUERY_TYPE)){
      res = doSingleEnsembleQuery(0,-1);
    }
    // Check for errors, setError if found
    List resultList = new ArrayList();
    resultList.add(res);
    generateAggregateResults(resultList);
  }
  
  /**
   * Send multiple queries to the server, depending on the the number specified
   * in getMaxResultsPerQuery(). This is a scrolling mechanism that could help 
   * server side performance.
   * 
   * TODO: Make this recursive?
   */
  private void doBatchQuery(){
    List resultList = new ArrayList();
    int startIndex = 0, resultsObtained = 0, totalResults = 0;
    QueryResults qr = null;    
    // Start off by doing a single query, it may be enough
    if(this.getQueryType().equals(QueryType.CONFIGURATION_QUERY_TYPE)){
      try{
        qr = doSingleConfigurationQuery(startIndex,getMaxResultsPerQuery());
      }
      catch(RemoteException re){
        String error = "A particular query to " + this.getEndpoint() + " failed:\n";
        error += re.getMessage();
        error += "(startIndex=" + startIndex +")";
        setError(error);
      }
    }
    else if(this.getQueryType().equals(QueryType.ENSEMBLE_QUERY_TYPE)){
      try{
        qr = doSingleEnsembleQuery(startIndex,getMaxResultsPerQuery());
      }
      catch(RemoteException re){
        String error = "A particular query to " + this.getEndpoint() + " failed:\n";
        error += re.getMessage();
        error += "(startIndex=" + startIndex +")";
        setError(error);
      }
    }
    
    if(qr != null){
      if(!(qr.getStatusCode().equals("MDC_SUCCESS"))){
        String error = "Error executing query: " + qr.getStatusCode();
        setError(error);
      }
      else{
        resultList.add(qr);
      }
    }
    else{
      setError("A null QueryResults was returned.");
      return;
    }    
    resultsObtained += qr.getNumberOfResults();
    totalResults = qr.getTotalResults();
    if(wasStopped()){
      return;
    }
    if(resultsObtained < qr.getTotalResults()){
      while( resultsObtained < totalResults ){
        startIndex += getMaxResultsPerQuery();
        if(this.getQueryType().equals(QueryType.CONFIGURATION_QUERY_TYPE)){
          try{
            qr = doSingleConfigurationQuery(startIndex,getMaxResultsPerQuery());
          }
          catch(RemoteException re){
            String error = "A particular query to " + this.getEndpoint() + " failed:\n";
            error += re.getMessage();
            error += "(startIndex=" + startIndex +")";
            setError(error);
          }
        }
        else if(this.getQueryType().equals(QueryType.ENSEMBLE_QUERY_TYPE)){
          try{
            qr = doSingleEnsembleQuery(startIndex,getMaxResultsPerQuery());
          }
          catch(RemoteException re){
            String error = "A particular query to " + this.getEndpoint() + " failed:\n";
            error += re.getMessage();
            error += "(startIndex=" + startIndex +")";
            setError(error);
          }
        }
        if(qr!=null){
          if(!(qr.getStatusCode().equals("MDC_SUCCESS"))){
            String error = "Error executing query: " + qr.getStatusCode();
            setError(error);
          }
          else{
            resultList.add(qr);
          }
        }
        else{
          setError("A null QueryResults was returned.");
          return;
        }
        resultsObtained += qr.getNumberOfResults();        
      }
      if(wasStopped()){
        return;
      }
    }
    generateAggregateResults(resultList);
  }
  
  
  
  /**
   * Takes a List of individual QueryResults objects and creates the result array
   * that will be returned.
   * @param resultList
   */
  private void generateAggregateResults(List resultList){
    int resultCount = 0;
    QueryResults res = null;        
    for(int i=0; i<resultList.size(); i++){
      res = (QueryResults) resultList.get(i);
      resultCount += res.getNumberOfResults();      
    }
    ////System.out.println("There are " + resultCount + " results");
    results_ = new ResultInfo[resultCount];
    int currentResultIndex = 0;
    ////System.out.println("Array is " + results_.length + " long");
    for(int i = 0; i<resultList.size(); i++){
      res = (QueryResults) resultList.get(i);
      ////System.out.println("Adding query result " + i + " which has " + res.getNumberOfResults() + " members.");
      for(int j=0; j<res.getNumberOfResults(); j++){        

        ////System.out.println("--Adding result " + j + " " + res.getResults()[j]);
        results_[currentResultIndex] = new ResultInfo(
            res.getResults()[j], 
            this.getEndpoint()
              );
        currentResultIndex++;
      }
    }
  }
  
  
  
  /**
   * Issues a single configuration query against the web service.
   * @param startIndex The startIndex of the results to be returned.
   * @param maxResults The maximum results to be returned. -1 means return all.
   * @return
   * @throws RemoteException
   */
  private QueryResults doSingleConfigurationQuery(int startIndex, int maxResults)
  throws RemoteException{
    ////System.out.println("doSingleConfigurationQuery, " + startIndex + "," + maxResults);
    QueryResults res = webService_.doConfigurationLFNQuery("Xpath",query_,startIndex,maxResults);
    return res;
  }
  
  
  /**
   * Issues a single ensemble query against the web service.
   * @param startIndex The startIndex of the results to be returned.
   * @param maxResults The maximum results to be returned. -1 means return all.
   * @return
   * @throws RemoteException
   */
  private QueryResults doSingleEnsembleQuery(int startIndex, int maxResults)
    throws RemoteException{
    QueryResults res = webService_.doEnsembleURIQuery("Xpath",query_,startIndex,maxResults); 
    return res;    
  }
  
  
}