/***********************************************************************
 *
 *   Filename:   MetadataValidator.java
 *
 *   Authors:    Daragh Byrne          (daragh)   EPCC.
 *
 *   Purpose:    This is the base class for a set of classes that 
 *               ensure metadata is valid before it is sent to the
 *               metadata catalogue. This class contains methods
 *               that validate documents against a schema.
 *
 *   Contents:   MetadataValidator
 *
 *   Used in:    Metadata handling tools
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

package uk.ac.ed.epcc.qcdgrid.metadata.validator;

import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;

import java.io.FileWriter;
import java.io.InputStream;
import java.io.File;
import java.io.IOException;
import java.io.FileReader;
import java.io.BufferedReader;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.xml.sax.SAXException;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXParseException;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

/**
 * This is the base class for a set of classes that ensure metadata is valid
 * before it is sent to the metadata catalogue. This class contains method that
 * validate documents against a schema.
 */
public abstract class MetadataValidator {

  // We store the schema in some file.
  // It is up to the subclasses how this is obtained. It must
  // be initialised before createValidDOM is called.
  protected File schemaFile_ = null;
  
  private QCDgridMetadataClient metadataClient_ = null;
  
  public MetadataValidator(QCDgridMetadataClient cl){
    metadataClient_ = cl;    
  }
  
  public MetadataValidator(String databaseUrl) throws QCDgridException{
    metadataClient_ = new QCDgridMetadataClient(databaseUrl);    
  }
  
  public MetadataValidator(File schemaFile){
    schemaFile_ = schemaFile;
  }
  
  protected QCDgridMetadataClient getMetadataClient(){
    return metadataClient_;
  }

  /**
   * Downloads the correct schema file and saves it for later use in validation.
   * Only needs to be done once. Could optimise to check for existence of the file
   * but not necessary just now.
   */
  protected void downloadSchemaFile(String schemaFileName) throws QCDgridException{  
    //System.out.println("Downloading schema file " + schemaFileName);
    String schemaFileContents = getMetadataClient().getDocument(schemaFileName);    
    if(schemaFileContents == null || schemaFileContents == null){
      throw new QCDgridException("There was a configuration error. The expected schema file "
          + schemaFileName + " was empty. Please contact an administrator." );
    }
    String schemaFileNameFull = writeSchemaFile(schemaFileName, schemaFileContents);
    this.schemaFile_ = new File(schemaFileNameFull);
  }

  /**
   * Writes the schema file to the home directory and returns its location.
   * @param fileName
   * @param contents
   * @return
   * @throws QCDgridException
   */
  protected String writeSchemaFile(String fileName, String contents) 
  throws QCDgridException{    
  String schemaFileFullName = "";
  try{
    schemaFileFullName = System.getProperty("user.home")+ "/" + fileName;    
 //   System.out.println("Writing schema to " + schemaFileFullName);
 //   System.out.println("Contents are: " + contents);
    FileWriter wr = new FileWriter(schemaFileFullName);
    wr.write(contents);
    wr.close();
  }catch(IOException ioe){
    throw new QCDgridException("There was a problem when trying to create the "
        +"schema file " + schemaFileFullName);
  }
  return schemaFileFullName;
}
  
  /**
   * This methods accepts File objects which are assumed to be XML documents
   * which are valid against a particular schema. The method creates a valid DOM
   * object using the method createValidDOM. This is then passed to the abstract
   * validateImpl where appropriate validation is performed by subclasses.
   * 
   * @param document
   *          The document to be validated. This method assumes that the file
   *          exists and is readable; to use otherwise is a breach of contract.
   * @throws IOException
   *           When there is a problem with the document parameter
   * @throws ValidationException
   *           When the document is invalid with respect to the schema, or
   *           already exists in the catalogue.
   */
  public final void validateMetadataDocument(File document)
      throws ValidationException, IOException, QCDgridException {
    // We recieve a File and need to convert it into a DOM. We can check that
    // it's valid against the schema at the same time.
    Document metadataDOM = null;
    try {
      metadataDOM = createValidDOM(document);
    } catch (ParserConfigurationException pce) {
      throw new ValidationException(
          "Validating the metadata failed due to an XML parser"
              + " configuration problem: " + pce.getMessage());
    } catch (SAXException se) {
      throw new ValidationException(
          "Validating the metadata failed due to a parsing " + "problem: "
              + se.getMessage());
    } catch (IOException ioe) {
      throw new ValidationException(
          "Validating the metadata failed due to an IO problem: "
              + ioe.getMessage());
    }
    // Further validation is carried out in the implementation of this
    // method in subclasses.
    if (metadataDOM == null) {
      throw new ValidationException("metadataDOM is null");
    }
    // System.out.println( "Validated document against schema.\n" );
    validateImpl(metadataDOM, document);
  }


  /**
   * Implemented in subclasses for specific types of MD documents.
   * 
   * @param document
   *          The document to be validated as a DOM.
   * @param docFile
   *          The file from which the DOM has been loaded.
   */
  protected abstract void validateImpl(Document document, File docFile)
      throws ValidationException, IOException, QCDgridException;

  /*
   * A fairly simple error handler which prints stores the root exception for
   * further processing.
   */
  protected class SchemaValidationErrorHandler implements ErrorHandler {
    public void error(SAXParseException exception) {
      // System.out.println("Error creating metadataDOM object: "+
      // exception.getMessage());
      cause_ = exception;
    }

    public void fatalError(SAXParseException exception) {
      // System.out.println("FatalError creating metadataDOM object "+
      // exception.getMessage());
      cause_ = exception;
    }

    public void warning(SAXParseException exception) {
      // System.out.println("Warning while creating metadataDOM object: "+
      // exception.getMessage());
      cause_ = exception;
    }

    SAXParseException cause_ = null;

    public Exception getError() {
      return cause_;
    }
  }

  /**
   * Contract: This takes a file object, that is assumed to point to an existing
   * file, containing the Metadata document to be validated. The document is
   * validated against whatever schema is stored in the schemaInputStream_ field -
   * usually this is set in the subclasses.
   * 
   * @param document
   *          A file that contains the document to be validated Contract: This
   *          takes a file object, that is assumed to point to an existing file,
   *          containing an XML document that is valid against the schema
   *          contained in this.schemaFile_;
   */
  protected Document createValidDOM(File document) throws ValidationException,
      ParserConfigurationException, SAXException, IOException {
    Document validDocument = null;
    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    factory.setValidating(true);
    factory.setNamespaceAware(true);
    factory.setAttribute(
        "http://java.sun.com/xml/jaxp/properties/schemaLanguage",
        "http://www.w3.org/2001/XMLSchema");
    factory.setAttribute(
        "http://java.sun.com/xml/jaxp/properties/schemaSource", schemaFile_);
    DocumentBuilder builder = factory.newDocumentBuilder();
    SchemaValidationErrorHandler handler = new SchemaValidationErrorHandler();
    builder.setErrorHandler(handler);
    validDocument = builder.parse(document);
    if (handler.getError() != null) {
      throw new ValidationException(
          "There was an error validating the metadata document against"
              + " the schema: " + handler.getError().getMessage());
    }

    return validDocument;
  }
  
  
  /**
   * Utility method for turning a file into a string.
   * @param docFile
   * @return
   * @throws IOException
   */
  protected String docAsString( File docFile ) throws IOException{
    FileReader fr;
    BufferedReader br;
    StringBuffer sb = new StringBuffer();
    String line;
    fr = new FileReader( docFile );
    br = new BufferedReader( fr );
    while( ( line = br.readLine() ) != null ){
        sb.append( line + "\n");
    }
    return sb.toString();
}


}
