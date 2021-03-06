/***********************************************************************
*
*   Filename:   ResultInfo.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    A class that describes an individual query result ie.
*               a single record.
*
*   Contents:   ResultInfo class
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

/**
 * A class that stores information about a single QCDgrid query result.
 * 
 * @author daragh
 * 
 */
public class ResultInfo {

  /**
   * Indicates that this result came from the QCDgrid central XML database.
   */
  public static final String QCDGRID_XMLDB_CATALOGUE_SOURCE = "QCDGRID_XMLDB_CATALOGUE";

  /** The identifier for the result */
  private String identifier_;

  /** The source for this result */
  private String source_;

  /**
   * Result identifer is either an XMLDB resource id (in the case of query
   * results that come from the eXist database) or the LFN/URI of the relevant
   * document.
   * 
   * @param resultIdentifier
   * @param source
   */
  public ResultInfo(String resultIdentifier, String source) {
    identifier_ = resultIdentifier;
    source_ = source;
  }

  /**
   * Default source is QCDGRID_XMLDB_CATALOGUE_SOURCE
   * 
   * @param resultIdentifier
   */
  public ResultInfo(String resultIdentifier) {
    this(resultIdentifier, ResultInfo.QCDGRID_XMLDB_CATALOGUE_SOURCE);
  }

  /**
   * Gets the source of the result
   * @return
   */
  public String getResultSource() {
    return source_;
  }
  
  /**
   * Gets the identifer for the result
   * @return
   */
  public String getResultIdentifier() {
    return identifier_;
  }

  
  public String toString(){
    return getResultIdentifier();
  }
  
}
