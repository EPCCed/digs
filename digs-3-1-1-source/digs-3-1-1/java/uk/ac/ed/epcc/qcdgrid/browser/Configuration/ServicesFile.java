/***********************************************************************
*
*   Filename:   ServicesFile.java
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    A class that parses an ILDG services file given its location.
*
*   Contents:   ServicesFile class
*
*   Used in:    All QCDgrid-related Java code
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


package uk.ac.ed.epcc.qcdgrid.browser.Configuration;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import java.io.File;
import java.util.ArrayList;
import java.util.List;


/**
 * This class parses an ILDG services file (see http://www.lqcd.org/ildg/Services.xml)
 * and provides several accessors that allow access to the extracted information.
 * 
 * @author daragh@epcc
 */
public class ServicesFile {
  
  // The tag name for the element that specifies the Metadata Catalogue 
  // location within the services file. 
  private static final String METADATA_CATALOGUE_TAG_NAME = "mdc";
  
  // A list of all the metadata catalogue service locations.
  private List metadataCatalogueServiceLocationList_  = new ArrayList();
  
  
  /**
   * Parses the services file and sets the information. This could be extended in 
   * the future:
   *   - Parse the Replica Catalogue information
   *   - Populate a list of CollaborationInfo objects instead of simple strings
   *   - etc.
   * @param servicesFileLocation The location on the filesystem of the 
   * services file.
   * @throws QCDgridException
   */
  public ServicesFile(String servicesFileLocation) throws QCDgridException {
    Document servicesDoc = XmlUtils.domFromFile(new File(servicesFileLocation));
    ////System.out.println("Doc: " + servicesDoc);
    NodeList nl = servicesDoc.getElementsByTagName(ServicesFile.METADATA_CATALOGUE_TAG_NAME);
    for(int i=0; i<nl.getLength(); i++){
      String location = extractMCLocation(nl.item(i));
      this.metadataCatalogueServiceLocationList_.add(location);
    }
  }
  
  
  
  /**
   * Returns the list of metadata catalogue service locations stored
   * in the services file
   * @return The list of metadata catalogue service locations stored
   * in the services file
   */
  public List getMetadataCatalogueServiceList(){
    return metadataCatalogueServiceLocationList_;
  }

  
  // Utility to get the node value.
  private String extractMCLocation(Node n){
    String ret = n.getChildNodes().item(0).getNodeValue();
    return ret;
  }
  
   
}
