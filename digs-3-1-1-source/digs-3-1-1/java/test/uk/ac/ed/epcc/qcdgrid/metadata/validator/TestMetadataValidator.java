/***********************************************************************
 *
 *   Filename:   TestMetadataValidator.java
 *
 *   Authors:    Daragh Byrne          (daragh)   EPCC.
 *
 *   Purpose:    Test class for the MetadataValidator class
 *
 *   Contents:   TestMetadataValidator
 *
 *   Used in:    Test Suite
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

import java.io.InputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.File;
import java.io.IOException;

import org.w3c.dom.Document;

import uk.ac.ed.epcc.qcdgrid.metadata.validator.MetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;

import test.uk.ac.ed.epcc.qcdgrid.QCDgridTestCase;

import junit.framework.TestCase;
import junit.framework.Assert;

/**
 * Tests the MetadataValidator operation.
 */

public class TestMetadataValidator extends QCDgridTestCase {
  // The schema is stored in an inputstream for easy validation.
  // When Java 1.5 is commonplace we could use the Validation classes.
  private File testXmlFile_;
  private File schemaFile_;

  // The object to test.
  private MetadataValidator validator_;

  // The MDC client
  private QCDgridMetadataClient mdClient_;

  public void setUp() {
  }

  public void tearDown() {

  }

  // A mock validator that always passes the test (i.e.
  class MockValidator extends MetadataValidator {
    protected void validateImpl(Document toValidate, File docFile)
        throws ValidationException, IOException {
      // System.out.println( toValidate.getDocumentElement().getTagName() );

    }

    public MockValidator(File f) {
      super(f);
    }
  }

  /**
   * Tests that no exception is thrown when the document is valid against the
   * schema
   */
  public void testSchemaValidateWithValidData() {
    try {
      schemaFile_ = new File(testDataDir_ + "testOrderSchema.xsd");
      testXmlFile_ = new File(testDataDir_ + "testOrder.xml");
      validator_ = new MockValidator(schemaFile_);
      validator_.validateMetadataDocument(testXmlFile_);
    } catch (ValidationException ve) {
      // Test fails
      System.out.println(ve.getMessage());
      fail("ValidationException was thrown: " + ve.getMessage());
    } catch (Exception e) {
      System.out.println(e.getMessage());
      fail("Exception was thrown: " + e.getMessage());
    }

  }

  /**
   * Tests that a ValidationException is thrown when the document is Invalid
   * against the schema.
   */
  public void testSchemaValidateWithInvalidData() {
    try {
      schemaFile_ = new File(testDataDir_ + "testOrderSchema.xsd");
      testXmlFile_ = new File(testDataDir_ + "testOrderBroken.xml");
      validator_ = new MockValidator(schemaFile_);
      validator_.validateMetadataDocument(testXmlFile_);
    } catch (ValidationException ve) {
      assertTrue(true);
    } catch (Exception ioe) {
      fail(ioe.toString());
    } finally {
    }

  }

  /**
   * When the validateMetadataDocument method is run multiple times the results
   * should be the same.
   */

  public void testSchemaValidateWithInvalidDataTwice() {
    testSchemaValidateWithInvalidData();
    testSchemaValidateWithInvalidData();
  }

  public void runTest() {
    testSchemaValidateWithValidData();
    testSchemaValidateWithInvalidDataTwice();
    testSchemaValidateWithInvalidData();
  }

}
