package test.uk.ac.ed.epcc.qcdgrid.browser.Configuration;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.List;

import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.ServicesFile;
import junit.framework.TestCase;


public class TestServicesFile extends TestCase {

  private ServicesFile sf_;
  private String configFileLocation_;
  
  
  public void setUp(){
    try{
      configFileLocation_ = System.getProperty("user.home");
      createConfigFile("ILDG", "20", "/home/daragh/Services.xml");
      FeatureConfiguration fc = new FeatureConfiguration();
      sf_ = new ServicesFile(fc.getServicesFileLocation());
    }
    catch(Exception ex){
      fail("Caught ex" + ex.getMessage());
    }
  }
  
  
  public void testConstructor(){
    List metadataCatalougeServiceList = sf_.getMetadataCatalogueServiceList();
    assertEquals(4,metadataCatalougeServiceList.size());
    assertTrue(
        metadataCatalougeServiceList.contains(
            "http://ukqcdcontrol.epcc.ed.ac.uk:8080/axis/services/MetadataCatalogue"));
    assertTrue(
        metadataCatalougeServiceList.contains(
            "http://www.usqcd.org/mdc-service/services/ILDGMDCService"));
    assertTrue(
        metadataCatalougeServiceList.contains(
            "http://globe-meta.ifh.de:8080/axis/services/ILDG_MDC"));
    assertTrue(
        metadataCatalougeServiceList.contains(
            "http://ws.jldg.org/axis/services/mdc"));
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
