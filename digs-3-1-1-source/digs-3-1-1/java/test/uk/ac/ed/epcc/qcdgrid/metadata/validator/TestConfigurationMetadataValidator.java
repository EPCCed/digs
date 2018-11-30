/***********************************************************************
 *
 *   Filename:   TestConfigurationMetadataValidator.java
 *
 *   Authors:    Daragh Byrne           (daragh)   EPCC.
 *
 *   Purpose:    Tests the ConfigurationMetadataValidator class
 *
 *   Contents:   TestConfigurationMetadataValidator class
 *
 *   Used in:    Test suite
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

package test.uk.ac.ed.epcc.qcdgrid.metadata.validator;

import junit.framework.TestCase;
import junit.framework.Assert;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ConfigurationMetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.MetadataValidator;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;

import test.uk.ac.ed.epcc.qcdgrid.QCDgridTestCase;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

import java.io.File;

/*
 * Tests to carry out: - ValidationException is thrown when document NOT
 * conforming to schema is tested. - when a CMD containing the same logical file
 * name as the document being tested is contained in the database, a
 * ValidationException should be thrown. These tests operate with version 1.1 of
 * the schema.
 */

public class TestConfigurationMetadataValidator extends QCDgridTestCase {

  private File mdDocAlreadyInMDC_;

  private File mdDocNotInMDCBadEnsID_;

  private File mdDocNotInMDCGoodEnsID_;

  private File randomInvalidXMLFile_;

  private QCDgridMetadataClient mdClient_;
  
  private ConfigurationMetadataValidator validator_;

  private String testBaseDir_;

  public void setUp() {
    
    try{
      mdClient_ = new QCDgridMetadataClient("localhost:8080");      
      validator_ = new ConfigurationMetadataValidator(mdClient_);          
    }
    catch(QCDgridException e){
      fail("Could not open database connection to localhost: " + e.getMessage());
    }
    mdDocAlreadyInMDC_ = new File(testDataDir_ + "configAlreadyInMDC.xml");
    mdDocNotInMDCBadEnsID_ = new File(testDataDir_
        + "configNotInMDCBadEnsID.xml");
    mdDocNotInMDCGoodEnsID_ = new File(testDataDir_
        + "configNotInMDCGoodEnsID.xml");
    randomInvalidXMLFile_ = new File(testDataDir_ + "testOrder.xml");
  }

  /**
   * Tries to validate a file that contains an LFN that is already represented
   * in the MDC. Expects an exception to be thrown. Prerequisite: file
   * nameOfFileAlreadyInMDC_ is actually in MDC
   */
  public void testWithFileAlreadyInMDC() {
    try {
      validator_.validateMetadataDocument(mdDocAlreadyInMDC_);
    } catch (ValidationException ve) {
      //System.out.println("testWithFileAlreadyInMDC: Validation failed: "
         // + ve.toString());
      assertEquals(ConfigurationMetadataValidator.MSG_CONFIGURATION_ID_INVALID,
          ve.getMessage());
      return;
    } catch (Exception e) {
      //System.out.println("Other exception:" + e);
    }
    fail("Exception not thrown.");
  }

  /**
   * Tries to validate a file that is not already in the database, but but has
   * an invalid ensemble ID. Expects an exception to be thrown.
   */
  public void testNotInDBBadEnsembleID() {
    try {
      validator_.validateMetadataDocument(mdDocNotInMDCBadEnsID_);
    } catch (ValidationException ve) {
      //System.out.println("testWithFileAlreadyInMDC: Validation failed: "
          //+ ve.toString());
      assertEquals(ConfigurationMetadataValidator.MSG_ENSEMBLE_ID_INVALID, ve
          .getMessage());
      return;
    } catch (Exception e) {
      ////System.out.println("Other exception:" + e);
    }
    fail("Expected exception not thrown");
  }

  /**
   * Tries to validate a file that is not already in the database, has a correct
   * valid ensemble ID (i.e. ensemble is already in the database). No exception
   * should be thrown.
   * 
   */
  public void testNotInDBGoodEnsembleID() {
    try {
      validator_.validateMetadataDocument(mdDocNotInMDCGoodEnsID_);
    } catch (ValidationException ve) {
      ////System.out.println(ve.getMessage());
      fail("Exception thrown when it should not have been.");
    } catch (Exception e) {
      ////System.out.println("Other exception:" + e);
      fail("Some unexpected exception");
    }
    assertTrue(true);
  }

  /**
   * Tries to validate a file that is not valid against the schema.
   */
  public void testWithFileInvalidAgainstSchema() {
    try {
      validator_.validateMetadataDocument(randomInvalidXMLFile_);
    } catch (ValidationException ve) {
      assertTrue(true);
    } catch (Exception e) {
      fail("An exception prevented the test from running: " + e.getMessage());
    }
  }

  /*
   * Runs the validator a number of times in each of the above scenarios.
   */
  public void testMultipleInvocations() {
    try {
      validator_.validateMetadataDocument(randomInvalidXMLFile_);
    } catch (ValidationException ve) {
      assertTrue(true);
    } catch (Exception e) {
      fail("An exception prevented the test from running: " + e.getMessage());
    }

    try {
      validator_.validateMetadataDocument(mdDocNotInMDCGoodEnsID_);
    } catch (ValidationException ve) {
      ve.printStackTrace();
      fail("Exception thrown when it should not have been: " + ve.getMessage());
      return;
    } catch (Exception e) {
      ////System.out.println("Other exception:" + e);
      fail("Some unexpected exception");
      return;
    }

    try {
      validator_.validateMetadataDocument(mdDocNotInMDCBadEnsID_);
    } catch (ValidationException ve) {
      ////System.out.println("testWithFileAlreadyInMDC: Validation failed: "
          //+ ve.toString());
      assertEquals(ConfigurationMetadataValidator.MSG_ENSEMBLE_ID_INVALID, ve
          .getMessage());
      return;
    } catch (Exception e) {
      ////System.out.println("Other exception:" + e);
    }
    fail("Expected exception not thrown");

  }

}
