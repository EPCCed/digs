/***********************************************************************
 *
 *   Filename:   FeatureConfiguration.java
 *
 *   Authors:    Daragh Byrne           (daragh)   EPCC.
 *
 *   Purpose:    Allows the mode of the browser to be configured.
 *
 *   Content:    FeatureConfiguration class
 *
 *   Used in:    Java client tools - browser
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2006 The University of Edinburgh
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

package uk.ac.ed.epcc.qcdgrid.browser.Configuration;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Properties;

/**
 * This class expects a browser.properties file to exist in the location
 * specified by the BROWSER_HOME system property. Usually this property will be
 * defined by using the -DBROWSER_HOME option on the command line. The file
 * should look like the following:
 * 
 * qcdgrid.browser.mode=ILDG queryrunner.maxresults=20
 * services.file.location=/home/daragh/Services.xml
 * 
 * qcdgrid.browser.mode is one of "ILDG" or "UKQCD". In the absence of this
 * file, or in the absence of the BROWSER_HOME property, the mode defaults to
 * "UKQCD".
 * 
 * queryrunner.maxresults is an integer. This specifies the maximum results that
 * the MetadataCatalogueWebServiceQuery class should request to be returned per
 * query.
 * 
 * services.file.location is the location of the Services.xml file that
 * describes the collaborations and services of the ILDG. See
 * http://www.lqcd.org/ildg/Services.xml for details of the format.
 * 
 * This class (and browser.properties) need to exist because: - some
 * configuration information needs to be specified at startup: .browserrc not
 * created until after the first run; - we wish to control the mode of the
 * issued browser.
 * 
 * @author daragh
 * 
 * TODO: make this a singleton? TODO: add a constructor that takes a URI
 * representing the location of the Services.xml file.
 * 
 */
public class FeatureConfiguration {

  /**
   * The UKQCD mode identifier.
   */
  public static final String UKQCD_MODE = "UKQCD";

  /**
   * The ILDG mode identifier
   */
  public static final String ILDG_MODE = "ILDG";

  /**
   * The name of the browser properties file.
   */
  public static final String FEATURE_CONFIG_FILE_NAME = "browser.properties";

  /**
   * The name of the property to load to identify the browser mode.
   */
  public static final String MODE_PROPERTY_IDENTIFIER = "qcdgrid.browser.mode";

  /**
   * The name of the property that identifies the maximum number of results the
   * web service query runner should attempt to return at a time.
   */
  public static final String MAXIMUM_QUERY_RESULTS_PROPERTY_IDENTIFIER = "queryrunner.maxresults";

  /**
   * The location of the services file that describes the ILDG
   */
  public static final String PATH_TO_SERVICES_FILE_IDENTIFIER = "services.file.location";

  /**
   * The name of the XMLDB user used to access the central database.
   */
  public static final String XMLDB_USER_NAME_IDENTIFIER = "xmldb.user.name";

  /**
   * The name of the field used to identify the pasword for the above user.
   */
  public static final String XMLDB_PASSWORD_IDENTIFIER = "xmldb.user.password";

  // The location of the properties file
  private String propertiesFileLocation_;

  // The actual mode
  private String browserMode_;

  // The location of the services file
  private String servicesFileLocation_;

  // The number of results per query, default to all.
  private int maxMDCWSQueryResults_ = -1;

  // The db username
  private String username_;

  // The db password
  private String password_;

  /**
   * Default constructor. Reads the value of the BROWSER_HOME system property/
   * environment variable. Parses the file named FEATURE_CONFIG_FILE_NAME from
   * the specified directory and determines the mode of the browser depending on
   * the value of the property called qcdgrid.browser.mode. If there is no such
   * file, or no such property, then the mode defaults to UKQCD. It also looks
   * up values for other configuration properties (see class docuemtation).
   */
  public FeatureConfiguration() throws FeatureConfigurationException {
    propertiesFileLocation_ = System.getProperty("BROWSER_HOME");
    init();
  }

  /**
   * Implements the activities described in the constructor doc.
   * 
   */
  private void init() throws FeatureConfigurationException {
    if (propertiesFileLocation_ == null || propertiesFileLocation_ == "") {
      setBrowserMode(FeatureConfiguration.UKQCD_MODE);
      System.out.println("Setting browser mode to default as "
          + "BROWSER_HOME was not specified.");
    }
    String configFileName = propertiesFileLocation_ + "/"
        + FeatureConfiguration.FEATURE_CONFIG_FILE_NAME;
    Properties p = new Properties();
    try {
      File propertyFile = new File(configFileName);
      if (!propertyFile.exists()) {
        System.out.println("Browser feature config file does not exist, "
            + "defaulting to UKQCD mode.");
        setBrowserMode(FeatureConfiguration.UKQCD_MODE);
      } else {
        FileInputStream fis = new FileInputStream(propertyFile);
        p.load(fis);
        String mode = p
            .getProperty(FeatureConfiguration.MODE_PROPERTY_IDENTIFIER);
        String servicesFile = p
            .getProperty(FeatureConfiguration.PATH_TO_SERVICES_FILE_IDENTIFIER);
        if (servicesFile == null || servicesFile.equals("")) {
          System.out
              .println("Warning - the location for the Services.xml file has not been set in "
                  + FeatureConfiguration.FEATURE_CONFIG_FILE_NAME);
	  servicesFile = null;
        }
	else {
	    servicesFileLocation_ = servicesFile.trim();
	}
        String username = p
            .getProperty(FeatureConfiguration.XMLDB_USER_NAME_IDENTIFIER);
        String password = p
            .getProperty(FeatureConfiguration.XMLDB_PASSWORD_IDENTIFIER);
        if (username == null) {
          username = "";
        }
        if (password == null) {
          password = "";
        }
        password_ = password.trim();
        username_ = username.trim();

        if (invalidMode(mode)) {
          throw new FeatureConfigurationException("Mode should be one of: "
              + FeatureConfiguration.ILDG_MODE + " or "
              + FeatureConfiguration.UKQCD_MODE + ", was " + mode);
        }
        setBrowserMode(mode);
      }
    } catch (FileNotFoundException fnfe) {
      System.out.println("Could not find file " + configFileName + ".");
      System.out.print("Setting browser mode to default of UKQCD");
      ;
      setBrowserMode(FeatureConfiguration.UKQCD_MODE);
    } catch (IOException ioe) {
      System.out.println("Could not find file " + configFileName + ".");
      System.out.print("Setting browser mode to default of UKQCD");
      ;
      setBrowserMode(FeatureConfiguration.UKQCD_MODE);
    }
    // Now deal with the max results property
    try {
      int maxResults = Integer
          .parseInt(p
              .getProperty(FeatureConfiguration.MAXIMUM_QUERY_RESULTS_PROPERTY_IDENTIFIER));
      setMDCWSQueryMaxResults(maxResults);
    } catch (NumberFormatException nfe) {
      System.out.println("Could not parse the number of query results to be"
          + " obtained in one go for metadata catalogue queries, defaulting to"
          + " -1 (all)");
    }
  }

  /*
   * Mode can only be ILDG or UKQCD.
   */
  private boolean invalidMode(String mode) {
    if (mode.compareTo(FeatureConfiguration.ILDG_MODE) == 0) {
      return false;
    }
    if (mode.compareTo(FeatureConfiguration.UKQCD_MODE) == 0) {
      return false;
    }
    return true;
  }

  /**
   * The maximum number of results per query as used by the
   * MetadataCatalogueWebServiceQuery class.
   */
  public int getMDCWSQueryMaxResults() {
    return this.maxMDCWSQueryResults_;
  }

  /*
   * Setter for the max queryresults parameter
   */
  private void setMDCWSQueryMaxResults(int maxResults) {
    this.maxMDCWSQueryResults_ = maxResults;
  }

  /**
   * Accessor.
   * 
   * @param mode
   */
  private void setBrowserMode(String mode) {
    browserMode_ = mode;
  }

  /**
   * Read the browser mode that has been configured.
   * 
   * @return
   */
  public String getBrowserMode() {
    return browserMode_;
  }

  /**
   * Get the location of the Services.xml file.
   */
  public String getServicesFileLocation() {
    return servicesFileLocation_;
  }

  /**
   * The XMLDB username
   * 
   * @return
   */
  public String getXMLDBUserName() {
    return username_;
  }

  /**
   * The XMLDB password
   * 
   * @return
   */
  public String getXMLDBPassword() {
    return password_;
  }

  /**
   * Thrown when the properties file contains invalid data.
   * 
   * @author daragh
   * 
   */
  public class FeatureConfigurationException extends Exception {
    public FeatureConfigurationException(String message) {
      super(message);
    }
  }

}
