/***********************************************************************
 *
 *   Filename:   QCDgridMetadataClient.java
 *
 *   Authors:    James Perry            (jamesp)   EPCC.
 *
 *   Purpose:    Provides a command line tool and Java API for performing
 *               common operations on the QCDgrid metadata catalogue
 *
 *   Contents:   QCDgridMetadataClient class
 *
 *   Used in:    Java client tools
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2003, 2005 The University of Edinburgh
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

import java.io.*;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

import uk.ac.ed.epcc.qcdgrid.*;
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.SchemaInfo;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QueryExecutionException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.MetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ConfigurationMetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.EnsembleMetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.DirectoryValidationException;

/**
 * Abstracts all access to the metadata catalogue, allowing the user to add,
 * remove and query the existence of metadata documents. Operates as a command
 * line client to allow document addition etc to be scripted.
 */
public class QCDgridMetadataClient {
  /** Reference to the collection being manipulated */
  private Collection collection;

  /** DB driver */
  private QCDgridExistDatabase driver_;
  
  /** Schema version information */
  private SchemaInfo schemaInfo_;
  
  /** Validators */
  private ConfigurationMetadataValidator configValidator_;

  private EnsembleMetadataValidator ensembleValidator_;
  
  /** 
   * The name of the file in the DB that contains info about the schemas 
   * used for validating Ensemble and Configuration metadata documents.
   */
  private static final String schemaInfoFile_ = "SchemaMappings.xml";

  /**
   * Generates a database name for a configuration or ensemble based on its
   * identifier. Substitutes /, \, and : with _ If this is used after
   * validation, the name is guaranteed to be unique in the catalogue.
   */
  private String lfnToCatalogueName(String lfn) {
    return (lfn.replace('/', '_').replace('\\', '_').replace(':', '_'))
        + ".xml";
  }

  /**
   * Constructs a new client interface to the Exist database.
   * Obtains information about the schema to be used in the validation 
   * process.
   * 
   * @param address
   *          the FQDN, port and path of the database e.g. "localhost:8080"
   */
  public QCDgridMetadataClient(String address) throws QCDgridException {
    try {
      FeatureConfiguration fc = new FeatureConfiguration();
      driver_ = new QCDgridExistDatabase();
      driver_.init(address, fc.getXMLDBUserName(), fc.getXMLDBPassword());
      collection = driver_.getCollection();      
      schemaInfo_ = new SchemaInfo(this);
      configValidator_ = new ConfigurationMetadataValidator(this);
      ensembleValidator_ = new EnsembleMetadataValidator(this);
    } catch (XMLDBException xdbe) {
      throw new QCDgridException("Could not connect to XML database server.");
    }
    catch (DocumentFormatException dfe){
      // This means that the schema information file used is invalid or empty
      throw new QCDgridException("There was an internal configuration error. The"
          + " schema information file used is invalid. Please inform the "
          + " system administrators.");      
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      throw new QCDgridException("Could not read browser.properties, please check.");
    }
   // System.out.println("Opened connection to xmldb on " + address);
  }

  /**
   * Submits a locally stored configuration XML file to the metadata catalogue,
   * validating it first.
   * 
   * @param localfile
   *          the local XML file to submit
   * @param dbname
   *          the name to call it by in the catalogue
   * @throws ValidationException
   *           If the file is not valid for putting on the grid.
   */
  public void submitConfigMetadataFile(File localfile) throws QCDgridException,
      IOException, ValidationException {
    // Check arguments and ensure file is valid for submission
    if (localfile == null)
      throw new IllegalArgumentException("localfile null");
    // if( dbname == null ) throw new IllegalArgumentException( "dbname null" );
    configValidator_.validateMetadataDocument(localfile);
    String dbname = lfnToCatalogueName(QCDgridLogicalNameExtractor
        .getLFNFromMetadataDocument(localfile));

    // System.out.println( "Storing MD file as " + dbname );
    doFileSubmission(localfile, dbname);
  }

  /**
   * Submits a locally stored ensemble XML file to the metadata catalogue,
   * validating it first.
   * 
   * @param localfile
   *          the local XML file to submit
   * @param dbname
   *          the name to call it by in the catalogue
   * @throws ValidationException
   *           If the file is not valid for putting on the grid.
   */
  public void submitEnsembleMetadataFile(File localfile)
      throws QCDgridException, IOException, ValidationException,
      DocumentFormatException {
    // Check arguments and ensure file is valid for submission
    if (localfile == null)
      throw new IllegalArgumentException("localfile null");
    // if( dbname == null ) throw new IllegalArgumentException( "dbname null" );
    ensembleValidator_.validateMetadataDocument(localfile);
    String dbname = new EnsembleNameExtractor()
        .getEnsembleNameFromConfigurationDocument(XmlUtils
            .domFromFile(localfile));
    dbname = lfnToCatalogueName(dbname);
    // System.out.println( "Storing emd file as " + dbname );
    doFileSubmission(localfile, dbname);
  }

  /**
   * This is a convenience method for the QCDgridSubmitter class, which carries
   * out validation when submitting directories. This is deliberately package
   * (default) level protection to discourage use; this class does not validate
   * metadata documents!
   */
  void submitPreviouslyValidatedMetadataFile(File localfile, String dbname)
      throws QCDgridException, IOException {
    if (localfile == null)
      throw new IllegalArgumentException("localfile null");
    if (dbname == null)
      throw new IllegalArgumentException("dbname null");
    doFileSubmission(localfile, dbname);
  }

  /**
   * Does the actual submission. Assumes that localfile has been validated.
   */
  private void doFileSubmission(File localfile, String dbname)
      throws QCDgridException, IOException {
    byte[] data = new byte[(int) localfile.length()];
    try {
      FileInputStream is = new FileInputStream(localfile);
      is.read(data);
      String content = new String(data);
      Resource res = collection.createResource(dbname, "XMLResource");
      res.setContent(content);
      collection.storeResource(res);
    } catch (FileNotFoundException fnfe) {
      throw new IllegalArgumentException(
          "File not found by QCDgridMetadataClient.");
    } catch (XMLDBException xde) {
      throw new QCDgridException(
          "There was a problem interacting with the metadata catalogue: "
              + xde.getMessage());
    } catch (IOException ioe) {
      throw ioe;
    }
  }

  /**
   * Removes an XML document from the metadata catalogue
   * 
   * @param dbname
   *          the name of the document in the database
   */
  public void removeFile(String dbname) throws Exception {
    Resource res = collection.getResource(dbname);
    if (res != null) {
      collection.removeResource(res);
    }
  }

  /**
   * Updates the schema stored in the metadata catalogue
   * 
   * @param newSchema
   *          local file containing the new schema
   */
  public void updateSchema(File newSchema) throws Exception {
    throw new Exception("not implemented at present");
    /*
     * byte[] data = new byte[(int)newSchema.length()]; FileInputStream is = new
     * FileInputStream(newSchema); is.read(data); String content = new
     * String(data);
     * 
     * Resource res = collection.getResource("schema.xml");
     * res.setContent(content); collection.storeResource(res);
     */
  }

  /**
   * Checks whether a metadata about a configuration with a particular Logical
   * File Name is stored in the database
   * 
   * @param lfn
   *          The logical file name of interest.
   */
  public boolean isLFNRepresentedInMDC(String lfn) throws QCDgridException {
    boolean result = false;
    try {
      QCDgridQueryResults queryResults = driver_.runConfigurationQuery("/gaugeConfiguration/markovStep/dataLFN[string(.) = '"
          + lfn + "']");
      if (queryResults.count() > 0) {
        result = true;
      }
      } catch (QueryExecutionException xdbe) {
	  throw new QCDgridException(
	      "A database query (configuration) could not be run.");
      }
    return result;
  }

  /**
   * Checks whether a document representing an ensemble with a particular
   * markovChain already exists in the database.
   * 
   * @param markovChainURI
   *          The URI that uniquely identifies the ensemble.
   */
  public boolean isEnsembleRepresentedInMDC(String markovChainURI)
      throws QCDgridException {
    boolean result = false;
    try {
      String query = "/markovChain/markovChainURI[string(.) = '" + markovChainURI + "']";
      //System.out.println( query );
      QCDgridQueryResults queryResults = driver_.runEnsembleQuery(query);
      if (queryResults.count() > 0) {
        result = true;
      }
    } catch (QueryExecutionException xdbe) {
	throw new QCDgridException(
	    "A database query (ensemble) could not be run.");
    }
    return result;
  }

  /**
   * Removes a file from the metadata database, given the logical grid filename
   * it contains
   * 
   * @param lfn
   *          logical grid filename
   */
  public void removeFileByLogicalFileName(String lfn) throws Exception {
    XPathQueryService service = (XPathQueryService) collection.getService(
        "XPathQueryService", "1.0");

    // FIXME: 'normalize-space' would be better than 'contains' here but eXist
    // doesn't seem to support it currently.
    // Maybe a regular expression could do the same?
    ResourceSet resultSet = service.query("//pointer_to_value[contains(., '"
        + lfn + "')]");

    if (resultSet.getSize() != 1) {
      System.err
          .println("Warning: more than one metadata document with logical filename "
              + lfn);
    }

    Resource res = resultSet.getResource(0);
    XMLResource xres = (XMLResource) res;

    collection.removeResource(collection.getResource(xres.getDocumentId()));
  }

  /**
   * Gets a schema file using its filename as an index.
   */
  public String getSchema(String schemaName) throws QCDgridException {
    String result = "";
    try {
      result = driver_.getSchema(schemaName);
    } catch (XMLDBException xdbe) {
      throw new QCDgridException("Database error when trying to load schema "
          + schemaName);
    }
    return result;
  }

  /**
   * Gets a named document from the catalogue.
   * @param docName The name of the document to return.
   * @return The document in question as a string.
   */
  public String getDocument(String docName) throws QCDgridException{
    String result = "";
    result = driver_.getDocument(docName);
    return result;
  }
  
  public SchemaInfo getSchemaInfo(){
    return schemaInfo_;
  }
  
}
