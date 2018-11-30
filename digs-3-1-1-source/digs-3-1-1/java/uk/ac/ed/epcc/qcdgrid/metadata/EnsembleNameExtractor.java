/***********************************************************************
 *
 *   Filename:   EnsembleNameExtractor.java
 *
 *   Authors:    Daragh Byrne           (daragh)   EPCC.
 *
 *   Purpose:    Extracts ensemble names from both configuration and
 *               ensemble metadata documents.
 *
 *   Content:    EnsembleNameExtractor class
 *
 *   Used in:    Java client tools
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

package uk.ac.ed.epcc.qcdgrid.metadata;

import uk.ac.ed.epcc.qcdgrid.metadata.DocumentFormatException;

import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.Element;

import uk.ac.ed.epcc.qcdgrid.XmlUtils;

/**
 * DOM-based class for extracting ensemble IDs from metadata documents. There
 * are two separate methods as, even though the element name is the same when
 * dealing with both document types, this could change in later versions of the
 * schema.
 */
public class EnsembleNameExtractor {

  /**
   * Extracts the value of the markovChainURI element from an Ensemble Metadata
   * Document.
   */
  public String getEnsembleNameFromEnsembleDocument(Document ensDoc)
      throws DocumentFormatException {
    NodeList list = ensDoc.getElementsByTagName("markovChainURI");
    if (list.getLength() == 0 || list == null) {
      // To cope with earlier schema versions
      list = ensDoc.getElementsByTagName("markovChainLFN");
      if (list.getLength() == 0 || list == null) {
        throw new DocumentFormatException(
            "Could not find element markovChainURI when extracting"
                + " ensemble name.");
      }
    }
    return getNodeValue(list).trim();
  }

  public String getEnsembleNameFromEnsembleDocument(String document)
      throws DocumentFormatException {
    Document docAsDom = XmlUtils.domFromString(document);
    return getEnsembleNameFromEnsembleDocument(docAsDom);
  }

  /**
   * Extracts the value of the markovChainURI element from a configuration
   * Metadata Document.
   */
  public String getEnsembleNameFromConfigurationDocument(Document configDoc)
      throws DocumentFormatException {

    NodeList list = configDoc.getElementsByTagName("markovChainURI");
    if (list.getLength() == 0 || list == null) {
      throw new DocumentFormatException(
          "Could not find element markovChainURI when extracting"
              + " ensemble name.");
    }
    return getNodeValue(list).trim();

  }

  private String getNodeValue(NodeList list) throws DocumentFormatException {
    list = list.item(0).getChildNodes();
    if (list.getLength() != 1) {
      throw new DocumentFormatException("Too many node children in"
          + " getEnsembleNameFromEnsembleDocument");
    }
    String val = (String) list.item(0).getNodeValue();
    return val;
  }

}
