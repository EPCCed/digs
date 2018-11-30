





package test.uk.ac.ed.epcc.qcdgrid.metadata;

import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.DirectoryValidationException;

import test.uk.ac.ed.epcc.qcdgrid.QCDgridTestCase;

import java.io.File;

import junit.framework.TestCase;
import junit.framework.Assert;

public class TestQCDgridMetadataClient extends QCDgridTestCase {

    private QCDgridMetadataClient mdc_;
    // A file with this ID is assumed to exist in the local DB
    private String lfnKnownToExist_ = "NF2/BETA52/CLOVER202/V16X32/KAPPA3565/GAUGE/D52C202K3565U029370.tar";
    // An ensemble with this ID is assumed to exist in the local DB
    private String mcuKnownToExist_ = "http://www.lqcd.org/ildg/ukqcd/ukqcd1";

    private String fileNameShouldNotExist_;

    private File invalidConfig_;
    private File validAndInvalidDirectory_;

    public void setUp(){
	try{
	    mdc_ = new QCDgridMetadataClient( "localhost:8080" );
	    invalidConfig_ = new File( testDataDir_ + "configAlreadyInMDC.xml" );
	    validAndInvalidDirectory_ = new File( testDataDir_ + "validAndInvalidFiles" );
	    fileNameShouldNotExist_ =  "should_not_exist.xml";
	    mdc_.removeFile( fileNameShouldNotExist_ );
	    /*    File [] fileList;
	    fileList=validAndInvalidDirectory_.listFiles();
	    for( int i = 0; i < fileList.length; i++ ){
	    try{
	    mdc_.removeFile( fileList[i].getName());
	    }
	    catch( Exception e ){}
	    
	    }*/
	}
	catch( Exception e ){
	    System.out.println( e.getMessage() );
	    fail( e.toString() );
	}
    }


    public void tearDown() {

    }


    /** 
     * We know that the file with dataLFN=lfn_ is present in the MDC.
     * isLFNRepresentedInMDC should return true.
     */
    public void testLfnRepresentedInMDC(){
	try{	
	    boolean isRepresented = mdc_.isLFNRepresentedInMDC( lfnKnownToExist_ );
	    if( !isRepresented ) {
		fail( "File not found in database" );
	    }
	}catch( QCDgridException e ){
	    fail( "Test could not be run." );
	}
    }

    /** 
     * We know the file with dataLFN=TOTALYMADEUPLFN is not present in the
     * MDC.  isRepresentedInMDC should return false.
     */
    public void testLfnNotRepresentedInMDC(){
	try{	
	    boolean isRepresented = mdc_.isLFNRepresentedInMDC( "TOTALLY_MADE_UP_LFN" );
	    if( !isRepresented ) {
		assertTrue( true );
	    }
	}catch( QCDgridException e ){
	    fail( "Test could not be run." );
	}
    }
    
    /**
     * We know that the ensemble with markovChainURI=markovChainURI_ is present.
     * isEnsemblePresentInMDC should return true.
     */
    public void testEnsembleRepresentedInMDC(){
	try{	
	    boolean isPresent = mdc_.isEnsembleRepresentedInMDC( mcuKnownToExist_ );
	    if( !isPresent ) {
		fail( "File not found in database" );
	    }
	}catch( QCDgridException e ){
	    fail( "Test could not be run." );
	}
    }




    /**
     * We know that the ensemble with the made up name is not in the DB.
     * isEnsembleRepresentedInMDC returns false.
     */
    public void testEnsembleNotRepresentedInMDC(){
	try{	
	    boolean isPresent = mdc_.isEnsembleRepresentedInMDC( "ACOMPLETELY-MADE-UPENSEMBLENAME-TH-AtsnotReal" );
	    if( !isPresent ) {
		assertTrue( true );
	    }
	}catch( QCDgridException e ){
	    fail( "Test could not be run." );
	}
    }


    /**
     * The validator should throw an exception for every file that isn't in 
     * the DB.
     */
    public void testInvalidConfigFileRejected(){
	try{
	    mdc_.submitConfigMetadataFile( invalidConfig_ );
	}
	catch( ValidationException ve ){
	    assertTrue(true);
	    return;
	}
	catch( Exception e ) {
	    fail( "Unexpected exception thrown." );
	}
	fail( "Expected exception not thrown" );
    }

    /** 
     * The client should return details of the files in a directory that
     * are invalid.
     */
    public void testDirectoryWithBadConfigsRejected(){
	/*
	try{
	    mdc_.submitConfigMetadataDirectory( validAndInvalidDirectory_ );
	}
	catch( DirectoryValidationException dve ){
	    assertEquals( 2, dve.getInvalidFileCount() );
	    for( int i=0; i<dve.getInvalidFileCount(); i++ ){
		System.out.println( dve.getInvalidFileList().get( i ) );
	    }
	    return;
	}
	catch( Exception e ){
	    fail( "Unexpected exception caught" );       
	}
	fail( "Expected exception not thrown" );
	*/
    }

    public void runTests(){
	testLfnRepresentedInMDC();
	testLfnNotRepresentedInMDC();
	testEnsembleRepresentedInMDC();
	testEnsembleNotRepresentedInMDC();
    }

}
