package org.lqcd.ildg.mdc;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.text.DateFormat;
import java.util.Date;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.w3c.dom.Node;
import org.apache.log4j.Logger;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;
import uk.ac.ed.epcc.qcdgrid.metadata.EnsembleNameExtractor;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridLogicalNameExtractor;
import uk.ac.ed.epcc.qcdgrid.metadata.DocumentFormatException;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;


/**
 * Parses the raw results of queries on the database. Extracts the relevant results so
 * that the maxResults, startIndex constraints are satisfied. Gets the LFN/URI relating
 * to each query result for return to the Web Service client.
 * 
 * @author daragh
 * 
 */
public class QueryResultsParser {

  private static Logger logger_ = Logger.getLogger(QueryResultsParser.class);

  
  private QCDgridExistDatabase driver_;

  public QueryResultsParser(QCDgridExistDatabase driver) {
    this.driver_ = driver;
  }

  /**
   * Returns the results of an ensemble query "as-is" without further element
   * extraction
   * 
   * @param rawResults
   * @param startIndex
   * @param maxResults
   * @return
   */
  public org.lqcd.ildg.mdc.QueryResults prepareRawEnsembleURIResultsForReturn(
      uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults rawResults,
      int startIndex, int maxResults) throws ResultsParserException {
    return getResultsAsMarkovChainURIs(rawResults, startIndex, maxResults);
  }

  /**
   * Processes the raw query results. Exctracts the appropriate subset of
   * results to send back to the client depending on the values of startIndex
   * and maxResults. If startIndex &lt; 0, then we assume that all results are
   * required and maxResults is ignored.
   * 
   * @param rawResults
   * @param startIndex
   *          zero-indexed index of where we want to start in the list of
   *          results
   * @param maxResults
   *          The maximum numbeimport org.r of results to be returned
   * @return
   */
  public org.lqcd.ildg.mdc.QueryResults prepareRawConfigLFNResultsForReturn(
      uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults rawResults,
      int startIndex, int maxResults) throws ResultsParserException {
    logger_.debug("Preparing raw configuration results for return");
    return getResultsAsConfigLFNs(rawResults, startIndex, maxResults);
  }

  /**
   * Takes the raw results. For each result: - gets the document to which that
   * result belongs - checks that the document is an ensemble document -
   * extracts the markovChainURI from that document Returns all of the results,
   * wrapped in a QueryResults object.
   * 
   * @param rawResults
   *          The results from the database to be processed
   * @param startIndex
   *          The start index of the results to be returned (0 or greater)
   * @param maxResults
   *          Can be -1, in which case all results are returned, or any number
   *          greater than zero.
   * @return
   */
  private org.lqcd.ildg.mdc.QueryResults getResultsAsMarkovChainURIs(
      uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults rawResults,
      int startIndex, int maxResults) throws ResultsParserException {
    String[] allURIs = null;
    int totalResults = 0;
    if (rawResults == null) {
      logger_.debug("Raw results are null");
      throw new ResultsParserException("Raw results are null.");
    }
    logger_.debug("Parsing " + rawResults.count() + " results.");
    try {
      ResourceSet rs = rawResults.resultsAsXMLDBResourceSet();
      ResourceIterator iter = rs.getIterator();
      EnsembleNameExtractor ne = new EnsembleNameExtractor();
      totalResults = (int) rs.getSize();
      allURIs = new String[totalResults];
      int i = 0;
      logger_.debug("Starting to parse resources.");
      while (iter.hasMoreResources()) {
        XMLResource resource = (XMLResource) iter.nextResource();
        String docId = resource.getDocumentId();
        logger_.debug("Doc id: " + docId);
        String document = driver_.getDocument(docId);
        // logger_.debug("Document is: " + document);
        Document docAsNode = XmlUtils.domFromString(document);
        logger_.debug("Document parsed as DOM successfully.");
        allURIs[i] = ne.getEnsembleNameFromEnsembleDocument(docAsNode);
        logger_.debug("Added URI  " + allURIs[i]);
        i++;
      }
    } catch (XMLDBException xdbe) {
      throw new ResultsParserException(
          "An XMLDBException occurred when parsing " + "query results.");
    } catch (QCDgridException qcde) {
      throw new ResultsParserException("Could not parse results: "
          + qcde.getMessage());
    } catch (DocumentFormatException de) {
      throw new ResultsParserException("Could not parse results: "
          + de.getMessage());
    }
    String[] actualResults = getAppropriateSubArrayAsStrings(allURIs,
        startIndex, maxResults);      
    return createSuccessfulResults(startIndex, totalResults, actualResults);
  }

  /**
   * Takes the raw results. For each result: - gets the document to which that
   * result belongs - checks that the document is a configuration document -
   * extracts the dataLFN from that document Returns all of the results, wrapped
   * in a QueryResults object.
   * 
   * @param rawResults
   * @param startIndex
   * 
   * @param maxIndex
   * @return
   */
  private org.lqcd.ildg.mdc.QueryResults getResultsAsConfigLFNs(
      uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults rawResults,
      int startIndex, int maxResults) throws ResultsParserException {
    String[] allLFNs = null;
    int totalResults = 0;
    if (rawResults == null) {
      logger_.debug("Raw results are null");
      throw new ResultsParserException("Raw results are null.");
    }
    logger_.debug("Parsing " + rawResults.count() + " results.");
    try {
      ResourceSet rs = rawResults.resultsAsXMLDBResourceSet();
      ResourceIterator iter = rs.getIterator();      
      totalResults = (int) rs.getSize();
      allLFNs = new String[totalResults];
      int i = 0;
      logger_.debug("Starting to parse resources.");
      while (iter.hasMoreResources()) {              
        XMLResource resource = (XMLResource) iter.nextResource();
        String docId = resource.getDocumentId();
        logger_.debug("Doc id: " + docId);
        String document = driver_.getDocument(docId);
        logger_.debug("Document is: " + document);
        allLFNs[i] = QCDgridLogicalNameExtractor.getLFNFromMetadataDocument(document); 
        logger_.debug("Added LFN  " + allLFNs[i]);
        i++;
      }
    } catch (XMLDBException xdbe) {
      throw new ResultsParserException(
          "An XMLDBException occurred when parsing " + "query results.");
    } catch (QCDgridException qcde) {
      throw new ResultsParserException("Could not parse results: "
          + qcde.getMessage());
    }
    String[] actualResults = getAppropriateSubArrayAsStrings(allLFNs,
        startIndex, maxResults);    
    return createSuccessfulResults(startIndex, totalResults, actualResults);
  }


  
  /**
   * Takes the raw set of results. Gets a string representation of each 
   * result. Extracts the appropriate sub-array as specified by startIndex, maxResults.
   * 
   * @param rawResults
   * @param startIndex
   * @param maxResults
   * @return
   */
  private String[] getAppropriateSubArrayAsStrings(Object[] rawResults,
      int startIndex, int maxResults) {
    String[] retval = null;
    int totalResults = rawResults.length;
    if ((startIndex + maxResults) > totalResults) { // Overlap between max and
                                                    // actual length
      logger_.debug("maxResults + startIndex > totalResults, allocating "
          + (totalResults - startIndex));
      retval = new String[totalResults - startIndex];
    } else if (maxResults > 0) { // Free to return maxResults entries
      logger_.debug("Returning maxResults (" + maxResults
          + ") entries to client");
      retval = new String[maxResults];
    } else if (maxResults == -1) {
      // Return all
      retval = new String[totalResults];
    }
    for (int i = 0; i < retval.length; i++) {
      retval[i] = rawResults[i + startIndex].toString();
    }
    return retval;
  }
  
  /**
   * Creates and returns a QueryResults object, with code set to MDC_SUCCESS
   * @param startIndex
   * @param totalResults
   * @param actualResults
   * @return
   */
  private org.lqcd.ildg.mdc.QueryResults createSuccessfulResults(
      int startIndex, int totalResults, String[] actualResults){
    // Initialise and return the processed results
    org.lqcd.ildg.mdc.QueryResults results 
    = new org.lqcd.ildg.mdc.QueryResults();
    //for (int j = 0; j < actualResults.length; j++) {
      //logger_.debug("Returning " + actualResults[j]);
    //}
    results.setResults(actualResults);
    results.setStatusCode("MDC_SUCCESS");
    results.setStartIndex(startIndex);
    results.setTotalResults(totalResults);
    results.setQueryTime(timeAsString());
    results.setNumberOfResults(actualResults.length);
    return results;
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
