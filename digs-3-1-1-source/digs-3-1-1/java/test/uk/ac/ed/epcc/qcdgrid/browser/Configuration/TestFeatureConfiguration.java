/***********************************************************************
*
*   Filename:   TestFeatureConfiguration.java
*
*   Authors:    Daragh Byrne           (daragh)   EPCC.
*
*   Purpose:    Tests the FeatureConfiguration class
*
*   Content:    TestFeatureConfiguration class
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



package test.uk.ac.ed.epcc.qcdgrid.browser.Configuration;

import junit.framework.TestCase;

import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;


/**
 * Test class for the FeatureConfiguration class, which holds information about
 * the mode the browser is to operate in.
 * @author daragh
 *
 */
public class TestFeatureConfiguration extends TestCase {

  
  /*
   * The FeatureConfiguration class assumes two things:
   * 1. The existance of a browser.properties
   * 2. The existance of a BROWSER_HOME system propertthat points to the
   * directory that the browser.properties file lives in. This should be set when
   * running the browser, e.g. 
   * java -DBROWSER_HOME=/path/to/browser
   * 
   * If either of these are missing, then the mode of the browser should default to 
   * UKQCD (i.e. FeatureConfiguration.UKQCD_MODE). It is expected that the configuration
   * file will live in the java directory of the root of the QCDgrid distribution.
   *
   */
  private String configFileLocation_;
  private FeatureConfiguration config_;
  
  public void setUp(){
    configFileLocation_ = System.getProperty("user.home");
  }
  
  /**
   * Deletes the configuration file if it exists.
   */
  public void tearDown(){
    try{
      deleteConfigFile();
      System.out.println("Deleted temp config file successfully.");
    }
    catch(Exception e){
      System.out.println("Could not delete temp config file.");
    }
  }
  
  
  /**
   * Creates a config file that sets the mode to UKQCD and tests the
   * basic operation.
   *
   */
  public void testBasicUKQCDMode(){
    System.setProperty("BROWSER_HOME", configFileLocation_);    
    try{
      createConfigFile(FeatureConfiguration.UKQCD_MODE, "-1", "/home/daragh/Services.xml");
    }
    catch(IOException e){
      fail("Error (1) creating dummy config file: "+ e.getMessage());
    }
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Corrupt browser.properties: " + fce.getMessage());
    }
    assertEquals(FeatureConfiguration.UKQCD_MODE, config_.getBrowserMode());
    
  }

  /**
   * Creates a config file that sets the mode to ILDG and tests the
   * basic operation.
   *
   */
  public void testBasicILDGMode(){
    System.setProperty("BROWSER_HOME", configFileLocation_);    
    try{
      createConfigFile(FeatureConfiguration.ILDG_MODE, "20", "/home/daragh/Services.xml");
    }
    catch(IOException e){
      fail("Error (2) creating dummy config file: " + e.getMessage());
    }
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Corrupt browser.properties: " + fce.getMessage());
    }
    assertEquals(FeatureConfiguration.ILDG_MODE, config_.getBrowserMode());
    
  }
  
  
  /**
   * Creates a config file that sets the mode to ILDG and tests the
   * basic operation.
   *
   */
  public void testBasicInvalidMode(){
    System.setProperty("BROWSER_HOME", configFileLocation_);    
    try{
      createConfigFile("INVALID_MODE", "-1", "/home/daragh/Services.xml");
    }
    catch(IOException e){
      fail("Error (3) creating dummy config file: " + e.getMessage());
    }
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException ife){
      //Success
      return;
    }
    fail("Exception not thrown.");
  }
  
  
  /**
   * Tests that, in the absence of the config file, the mode defaults to 
   * UKQCD.
   *
   */
  public void testConfigFilePresentDefaultsToQCDgridMode(){

    System.setProperty("BROWSER_HOME", configFileLocation_);    
    // There should be no file present at the start of the test
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Corrupt browser.properties: " + fce.getMessage());
    }
    assertEquals(FeatureConfiguration.UKQCD_MODE, config_.getBrowserMode());    
  }
  
  
  /**
   * Should default to UKQCD when the BROWSER_HOME value is null or empty.
   * Cannot test the null case at the moment as it is impossible to do 
   * System.setProperty("KEY", null);
   */
  public void testBrowserHomePropertyNotSet(){
    System.setProperty("BROWSER_HOME", "");    
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Corrupt browser.properties");
    }
    assertEquals(FeatureConfiguration.UKQCD_MODE, config_.getBrowserMode());    
    
  }
  
  
  public void testGetMaxMDCWSQueryResults(){
    System.setProperty("BROWSER_HOME", configFileLocation_);    
    try{
      createConfigFile(FeatureConfiguration.ILDG_MODE, "20", "/home/daragh/Services.xml");
    }
    catch(IOException e){
      fail("Error (4) creating dummy config file: " + e.getMessage());
    }
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Corrupt browser.properties: " + fce.getMessage());
    }
    assertEquals(FeatureConfiguration.ILDG_MODE, config_.getBrowserMode());
    assertEquals(20, config_.getMDCWSQueryMaxResults());
    
    // Try with bad string
    System.out.println("Trying with bad string");
    try{
      createConfigFile(FeatureConfiguration.ILDG_MODE, "bal3]]#23f", "/home/daragh/Services.xml");
    }
    catch(IOException e){
      fail("Error (5) creating dummy config file: " + e.getMessage());
    }
    try{
      config_ = new FeatureConfiguration();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Corrupt browser.properties: " + fce.getMessage());
    }
    assertEquals(FeatureConfiguration.ILDG_MODE, config_.getBrowserMode());
    assertEquals(-1, config_.getMDCWSQueryMaxResults());
    
  }
  
  public void testServicesFileLocation(){
    
    try{
      createConfigFile(FeatureConfiguration.ILDG_MODE, "20", "/home/daragh/Services.xml");
    }
    catch(IOException e){
      fail("Exception: " + e.getMessage());
    }
    try{
      FeatureConfiguration fc = new FeatureConfiguration();
      
      assertEquals("/home/daragh/Services.xml", fc.getServicesFileLocation());
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      fail("Caught featureconfigexception");
    }
  }
  
  
  /**
   * Creates a dummy config file in the correct format with the specified
   * mode.
   * @param mode
   * @throws IOException
   */
  private void createConfigFile(String mode, String maxResults, String fileLocation) throws IOException{
    String configFileContents = FeatureConfiguration.MODE_PROPERTY_IDENTIFIER 
      + "=" + mode;
    configFileContents += "\n"+ FeatureConfiguration.MAXIMUM_QUERY_RESULTS_PROPERTY_IDENTIFIER
      +"=" + maxResults;
    configFileContents += "\n" + FeatureConfiguration.PATH_TO_SERVICES_FILE_IDENTIFIER
       + "=" + fileLocation;
    String filename = configFileLocation_+"/"+FeatureConfiguration.FEATURE_CONFIG_FILE_NAME;
    System.out.println("Creating config file: " + filename);
    FileWriter fw = new FileWriter(
        new File(filename));
    fw.write(configFileContents);
    fw.close();
  }
  
  

  /**
   * Deletes the dummy configuration file if it exists.
   */  
  private void deleteConfigFile() throws IOException{
    File f = new File(configFileLocation_+"/"+FeatureConfiguration.FEATURE_CONFIG_FILE_NAME);
    f.delete();
  }
  
  
}
