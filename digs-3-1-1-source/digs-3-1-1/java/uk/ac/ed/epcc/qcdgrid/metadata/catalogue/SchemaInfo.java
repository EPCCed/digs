
/***********************************************************************
*
*   Filename:   SchemaInfo.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    Submitting data and metadata to the grid in a single
*               operation
*
*   Contents:   SchemaInfo class
*
*   Used in:    Java client tools
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003 The University of Edinburgh
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

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXParseException;

import uk.ac.ed.epcc.qcdgrid.XmlUtils;
import uk.ac.ed.epcc.qcdgrid.metadata.DocumentFormatException;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryTypeManager;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;

/**
 * Provides information about the schema associated with particular query
 * types.
 * @author daragh
 *
 */
public class SchemaInfo {
 
  public final static String SchemaInfoFileName = "QueryTypes.xml";
  
  /**
   * Name of the file containing the current configuration schema.
   */
  private String configurationSchemaFileName_ = null;

  /**
   * Name of the file containing the current ensemble schema.
   */
  private String ensembleSchemaFileName_ = null;
  
  /**
   * Uses the existing behaviour in the QueryTypesManager to figure out the
   * names of the schema files used for each query type. This depends on the
   * existance of a file called QueryTypes.xml in the repository.
   */
  
  public SchemaInfo( QCDgridMetadataClient client ) throws DocumentFormatException,
     QCDgridException {        
    QueryTypeManager mgr = new QueryTypeManager();
    QueryType ensType = mgr.queryTypeFromName("ensemble");
    ensembleSchemaFileName_ = ensType.getSchemaName();
    QueryType cfgType = mgr.queryTypeFromName("gaugeconfig");
    configurationSchemaFileName_ = cfgType.getSchemaName();        
  }
  
  /**
   * Returns the name of the schema in the database to use for validating
   * ensemble metadata documents.
   * @return
   */
  public String getEnsembleSchemaFileName(){
    return ensembleSchemaFileName_;
  }
  
  
  /**
   * Returns the name of the schema in the database to use for validating
   * ensemble metadata documents.
   * @return
   */
  public String getConfigurationSchemaFileName(){
    return configurationSchemaFileName_; 
  }
  
  
  
  
}
