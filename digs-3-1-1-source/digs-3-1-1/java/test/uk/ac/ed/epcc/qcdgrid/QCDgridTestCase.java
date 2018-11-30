


package test.uk.ac.ed.epcc.qcdgrid;

import junit.framework.TestCase;
import junit.framework.Assert;


public class QCDgridTestCase extends TestCase{

    protected String testDataDir_;

    public QCDgridTestCase(){
      testDataDir_ = System.getProperty( "user.home" );
      testDataDir_ += "/metadata/java/testData/";
    }


}
