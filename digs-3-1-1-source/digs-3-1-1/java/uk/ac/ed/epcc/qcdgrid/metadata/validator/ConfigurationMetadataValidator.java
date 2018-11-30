/***********************************************************************
 *
 *   Filename:   ConfigurationMetadataValidator.java
 *
 *   Authors:    Daragh Byrne           (daragh)   EPCC.
 *
 *   Purpose:    Used to validate configuration metadata documents.
 *
 *   Contents:   ConfigurationMetadataValidator
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
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.SchemaInfo;

import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.FileWriter;
import java.io.IOException;
import javax.xml.parsers.ParserConfigurationException;
import org.xml.sax.SAXException;

import java.util.Hashtable;

import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.Element;

import org.xmldb.api.base.XMLDBException;

/**
 * Ensures that Configuration metadata documents are allowed to be submitted to
 * the metadata catalogues. Validity criteria are: 1) The document is valid
 * against the schema. 2) The configuration LFN in the dataLFN element is not
 * already in use by a document stored in the catalogue 3) The value of the
 * markovChainURI element is that of an ENSEMBLE metadata document that is
 * ALREADY stored in the catalogue.
 */
public class ConfigurationMetadataValidator extends MetadataValidator {

  public static final String MSG_ENSEMBLE_ID_INVALID = "The ensemble ID in this document is invalid.";

  public static final String MSG_CONFIGURATION_ID_INVALID = "A configuration using the Logical File Name "
      + "is already present in the database. "
      + "\nPlease use another Logical File Name.";

  private EnsembleNameExtractor ensExtractor_ = new EnsembleNameExtractor();

  /**
   * Provide this with a database URL of the form hostname:port
   */
  public ConfigurationMetadataValidator(String databaseURL)
      throws QCDgridException {
    super(databaseURL);
    downloadSchemaFile(getMetadataClient().getSchemaInfo().getConfigurationSchemaFileName()); 
  }
  
  public ConfigurationMetadataValidator(QCDgridMetadataClient mdClient)
    throws QCDgridException {
    super(mdClient);
    downloadSchemaFile(getMetadataClient().getSchemaInfo().getConfigurationSchemaFileName());    
  }
  
  
  public ConfigurationMetadataValidator(File schemaFile){
    super(schemaFile);
    //No more initialisation required here, the schema already exists.
  }


  /**
   * When a CMD containing the same logical file name as the document being
   * tested is contained in the database, a ValidationException is thrown. The
   * schema validation is carried out by the superclass. The Ensemble ID
   * contained in the document is also extracted. If an Ensemble Metadata
   * document with that identifier is NOT in the database, a ValidationException
   * is thrown.
   * 
   * Possible refactor: LogicalNameExtractor to use DOM and remove the File
   * parameter - applies to both validators.
   */
  protected void validateImpl(Document document, File docFile)
      throws ValidationException, IOException, QCDgridException {
    String docAsString = docAsString(docFile);
    // System.out.println("ConfigMDValidator::validateImpl");
    String lfn = QCDgridLogicalNameExtractor
        .getLFNFromMetadataDocument(docAsString);
    if (isFileInDB(lfn)) {
      throw new ValidationException(
          ConfigurationMetadataValidator.MSG_CONFIGURATION_ID_INVALID);
    }
    if (!ensembleIdIsValid(document)) {
      throw new ValidationException(
          ConfigurationMetadataValidator.MSG_ENSEMBLE_ID_INVALID);
    }
  }

  /**
   * Determines if a file with that LFN is in the MetadataCatalogue already.
   */
  private boolean isFileInDB(String lfn) throws ValidationException,
      QCDgridException {
    boolean inDB = getMetadataClient().isLFNRepresentedInMDC(lfn);
    return inDB;
  }

  /**
   * In order for the metadata document to be valid, it must reference an
   * ensemble (markovChainURI element) for which an ensemble metadata document
   * exists in the metadata catalogue.
   */
  private boolean ensembleIdIsValid(Document document) throws QCDgridException {
    String ensID = null;
    boolean result = false;
    try {
      ensID = ensExtractor_.getEnsembleNameFromConfigurationDocument(document);
      result = getMetadataClient().isEnsembleRepresentedInMDC(ensID);
    } catch (DocumentFormatException dfe) {
      throw new QCDgridException(
          "Could not get an ensemble ID from the metadata document: "
              + dfe.getMessage());
    }
    return result;
  }
  
  
  

}
