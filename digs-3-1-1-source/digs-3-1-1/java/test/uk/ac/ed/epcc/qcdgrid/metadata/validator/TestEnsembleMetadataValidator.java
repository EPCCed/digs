/***********************************************************************
 *
 *   Filename:   EnsembleMetadataValidator.java
 *
 *   Authors:    Daragh Byrne          (daragh)   EPCC.
 *
 *   Purpose:    Tests the EnsembleMetadataValidator class
 *
 *   Contents:   TestEnsembleMetadataValidator
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

package test.uk.ac.ed.epcc.qcdgrid.metadata.validator;

import junit.framework.TestCase;
import junit.framework.Assert;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.EnsembleMetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.MetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

import test.uk.ac.ed.epcc.qcdgrid.QCDgridTestCase;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

import java.io.File;

/**
 * Checks the operation of the Ensemble metadata validator, which is supposed to
 * throw appropriate exceptions when invalid documents are tested.
 */
public class TestEnsembleMetadataValidator extends QCDgridTestCase {

  private File ensembleInMDC_;

  private File ensembleNotInMDC_;

  private File randomInvalidXMLFile_;

  private EnsembleMetadataValidator validator_;

  private QCDgridMetadataClient mdClient_;
  
  public void setUp() {
 
    ensembleInMDC_ = new File(testDataDir_ + "ensembleInMDC.xml");
    ensembleNotInMDC_ = new File(testDataDir_ + "ensembleNotInMDC.xml");
    randomInvalidXMLFile_ = new File(testDataDir_ + "testOrder.xml");
    try {
      mdClient_ = new QCDgridMetadataClient("localhost:8080");
      validator_ = new EnsembleMetadataValidator(mdClient_);
    } catch (Exception e) {
      e.printStackTrace();
      fail("Unexpected exception was thrown: " + e.getClass().getName());
    }
  }

  /**
   * When the ensemble is already in the MDC, then it is INVALID and a
   * ValidationException is thrown.
   */
  public void testEnsembleInMDC() {
    try {
      validator_.validateMetadataDocument(ensembleInMDC_);
    } catch (ValidationException ve) {
      assertEquals(EnsembleMetadataValidator.MSG_ENSEMBLE_ID_INVALID, ve
          .getMessage());
      return;
    } catch (Exception e) {
      fail("Unexpected exception: " + e.getMessage() + e.getClass().getName());
    }
    fail("Appropriate exception not thrown");
  }

  /**
   * When the ensemble is NOT already in the MDC, then it is VALID, and no
   * exception should be thrown.
   */
  public void testEnsembleNotInMDC() {
    try {
      validator_.validateMetadataDocument(ensembleNotInMDC_);
    } catch (ValidationException ve) {
      fail("Unexpected validation exception thrown");
    } catch (Exception e) {
      fail("Unexpected exception thrown: " + e.getMessage());
    }
  }

  /**
   * Tries to validate a file that is not valid against the schema. TODO: When
   * the schema is fixed, make this work again. public void
   * testWithFileInvalidAgainstSchema(){ try{
   * validator_.validateMetadataDocument( randomInvalidXMLFile_ ); } catch(
   * ValidationException ve ){ assertTrue( true ); } catch( Exception e ){ fail(
   * "An exception prevented the test from running: " + e.getMessage() ); } }
   */

}
