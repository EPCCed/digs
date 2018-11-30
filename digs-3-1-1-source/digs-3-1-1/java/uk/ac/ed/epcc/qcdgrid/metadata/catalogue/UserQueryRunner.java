/***********************************************************************
*
*   Filename:   UserQueryRunner.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    This interface should be implemented by any classes wishing
*               to execute Xpath queries generated by the user.
*
*   Contents:   UserQueryRunner interface
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

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Any class that wishes to execute user queries against some back end 
 * (e.g. an XML database, a web service or set of web services) should 
 * implement this interface.  
 * @author daragh
 *
 */
public interface UserQueryRunner {
  
  /**
   * Runs an XPath query that is expected to return configuration related
   * metadata.
   * @param XPathQuery
   * @return
   */
  public QCDgridQueryResults runConfigurationQuery(String XPathQuery) throws QueryExecutionException;
  
  /**
   * Runs an XPath query that is expected to return ensemble related
   * metadata.
   * @param XPathQuery
   * @return
   */
  public QCDgridQueryResults runEnsembleQuery(String XPathQuery) throws QueryExecutionException;

  /**
   * Runs an XPath query that is expected to return correlator related
   * metadata.
   * @param XPathQuery
   * @return
   */
  public QCDgridQueryResults runCorrelatorQuery(String XPathQuery) throws QueryExecutionException;

}