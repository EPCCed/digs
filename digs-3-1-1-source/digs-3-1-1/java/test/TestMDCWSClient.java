

import java.rmi.RemoteException;

import org.lqcd.ildg.mdc.MdcSoapBindingStub;
import org.lqcd.ildg.mdc.MDCInterface;
import org.lqcd.ildg.mdc.MetadataCatalogueService;
import org.lqcd.ildg.mdc.MetadataCatalogueServiceLocator;
import org.lqcd.ildg.mdc.QueryResults;

public class TestMDCWSClient{

  private MDCInterface mdc_;
  
  public TestMDCWSClient(String url)
  {
    try{
      MetadataCatalogueServiceLocator locator = new MetadataCatalogueServiceLocator();
      mdc_ = locator.getMetadataCatalogue(new java.net.URL(url));
    }
    catch(Exception e){
      System.out.println( e.getMessage() );
//      e.printStackTrace();
    }
  }

  public static void main(String [] args){
    String oper;
    String url;
    String query;
    
    if(args.length < 3){
      printUsage();
    }
    url   = args[0];
    oper  = args[1];
    query = args[2];
    System.out.println( "URL: \t\t" + url );
    System.out.println( "Operation: \t" + oper );
    System.out.println( "Query: \t\t" + query );
    try{
      TestMDCWSClient cl = new TestMDCWSClient(url);
      QueryResults res   = cl.executeQuery(oper, query);
      printQueryResults(res);
    }catch(Exception e){
      System.out.println("Exception occurred: " + e.getMessage() );
      e.printStackTrace();
    }
  }

  public static void printUsage(){
    System.out.println("Usage: java TestMDCWSClient <url> <operationName> <query>");
  }

  public static void printQueryResults(QueryResults results){    
    if(results!=null){
      System.out.println("Recieved "+results.getNumberOfResults() + " results.");
      for(int i=0; i<results.getNumberOfResults(); i++){
        System.out.println(results.getResults()[i]);
      }
    }
    else{
      System.out.println("Null results recieved.");
    }
  }

  public QueryResults executeQuery( String operationName, String query ) throws RemoteException{
    /*if( operationName.equals( "doEnsembleQuery" ) ){
      System.out.println("doEnsembleQuery: " + query );
      return mdc_.doEnsembleQuery("Xpath", query, 1, 10);
    }*
    else*/
    if( operationName.equals("doEnsembleURIQuery") ){
      System.out.println("doEnsembleURIQuery: " + query);
      return mdc_.doEnsembleURIQuery("Xpath", query, 1, 10);
    }
    /*else if( operationName.equals( "doConfigurationQuery" ) ){
      System.out.println("doConfigurationQuery: " + query );
      return mdc_.doConfigurationQuery("Xpath", query, 1, 10);
    }*/
    else if( operationName.equals( "doConfigurationLFNQuery" ) ){
      System.out.println("doConfigurationLFNQuery: " + query );
      return mdc_.doConfigurationLFNQuery("Xpath", query, 1, 10);
    }
    /*else if( operationName.equals( "getConfigurationMetadata" )){
      return mdc_.getConfigurationMetadata( query );
    }*/
    else{
      System.out.println( "Invalid operation." );
      return null; 
    }
  }
}
