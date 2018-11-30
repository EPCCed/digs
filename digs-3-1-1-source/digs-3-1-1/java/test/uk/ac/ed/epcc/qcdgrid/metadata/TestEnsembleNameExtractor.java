/***********************************************************************
*
*   Filename:   TestEnsembleNameExtractor.java
*
*   Authors:    Daragh Byrne           (daragh)   EPCC.
*
*   Purpose:    Tests the EnsembleNameExtractor class
*
*   Content:    TestEnsembleNameExtractor class
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
package test.uk.ac.ed.epcc.qcdgrid.metadata;

import test.uk.ac.ed.epcc.qcdgrid.QCDgridTestCase;

import uk.ac.ed.epcc.qcdgrid.metadata.DocumentFormatException;

import uk.ac.ed.epcc.qcdgrid.metadata.EnsembleNameExtractor;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;

import org.w3c.dom.Document;

import java.io.File;

public class TestEnsembleNameExtractor extends QCDgridTestCase {
    private EnsembleNameExtractor extractor_;
 
    public void setUp(){
	extractor_ = new EnsembleNameExtractor();
    }


    /**
     * Needs to be able to extract the ensemble name from
     * a configuration MDD
     */
    public void testExtractionFromConfigurationDocument(){
	// Name is: http://www.lqcd.org/ildg/ukqcd/ukqcd1
	String validConfigDocumentPath
	    = testDataDir_ + "configAlreadyInDB.xml";
	Document domConfigDoc = null;
	try{
	    File f = new File( validConfigDocumentPath );
	    domConfigDoc = XmlUtils.domFromFile( f );
	}
	catch( Exception e) {
	    fail( "Couldn't create document " +  e.getMessage() );
	}
	String expectedName = "http://www.lqcd.org/ildg/ukqcd/ukqcd1";
	String receivedName = null;
	try{
	    receivedName = extractor_.getEnsembleNameFromConfigurationDocument( domConfigDoc );
	}catch( DocumentFormatException dfe ){
	    System.out.println( dfe );
	    fail( "format exception" );
	}
	
	assertEquals( expectedName, receivedName );
    }


    /**
     * Needs to be able to extract the ensemble name from
     * an ensemble MDD.
     */
    public void testExtractionFromEnsembleDocument(){
	// Name is: http://www.lqcd.org/ildg/ukqcd/ukqcd1
	String validEnsembleDocumentPath = testDataDir_ + "UKQCD1.xml";
	Document domEnsembleDoc = null;
	try{
	    File f = new File( validEnsembleDocumentPath );
	    domEnsembleDoc = XmlUtils.domFromFile( f );
	}
	catch( Exception e) {
	    fail(  "Couldn't create document " + e.getMessage() );
	}
	String expectedName = "http://www.lqcd.org/ildg/ukqcd/ukqcd1";
	String receivedName = null;
	try{
	   receivedName = extractor_.getEnsembleNameFromEnsembleDocument( domEnsembleDoc );
	}
	catch( DocumentFormatException dfe) {
	    System.out.println( dfe );
	    fail( "format exception" );
	}

	assertEquals( expectedName, receivedName );

    }



}


