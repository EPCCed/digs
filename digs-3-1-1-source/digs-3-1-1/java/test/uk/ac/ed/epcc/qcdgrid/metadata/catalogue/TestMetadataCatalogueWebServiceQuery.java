package test.uk.ac.ed.epcc.qcdgrid.metadata.catalogue;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MetadataCatalogueWebServiceQuery;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ResultInfo;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import javax.xml.rpc.ServiceException;

import junit.framework.TestCase;




/**
 * Some of the tests in this testcase depend on the correct (ILDG) test database
 * being installed in the exist server on the localhost. There are three ensembles
 * containing 3, 4 and 2 configurations in this server.
 * 
 * To run with Junit, specify -DBROWSER_LOCATION=/some/path/to/writable/dir
 * and -noloading.
 * @author daragh
 *
 */
public class TestMetadataCatalogueWebServiceQuery extends TestCase {

  private MetadataCatalogueWebServiceQuery ensembleQuery_;
  private MetadataCatalogueWebServiceQuery configurationQuery_;
  private MetadataCatalogueWebServiceQuery longRunningQuery_;
  private MetadataCatalogueWebServiceQuery queryReturningNoResults_;
  private String localEndPoint_ = "http://localhost:8080/axis/services/MetadataCatalogue";
  private String badEndPoint_ = "http://nosuchhost.com:8080/";
  private String productionEndPoint_ = "http://ukqcdcontrol:8080/axis/services/MetadataCatalogue";  
  private String configFileLocation_ = System.getProperty("BROWSER_HOME");
  
  
  public void setUp(){
    try{
      createConfigFile("ILDG", "-1");
      String query = "/markovChain";
      String queryType = QueryType.ENSEMBLE_QUERY_TYPE;
      ensembleQuery_ = new MetadataCatalogueWebServiceQuery(localEndPoint_, query, queryType);
      
      createConfigFile("ILDG", "2");
      query = "/gaugeConfiguration";
      queryType = QueryType.CONFIGURATION_QUERY_TYPE;
      configurationQuery_ = new MetadataCatalogueWebServiceQuery(localEndPoint_, query, queryType);  
      
      // We know this will take a little while on the production server
      createConfigFile("ILDG", "20");
      query="/gaugeConfiguration[//dataLFN='NF2/BETA52/CLOVER202/V16X32/KAPPA3500/GAUGE/D52C202K3500U008900.tar']";
      longRunningQuery_ = new MetadataCatalogueWebServiceQuery(productionEndPoint_, query, queryType);
      
      // This should return no results
      queryReturningNoResults_ = new MetadataCatalogueWebServiceQuery(localEndPoint_,
          "/noSuchElement", 
          queryType);
    }
    catch(Exception e){
      fail("Unexpected exception in setUp: " + e.getMessage());
    }
  }
  
  public void tearDown(){
    try{
      deleteConfigFile();
    }
    catch(Exception e){
      
    }
  }
  
  /**
   * Checks that fundamental propeties are set correctly.
   *
   */
  public void testConstructor() {
    // Check the properties are set correctly.
    assertEquals(localEndPoint_, ensembleQuery_.getEndpoint());
    assertEquals(localEndPoint_, configurationQuery_.getEndpoint());
    assertEquals(QueryType.ENSEMBLE_QUERY_TYPE, ensembleQuery_.getQueryType() );
    assertEquals(QueryType.CONFIGURATION_QUERY_TYPE, configurationQuery_.getQueryType() );
    assertEquals("/markovChain", ensembleQuery_.getQuery());
    assertEquals("/gaugeConfiguration", configurationQuery_.getQuery());
    assertTrue(this.ensembleQuery_.isRunning() == false);
    assertTrue(this.configurationQuery_.isRunning() == false);
    assertEquals(-1, this.ensembleQuery_.getMaxResultsPerQuery());
    assertEquals( 2, this.configurationQuery_.getMaxResultsPerQuery());
    assertNull(ensembleQuery_.getErrorList());
    assertNull(configurationQuery_.getErrorList());
  }

  /**
   * Tests the doQuery method. This exercises the doBatchQuery private method, 
   * since maxResults &lt; 0 in the setup() for the configurationQuery object.
   */
  public void testDoQueryCfg(){
    configurationQuery_.doQuery();
    ResultInfo [] res = configurationQuery_.getResults();
    assertNotNull(res);
    assertEquals(9, res.length);
    ArrayList l = new ArrayList();
    for(int i=0; i<res.length; i++){
      l.add(res[i].getResultIdentifier());
    }
    assertTrue(l.contains("lfn://ukqcd/ildg/test/Cfg11"));
    assertTrue(l.contains("lfn://ukqcd/ildg/test/Cfg24"));
    assertTrue(l.contains("lfn://ukqcd/ildg/test/Cfg32"));
    assertTrue(!l.contains("lfn://ukqcd/ildg/test/Cfg55"));
    assertNull(configurationQuery_.getErrorList());
    assertTrue(configurationQuery_.isFinished());
  }
  
  
  /**
   * Tests the doQuery method. This exercises the doQueryAllResults private method, since
   *  since maxResults &gt; 0 in the setup() for the ensembleQuery object.
   *
   */
  public void testDoQueryEns(){
    this.ensembleQuery_.doQuery();
    ResultInfo [] res = ensembleQuery_.getResults();
    assertNotNull(res);
    assertEquals(3, res.length);
    ArrayList l = new ArrayList();
    for(int i=0; i<res.length; i++){
      l.add(res[i].getResultIdentifier());
    }
    assertTrue(l.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS1"));
    assertTrue(l.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS2"));
    assertTrue(l.contains("http://www.epcc.ed.ac.uk/ildg/test/ENS3"));
    assertTrue(!l.contains("lfn://ukqcd/ildg/test/Cfg55"));
    assertNull(ensembleQuery_.getErrorList());
    assertTrue(ensembleQuery_.isFinished());
  }
  
  public void testErrorSetOnBadEndpoint(){
    try{
    configurationQuery_ = new MetadataCatalogueWebServiceQuery( badEndPoint_,
        "/query", QueryType.CONFIGURATION_QUERY_TYPE);
    configurationQuery_.doQuery();
    }catch(ServiceException se){ 
      fail("Unexpected ServiceException");
    }
    assertNotNull(configurationQuery_.getErrorList());
    ////System.out.println("Size of error:" + configurationQuery_.getErrorList().size());
    assertTrue(1<configurationQuery_.getErrorList().size());    
  }
  
  
  public void testConcurrentProperties(){
    Thread t = new Thread(longRunningQuery_);
    t.start();
    try{
      Thread.sleep(1000);
    }
    catch(InterruptedException e){
      
    }
    assertTrue(longRunningQuery_.isRunning()==true);
    System.out.println("Asserted running");
    longRunningQuery_.stop();
    assertTrue(longRunningQuery_.wasStopped()==true);
    System.out.println("Asserted stopped");
    assertTrue(longRunningQuery_.isRunning()==false);
    System.out.println("Asserted isRunning=false");
    assertNull(longRunningQuery_.getResults());
    System.out.println("Asserted null results");
  }
  
  public void testLongRunningQuery(){
    Thread t = new Thread(longRunningQuery_);
    t.start();
    try{
      Thread.sleep(1000);
    }
    catch(InterruptedException e){
      
    }
    assertTrue(longRunningQuery_.isRunning()==true);
    while(!longRunningQuery_.isFinished()){
      try{
        Thread.sleep(5000);
        System.out.println("Waiting...");
      } 
      catch(InterruptedException e){
        
      }
      
    }
    
  }
  
  public void testQueryExpectingNoResults(){
    queryReturningNoResults_.doQuery();
    ResultInfo[]ri = queryReturningNoResults_.getResults();
    assertEquals(0,ri.length);
  }
  
  public void testErrorSetOnBadQuery(){
    try{
      configurationQuery_ = new MetadataCatalogueWebServiceQuery( localEndPoint_,
          "#432#5#((£", QueryType.CONFIGURATION_QUERY_TYPE);
      configurationQuery_.doQuery();
      }catch(ServiceException se){ 
        fail("Unexpected ServiceException");
      }
      catch(Exception e){
        fail(e.getMessage());
      }
      assertNotNull(configurationQuery_.getErrorList());
      assertEquals(1,configurationQuery_.getErrorList().size());      
  }
  
  /**
   * Creates a dummy config file in the correct format with the specified
   * mode.
   * @param mode
   * @throws IOException
   */
  private void createConfigFile(String mode, String maxResults) throws IOException{
    String configFileContents = FeatureConfiguration.MODE_PROPERTY_IDENTIFIER 
      + "=" + mode;
    configFileContents += "\n"+ FeatureConfiguration.MAXIMUM_QUERY_RESULTS_PROPERTY_IDENTIFIER
      +"=" + maxResults;
    String filename = configFileLocation_+"/"+FeatureConfiguration.FEATURE_CONFIG_FILE_NAME;
    ////System.out.println("Creating config file: " + filename);
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
