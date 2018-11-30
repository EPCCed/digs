/***********************************************************************
*
*   Filename:   MDCWebServiceTest.java
*
*   Authors:    Daragh Byrne           (daragh)   EPCC.
*
*   Purpose:    Testing the metadata catalogue web service
*
*   Content:    MDCWebServiceTest class
*
*   Used in:    Test suite
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2005 The University of Edinburgh
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



/*
 * This implements the tests discussed in the sensible_tests.txt document. 
 * It makes calls to the metadata cataolgue web service and tests the results.
 * Depends on QCDgridWebServices.jar, QCDgrid.jar
 */

package test.org.lqcd.ildg.mdc;

import java.io.FileOutputStream;
import java.io.FileWriter;
import java.net.URL;
import java.rmi.RemoteException;
import java.util.ArrayList;

import junit.framework.TestCase;

import org.lqcd.ildg.mdc.QueryResults;
import org.lqcd.ildg.mdc.MDCinfo;
import org.lqcd.ildg.mdc.MDCInterface;
import org.lqcd.ildg.mdc.MetadataCatalogueServiceLocator;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridLogicalNameExtractor;
import uk.ac.ed.epcc.qcdgrid.metadata.EnsembleNameExtractor;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

public class MDCWebServiceTest extends TestCase {

  /**
   * The service to be tested.
   */
  private MDCInterface service_;

  /**
   * Instantiates a new metadata catalogue service proxy object for use in the tests.
   */
  public void setUp() {
    try {
      MetadataCatalogueServiceLocator locator = new MetadataCatalogueServiceLocator();
      service_ = locator.getMetadataCatalogue();
    } catch (javax.xml.rpc.ServiceException se) {
      fail("Service creation failed: " + se.getMessage());
    }
  }


  
  /*
   * ####################################################################
   * doEnsembleURIQuery tests.
   * ####################################################################
   */
  
  /**
   * Tests the query format NOSUCHFORMAT is rejected in an appropriate manner. 
   */
  public void testEnsembleURIQueryRejectsBadQueryFormat(){
    // test that invalid query formats are rejected 
    QueryResults results=null;
    try{
      results = service_.doEnsembleURIQuery("NOSUCHFORMAT", "empty", 0, 2);      
    }
    catch( RemoteException re){
      fail("Unexpected exception");
    }
    assertNotNull(results);
    assertEquals("MDC_INVALID_REQUEST", results.getStatusCode());
    assertEquals(0, results.getNumberOfResults());
    assertEquals(0, results.getTotalResults());
    assertEquals(0, results.getStartIndex());
    assertNotNull(results.getQueryTime());
  }
  
  
  /**
   * Queries for all ensemble URI results and counts them and checks values correct.
   */
  public void testEnsembleURIQueryAllResultsReturned(){
    QueryResults results = null;
    try{
      results = service_.doEnsembleURIQuery(
        "Xpath", "/markovChain", 
        0, -1    
      );
     // printQueryResults(results);
      
    }catch(Exception e){
      fail( "Unexpected exception in doEnsembleURIQuery: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals(3, results.getNumberOfResults());
    ArrayList resultList = new ArrayList();
    for(int i=0; i<results.getNumberOfResults(); i++){
      resultList.add(results.getResults()[i]);
    }
    assertTrue(resultList.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS1"));
    assertTrue(resultList.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS2"));
    assertTrue(resultList.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS3"));      
  }
  
  /**
   * Startindex should never be negative.
   */
  public void testEnsembleURIRejectsNegativeStartIndex(){
    QueryResults results = null;
    try{
      results = service_.doEnsembleURIQuery(
        "Xpath", "/markovChain", 
        -1, -1    
      );
     // printQueryResults(results);
      
    }catch(Exception e){
      fail( "Unexpected exception in doEnsembleURIQuery: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals("MDC_INVALID_REQUEST", results.getStatusCode());
  }
  
  /**
   * Tests the scrolling properties of the doEnsembleURIQuery operation.
   */
  public void testEnsembleURIQueryScrolling(){
    QueryResults results = null;
    // Run query with startIndex=1, max=1 and check that correct numbers of 
    // results returned (note that this assumes eXist always returns query results in
    // correct order)
    try{
      results = service_.doEnsembleURIQuery("Xpath", "/markovChain", 1, 1 );      
    }catch(Exception e){
      fail( "Unexpected exception in doEnsembleURIQuery: " + e.getMessage());
    }
    //printQueryResults(results);
    assertNotNull(results);
    assertEquals(1, results.getNumberOfResults());
    assertEquals(3, results.getTotalResults());
  }

  /**
   * Tests that the LFNs are still returned even when a query is issued that 
   * does not contain the LFN in its result.
   */
  public void testEnsURIQueryNotReturningMarkovChainURIElement() {       
    QueryResults results=null;
    // Try with a query that does not return a full document, in fact, does not 
    // return a part of the document containing the dataLFN element
    try{
      results = service_.doEnsembleURIQuery(
        "Xpath", "//management/projectName", 
        0, -1    
      );   
    }catch(Exception e){
      fail("Unexpeceted exception in doEnsembleURIQuery: " + e.getMessage());
    }
    printQueryResults(results);
    assertNotNull(results);
    assertEquals(3, results.getNumberOfResults());
    ArrayList resultList = new ArrayList();
    for(int i=0; i<results.getNumberOfResults(); i++){
      resultList.add(results.getResults()[i]);
    }
    assertTrue(resultList.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS1"));
    assertTrue(resultList.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS2"));
    assertTrue(resultList.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS3"));     
  }
  
  /**
   * When a nonsensical Xpath expression is issued, the result should be 
   * MDC_NO_DATA in the status field, with 0/null in the other fields. When the 
   * Xpath is syntactically invalid, the field should be MDC_INVALID_SYNTAX, if
   * it is possible for the 
   */
  public void testEnsembleURIQueryNonsenseXPath(){
    QueryResults results = null;  
    // Try with an obviously wrong query
    try{
      results = service_.doEnsembleURIQuery(
        "Xpath", "/this/query/is/bad", 
        2, 2    
      );      
    }catch(Exception e){
      fail("Unexpeceted exception: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals("MDC_NO_DATA", results.getStatusCode());
    assertEquals(0, results.getNumberOfResults());
    assertEquals(0, results.getTotalResults());
    assertEquals(2, results.getStartIndex());   
  }

  /**
   * Checks that MDC_INVALID_SYNTAX is returned when a syntactly incorrect query 
   * is issued.
   */
  public void testEnsembleURIQuerySyntaxErrorXPath(){
    QueryResults results = null;
    // Try with an obviously wrong query
    try{
      results = service_.doEnsembleURIQuery(
        "Xpath", "^^~$W", 
        0, -1    
      );      
    }catch(Exception e){
      fail("Unexpeceted exception: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals("MDC_INVALID_SYNTAX", results.getStatusCode());
    assertEquals(0, results.getNumberOfResults());
    assertEquals(0, results.getTotalResults());
    assertEquals(0, results.getStartIndex());   
  }
  

  
  /*
   * ####################################################################
   * doConfigurationLFNQuery tests.
   * ####################################################################
   */
  
  /**
   * Tests the query format NOSUCHFORMAT is rejected in an appropriate manner. 
   */
  public void testConfigurationLFNQueryRejectsBadQueryFormat(){
    // test that invalid query formats are rejected 
    QueryResults results=null;
    try{
      results = service_.doConfigurationLFNQuery("NOSUCHFORMAT", "empty", 0, 9);      
    }
    catch( RemoteException re){
      fail("Unexpected exception");
    }
    assertNotNull(results);
    assertEquals("MDC_INVALID_REQUEST", results.getStatusCode());
    assertEquals(0, results.getNumberOfResults());
    assertEquals(0, results.getTotalResults());
    assertEquals(0, results.getStartIndex());
    assertNotNull(results.getQueryTime());
  }
  
  
  /**
   * Queries for all Configuration URI results and counts them and checks values correct.
   */
  public void testConfigurationLFNQueryAllResultsReturned(){
    QueryResults results = null;
    try{
      results = service_.doConfigurationLFNQuery(
        "Xpath", "/gaugeConfiguration", 
        0, -1    
      );
     // printQueryResults(results);
      
    }catch(Exception e){
      fail( "Unexpected exception in doConfigurationLFNQuery: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals(9, results.getNumberOfResults());
    ArrayList resultList = new ArrayList();
    for(int i=0; i<results.getNumberOfResults(); i++){
      resultList.add(results.getResults()[i]);
    }
    assertTrue(resultList.contains("lfn://ukqcd/ildg/test/Cfg11"));
    assertTrue(resultList.contains("lfn://ukqcd/ildg/test/Cfg22"));
    assertTrue(resultList.contains("lfn://ukqcd/ildg/test/Cfg31"));      
  }
  
  /**
   * Startindex should never be negative.
   */
  public void testConfigurationLFNRejectsNegativeStartIndex(){
    QueryResults results = null;
    try{
      results = service_.doConfigurationLFNQuery(
        "Xpath", "/markovChain", 
        -1, -1    
      );
     // printQueryResults(results);
      
    }catch(Exception e){
      fail( "Unexpected exception in doConfigurationLFNQuery: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals("MDC_INVALID_REQUEST", results.getStatusCode());
  }
  
  /**
   * Tests the scrolling properties of the doConfigurationLFNQuery operation.
   */
  public void testConfigurationLFNQueryScrolling(){
    QueryResults results = null;

    try{
      results = service_.doConfigurationLFNQuery("Xpath", "/gaugeConfiguration", 1, 3 );      
    }catch(Exception e){
      fail( "Unexpected exception in doConfigurationLFNQuery: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals(3, results.getNumberOfResults());
    assertEquals(9, results.getTotalResults());
  }

  /**
   * Tests that the LFNs are still returned even when a query is issued that 
   * does not contain the LFN in its result.
   */
  public void testConfigLFNQueryNotReturningDataLFNElement() {       
    QueryResults results=null;
    // Try with a query that does not return a full document, in fact, does not 
    // return a part of the document containing the dataLFN element
    try{
      results = service_.doConfigurationLFNQuery(
        "Xpath", "//management/crcCheckSum", 
        0, -1    
      );   
    }catch(Exception e){
      fail("Unexpected exception in doConfigurationLFNQuery: " + e.getMessage());
    }
    //printQueryResults(results);
    assertNotNull(results);
    assertEquals(9, results.getNumberOfResults());
    ArrayList resultList = new ArrayList();
    for(int i=0; i<results.getNumberOfResults(); i++){
      resultList.add(results.getResults()[i]);
    }   
    assertTrue(resultList.contains("lfn://ukqcd/ildg/test/Cfg11"));
    assertTrue(resultList.contains("lfn://ukqcd/ildg/test/Cfg21"));
    assertTrue(resultList.contains("lfn://ukqcd/ildg/test/Cfg32"));   
  }
  
  /**
   * When a nonsensical Xpath expression is issued, the result should be 
   * MDC_NO_DATA in the status field, with 0/null in the other fields. When the 
   * Xpath is syntactically invalid, the field should be MDC_INVALID_SYNTAX, if
   * it is possible for the 
   */
  public void testConfigurationLFNQueryNonsenseXPath(){
    QueryResults results = null;
    // Try with an obviously wrong query
    try{
      results = service_.doConfigurationLFNQuery(
        "Xpath", "/this/query/is/bad", 
        2, 2    
      );      
    }catch(Exception e){
      fail("Unexpeceted exception: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals("MDC_NO_DATA", results.getStatusCode());
    assertEquals(0, results.getNumberOfResults());
    assertEquals(0, results.getTotalResults());
    assertEquals(2, results.getStartIndex());   
  }

  /**
   * Checks that MDC_INVALID_SYNTAX is returned when a syntactly incorrect query 
   * is issued.
   */
  public void testConfigurationLFNQuerySyntaxErrorXPath(){
    QueryResults results = null;
    // Try with an obviously wrong query
    try{
      results = service_.doConfigurationLFNQuery(
        "Xpath", "^^~$W", 
        0, -1    
      );      
    }catch(Exception e){
      fail("Unexpeceted exception: " + e.getMessage());
    }
    assertNotNull(results);
    assertEquals("MDC_INVALID_SYNTAX", results.getStatusCode());
    assertEquals(0, results.getNumberOfResults());
    assertEquals(0, results.getTotalResults());
    assertEquals(0, results.getStartIndex());   
  }

 
  
  
  /*
   * ####################################################################
   * getConfigurationMetadata tests.
   * ####################################################################
   */
  
  /**
   * Queries for a document that should exist in the catalogue and 
   * examines it to ensure the correct one is returned.
   */
  public void testGetConfigMetadata(){
    String lfn = "lfn://ukqcd/ildg/test/Cfg12";
    // Issue the query
    org.lqcd.ildg.mdc.MetadataResult res = null;
    String doc = null;
    // Try extracting the dataLFN
    String extractedLFN = null;
    try{
      res = service_.getConfigurationMetadata( lfn );
      if(res == null) {
        System.out.println("res null");
      }
      doc = res.getDocument();
      System.out.println(doc);
      extractedLFN = QCDgridLogicalNameExtractor.getLFNFromMetadataDocument(doc);
    }catch(Exception e){
      System.out.println(e.getClass().toString());
      fail("Unexpected exception in testGetConfigMDBasic: " + e.getMessage());      
    }
    assertEquals(lfn,extractedLFN);
  }
  
  
  /**
   * Queries for a document that should not exist in the catalogue and
   * ensures that an appropriate response is returned.
   */
  public void testGetConfigMetadataNotThere(){
    String lfn = "lfn://ukqcd/ildg/test/Cfg44";
    // Issue the query
    org.lqcd.ildg.mdc.MetadataResult res = null;
    String doc = null;
    try{
      res = service_.getConfigurationMetadata(lfn);      
    }catch(Exception e){
      fail("Unexpected exception in testGetConfigMDBasic: " + e.getMessage());      
    }
    assertNotNull(res);
    assertEquals("MDC_NO_DATA",res.getStatusCode());
    assertEquals(null, res.getDocument());
  }
  
  
  /**
   * Queries for a document that should exist in the catalogue and 
   * examines it to ensure the correct one is returned.
   */
  public void testGetEnsembleMetadata(){
    String lfn = "http://www.epcc.ed.ac.uk/ildg/test/ENS1";
    // Issue the query
    org.lqcd.ildg.mdc.MetadataResult res = null;
    String doc = null;
    // Try extracting the dataLFN
    String extractedLFN = null;
    try{
      res = service_.getEnsembleMetadata( lfn );
      if(res == null) {
        System.out.println("res null");
      }
      doc = res.getDocument();
      System.out.println("doc is: " + doc);      
      extractedLFN = new EnsembleNameExtractor().getEnsembleNameFromEnsembleDocument(doc);      
      System.out.println("Extracted name successfully.");
    }catch(Exception e){
      System.out.println(e.getClass().toString());
      fail("Unexpected exception in testGetEnsembleMetadata: " + e.getMessage());     
    }
    assertEquals(lfn,extractedLFN);
    
    lfn = "http://www.epcc.ed.ac.uk/ildg/test/ENS3";
    try{
      res = service_.getEnsembleMetadata( lfn );
      if(res == null) {
        System.out.println("res null");
      }
      doc = res.getDocument();
     // System.out.println(doc);      
      extractedLFN = new EnsembleNameExtractor().getEnsembleNameFromEnsembleDocument(doc);      
    }catch(Exception e){
      System.out.println(e.getClass().toString());
      fail("Unexpected exception in testGetEnsembleMetadata(2): " + e.getMessage());      
    }
    assertEquals(lfn,extractedLFN);
    
    
    
  }
  
  
  /**
   * Queries for a document that should not exist in the catalogue and
   * ensures that an appropriate response is returned.
   */
  public void testGetEnsembleMetadataNotThere(){
    String lfn = "http://www.epcc.ed.ac.uk/ildg/test/ENS4";
    // Issue the query
    org.lqcd.ildg.mdc.MetadataResult res = null;
    String doc = null;
    try{
      res = service_.getEnsembleMetadata(lfn);      
    }catch(Exception e){
      fail("Unexpected exception in testGetConfigMDBasic: " + e.getMessage());      
    }
    assertNotNull(res);
    assertEquals("MDC_NO_DATA",res.getStatusCode());
    assertEquals(null, res.getDocument());
  }
  
  /*
   * ####################################################################
   * Utilities.
   * ####################################################################
   */
  private void printQueryResults(QueryResults results){    
    if(results!=null){
      System.out.println("Recieved "+results.getNumberOfResults() + " results.");
      for(int i=0; i<results.getNumberOfResults(); i++){
        System.out.println(results.getResults()[i]);
      }
    }
    else{
      System.out.println("Null results recieved.");
    }
    
  }
  
  
  private void printQueryResults(String message, QueryResults results){
    System.out.println(message);
    printQueryResults(results);
  }
  
 
}
