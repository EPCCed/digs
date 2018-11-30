/***********************************************************************
 *
 *   Filename:   EnsembleMetadataValidator.java
 *
 *   Authors:    Daragh Byrne          (daragh)   EPCC.
 *
 *   Purpose:    Validates Ensemble Metadata Documents, ensuring they
 *               are OK to be placed in the Metadata Catalogue.
 *
 *   Contents:   EnsembleMetadataValidator
 *
 *   Used in:    Metadata handling tools
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

package uk.ac.ed.epcc.qcdgrid.metadata.validator;

import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridLogicalNameExtractor;
import uk.ac.ed.epcc.qcdgrid.metadata.EnsembleNameExtractor;
import uk.ac.ed.epcc.qcdgrid.metadata.DocumentFormatException;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;

import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.IOException;
import org.xml.sax.SAXException;

import java.util.Hashtable;

import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.Element;

import org.xmldb.api.base.XMLDBException;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

/**
 * Validates Ensemble Metadata Documents, ensuring they are OK to be placed in
 * the Metadata Catalogue. Conditions for validity are: 1) A document with the
 * same value of the markovChainURI element is NOT already in the metadata
 * catalogue.
 * 
 * 2) TODO - at present we don't validate against the schema as it is broken.
 * 
 */
public class EnsembleMetadataValidator extends MetadataValidator {

  /** The message when the ensemble ID (markovChainURI) is already in use */
  public static final String MSG_ENSEMBLE_ID_INVALID = "The ensemble identifier you have chosen is"
      + " already in use. Please choose another.";

  private EnsembleNameExtractor extractor_ = new EnsembleNameExtractor();

  /**
   * 
   */
  public EnsembleMetadataValidator(String databaseURL)
      throws QCDgridException {
    super(databaseURL);
    downloadSchemaFile(getMetadataClient().getSchemaInfo().getEnsembleSchemaFileName()); 
  }
  
  public EnsembleMetadataValidator(QCDgridMetadataClient mdClient)
    throws QCDgridException {
    super(mdClient);
    downloadSchemaFile(getMetadataClient().getSchemaInfo().getEnsembleSchemaFileName());    
  }
  
  
  public EnsembleMetadataValidator(File schemaFile){
    super(schemaFile);
    //No more initialisation required here, the schema already exists.
  }

  /**
   * Checks to see if the ensemble listed in the document is already present in
   * the database. If it is, an exception is raised - when submitting, a unique
   * ID should be used.
   */
  public void validateImpl(Document doc, File docAsFile)
      throws ValidationException, IOException, QCDgridException {
    String ensembleName;
    boolean ensembleIsInDB;
    try {
      // This is temporary schema type validatation
      if (doc.getDocumentElement().getTagName() != "markovChain") {
        throw new DocumentFormatException("Document format problem: "
            + "The root element of the ensemble document is NOT markovChain.");
      }
      ensembleName = extractor_.getEnsembleNameFromEnsembleDocument(doc);
      ensembleIsInDB = getMetadataClient().isEnsembleRepresentedInMDC(
          ensembleName);
    } catch (DocumentFormatException dfe) {
      throw new QCDgridException(
          "Could not get an ensemble ID from the metadata document: "
              + dfe.getMessage());
    }
    if (ensembleIsInDB) {
      throw new ValidationException(
          EnsembleMetadataValidator.MSG_ENSEMBLE_ID_INVALID);
    }
  }

  /**
   * FIXME:TODO: At present, the QCDML 1.1 ensemble schema is broken. We need to
   * turn off validation against the schema for now - hence this override.
   *
  protected Document createValidDOM(File document) throws ValidationException,
      ParserConfigurationException, SAXException, IOException {

    Document validDocument = null;
    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    factory.setValidating(false);
    factory.setNamespaceAware(true);
    DocumentBuilder builder = factory.newDocumentBuilder();
    SchemaValidationErrorHandler handler = new MetadataValidator.SchemaValidationErrorHandler();
    builder.setErrorHandler(handler);
    validDocument = builder.parse(document);
    if (handler.getError() != null) {
      throw new ValidationException(
          "There was an error validating the metadata document against"
              + " the schema: " + handler.getError().getMessage());
    }
    *
    return validDocument;

  }*/

}
