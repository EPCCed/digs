/***********************************************************************
*
*   Filename:   ILDGQueryResults.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    A subclass of QCDgridQueryResults specifically for ILDG
*               query results.
*
*   Contents:   ILDGQueryResults class.
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

import java.util.ArrayList;
import java.util.List;

/**
 * This is a class that wraps a set of results from multiple 
 * metadata catalogue web service instances.
 */
public class ILDGQueryResults extends QCDgridQueryResults {

  // The actual results.
  private ResultInfo [] results_ = new ResultInfo[0];
  
  /**
   * Creates the queryResults object. queryTypeInfo should be 
   * one of QueryType.ILDG_QUERY_TYPE or QueryType.UKQCD_QUERY_TYPE.
   * @param queryTypeInfo
   */
  public ILDGQueryResults(String queryTypeInfo){    
    super();
    this.setQueryTypeInfo(queryTypeInfo);
  }
  
  /**
   * Provided for compatibility with the base class. Never used.
   * @param rs
   * @param queryTypeInfo
   * @throws Exception
   */
  public ILDGQueryResults(org.xmldb.api.base.ResourceSet rs, String queryTypeInfo) throws Exception{
    this.setQueryTypeInfo(queryTypeInfo);
    throw new Exception( "This is not yet implemented, and likely never will be.");
  }
  
  /**
   * Returns an array of resultInfo objects, which contain information
   * about the results returned.
   */
  public ResultInfo [] getIndividualResultInfoObjects(){
    return results_;
  }
  
  /**
   * All of the results joined up together as a string.
   */
  public String combinedResult(){
    StringBuffer sb = new StringBuffer();
    String [] results = getResults();
    for(int i=0; i<results.length; i++){
      sb.append(results_[i].toString());
    }
    return sb.toString();
  }
  
  /**
   * The results as strings.
   */
  public String[] getResults() {
    String [] ret = new String[results_.length];
    for(int i=0; i<results_.length; i++){
      ret[i] = results_[i].toString();
    }
    return ret;
  }
  
  
  /**
   * Add a set of results to this object. The results are stored in an
   * array; a brute force concatenation is used.
   * @param resultsToAdd
   */
  public void addResults(ResultInfo[] resultsToAdd){
    ResultInfo [] newResults = new ResultInfo[results_.length + resultsToAdd.length];
    int i=0;
    for(i=0; i<results_.length;i++){
      newResults[i] = results_[i];
    }
    int j=0;
    for(;i<newResults.length;i++,j++){
      newResults[i]=resultsToAdd[j];
    }
    results_ = newResults;
    //System.out.println("There are now " + results_.length + " results."  );
  }
  
  
  /**
   * Returns the number of results stored by this object.
   */
  public int count(){
    return results_.length;
  }
  
}
