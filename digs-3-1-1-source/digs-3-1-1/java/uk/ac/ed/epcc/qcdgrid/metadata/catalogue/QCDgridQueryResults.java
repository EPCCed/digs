/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 * 
 * Revisions in 2006 to cope with the new ILDG browser mode.
 */
package uk.ac.ed.epcc.qcdgrid.metadata.catalogue;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import org.xmldb.api.base.*;
import org.xmldb.api.modules.XMLResource;

/**
 * Title: QCDgridQueryResults Description: Encapsulates the results of a query
 * run on the database Copyright: Copyright (c) 2003 Company: The University of
 * Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class QCDgridQueryResults {

  /** The results as one single string */
  private String results_;

  /** The results as an XML:DB result set */
  private ResourceSet resultSet;

  /** Some information about the type of the query eg ensemble, configuration */
  private String queryTypeInfo_ = null;

  /**
   * Constructor which takes ResourceSet, and some information about the type of
   * query that was issued.
   */
  public QCDgridQueryResults(ResourceSet rs, String queryTypeInfo) {
    resultSet = rs;
    queryTypeInfo_ = queryTypeInfo;
  }

  public QCDgridQueryResults(){
    resultSet = null;
    queryTypeInfo_ = "";
  }
  
  /**
   * @return the number of documents returned
   */
  public int count() {
    int c = -1;

    if (resultSet != null) {
      try {
        c = (int) resultSet.getSize();
      } catch (Exception e) {
        System.err.println("Exception: " + e);
      }
    } else {
      // not implemented yet
    }

    return c;
  }

  /**
   * @return the XML:DB resource set containing the results (if there is one)
   */
  public ResourceSet resultsAsXMLDBResourceSet() {
    if (resultSet != null) {
      return resultSet;
    } else {
      return null;
    }
  }

  /**
   * Returns an array of ResultInfo objects, each of which contain info
   * about a single query result. The information includes:
   *  - The source of the result
   *  - The XMLDB resource id for the result.
   * This could take some time with large result sets.
   * @return
   * @throws QCDgridException
   */
  public ResultInfo[] getIndividualResultInfoObjects() throws QCDgridException {

    ResultInfo[] ret = new ResultInfo[count()];
    try {
      ResultInfo result;
      // For each result, get its name and add that to the list model
      for (int i = 0; i < count(); i++) {
        Resource res = resultSet.getResource(i);
        XMLResource xres = (XMLResource) res;
        result = new ResultInfo( xres.getDocumentId() );
        ret[i] = result;
      }
    } catch (XMLDBException xdbe) {
      throw new QCDgridException(
          "Could not load identifying strings in QCDgridQueryResults: "
              + xdbe.getMessage());
    }
    return ret;
  }

  /**
   * @return a string containing all the results combined
   */
  public String combinedResult() {
    if (resultSet != null) {
      String res = "";
      try {
        for (int i = 0; i < resultSet.getSize(); i++) {
          Resource r = resultSet.getResource(i);
          res += (String) r.getContent();
        }
      } catch (Exception e) {
        System.err.println("Exception: " + e);
      }
      return res;
    } else {
      return results_;
    }
  }

  public String[] getResults() {
    String[] results = null;
    try {
      results = new String[(int) resultSet.getSize()];
      for (int i = 0; i < resultSet.getSize(); i++) {
        Resource r = resultSet.getResource(i);
        results[i] = (String) r.getContent();
      }
    } catch (XMLDBException xdbe) {
      System.out.println(xdbe.getMessage());
    }

    return results;
  }

  public String getQueryTypeInfo() {
    return this.queryTypeInfo_;
  }

  public void setQueryTypeInfo(String queryTypeInfo){
    queryTypeInfo_ = queryTypeInfo;
  }
  
  public String toString() {
    return count() + " " + combinedResult();
  }
}
