package test.uk.ac.ed.epcc.qcdgrid.metadata.catalogue;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ILDGQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MultipleWebServiceQueryRunner;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QueryExecutionException;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ResultInfo;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.List;

public class TestMultipleWebServiceQueryRunner extends TestCase {

  private MultipleWebServiceQueryRunner runner_;
  
  
  protected void setUp() throws Exception {
    List hosts = new ArrayList();
    hosts.add("http://localhost:8080/axis/services/MetadataCatalogue");
    hosts.add("http://ukqcdcontrol:8080/axis/services/MetadataCatalogue");
    runner_ = new MultipleWebServiceQueryRunner(hosts);
  }

  protected void tearDown() throws Exception {
  }

  /**
   * 
   *
   */
  public void testRunConfigurationQuery() {
    
    try{
      // We know this query only returns the one result
      ILDGQueryResults results 
      = (ILDGQueryResults)runner_.runConfigurationQuery(
          "/gaugeConfiguration[//dataLFN='NF2/BETA52/CLOVER202/V16X32/KAPPA3500/GAUGE/D52C202K3500U008900.tar']");
      assertNotNull(results);
      ResultInfo [] ri = results.getIndividualResultInfoObjects();
      assertNotNull(ri);
      System.out.println("not null results ok");
      assertEquals(1, ri.length);
      System.out.println("There were: " + ri.length + " results.");
      for(int i=0; i<ri.length; i++){
        System.out.println("Ri: " + i + " " + ri[i]);
      }
    }
    catch(QueryExecutionException qe){
      fail("Exception occurred when executing query.");
    }
    
  }


  /*
   *
   */
  public void testRunEnsembleQuery() {

  }

  public void testStop(){
    
  }
  
  
  
  public void testWithNonExistantServicesList(){
    
  }

}
