package test.uk.ac.ed.epcc.qcdgrid.metadata.catalogue;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.SchemaInfo;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridMetadataClient;

import junit.framework.TestCase;

public class TestSchemaInfo extends TestCase {

 
  /**
   * Tests that the class works correctly when passed valid input data in 
   * the constructor.
   */
  public void testWorkingCorrectly(){
    SchemaInfo info = null;
    try{
      QCDgridMetadataClient cl = new QCDgridMetadataClient("localhost:8080");
      System.out.println("Client created successfully");      
      info = new SchemaInfo(cl);
    }
    catch(Exception e){
      e.printStackTrace();
      fail("Unexpected creation exception: " + e.getMessage());
    }    
    assertEquals("QCDmlConfig1.3.0.xsd", info.getConfigurationSchemaFileName());
    assertEquals("QCDmlEnsemble1.3.0.xsd", info.getEnsembleSchemaFileName());
  }
  
 
  
}
