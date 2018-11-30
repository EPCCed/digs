/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.Configuration;

import java.io.*;
import java.util.Vector;
import javax.swing.DefaultListModel;

import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.*;

import javax.xml.parsers.*;
import org.xml.sax.SAXException;
import org.w3c.dom.*;

/**
 * Title: Configuration Description: Handles the loading, saving and providing
 * of the browser's state and configuration Copyright: Copyright (c) 2002
 * Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class Configuration {

  /**
   * The name of the file containing the user's browser configuration
   */
  private String configFileName;

  /**
   * User's default query language
   */
  private String queryLanguage;

  /**
   * User's default result handler (full class name)
   */
  private String resultHandler;

  /**
   * User's default namespace (currently a prefix, but in the end should be a
   * real name space identifier)
   */
  private String nameSpace;

  /**
   * The host and port name where the XML database can be accessed
   */
  private String databaseUrl;

  /**
   * These two vectors hold the names and contents (as XPath strings) of all the
   * user's queries. Elements in the same positions correspond to the same
   * query, so these vectors are always the same size as each other
   */
  private Vector queries;

  /** Inner class to represent a query as stored in the configuration */
  class StoredXPathQuery {

    /** name of this query */
    String name;

    /** a vector of StoredAtomicXPathQuerys containing the contents */
    Vector contents;

    /** XML representation */
    String xml;

    /** Query type name */
    String type;

    /** whether name automatically generated */
    boolean autoNamed;

    public StoredXPathQuery() {
      contents = new Vector();
    }
  }

  class StoredAtomicXPathQuery {
    String xpath;

    String predicate;

    String node;
  }

  /**
   * Constructor: sets the configuration values to their defaults and attempts
   * to parse the config file if it exists
   */
  public Configuration() {
    /* Default values */
    queryLanguage = null;
    resultHandler = null;
    nameSpace = "";
    databaseUrl = null;

    queries = new Vector();

    /* Find where config file should be and check if it exists */
    configFileName = System.getProperty("user.home") + "/.browserrc";

    File configFile = new File(configFileName);
    if (configFile.exists()) {
      /* Try to parse file */
      readConfigFile();
    } else {
      System.out
          .println("No " + configFileName + " file found, using defaults");
    }
  }

  /**
   * Gets queries from the configuration and adds them to the query builder
   * 
   * @param qb
   *          the query builder to put the queries in
   */
  public void getQueries(XPathQueryBuilder qb) {

    DefaultListModel dlm = qb.getQueryList();

    for (int i = 0; i < queries.size(); i++) {

      StoredXPathQuery sxpq = (StoredXPathQuery) queries.elementAt(i);
      Vector atoms = new Vector();

      for (int j = 0; j < sxpq.contents.size(); j++) {
        StoredAtomicXPathQuery saxpq = (StoredAtomicXPathQuery) sxpq.contents
            .elementAt(j);
        AtomicXPathQueryExpression axpqe = null;
        if (saxpq.xpath != null) {
          axpqe = new AtomicXPathQueryExpression(saxpq.xpath);
        } else {
          axpqe = new AtomicXPathQueryExpression(qb.nodeFromPath(saxpq.node,
              sxpq.type), saxpq.predicate);
        }
        atoms.add(axpqe);
      }

      dlm.addElement(new XPathQueryExpression(sxpq.name, atoms, sxpq.type));
    }
  }

  /**
   * Sets queries in the user's configuration, based on what is currently in the
   * query builder
   * 
   * @param qb
   *          the query builder to get the queries from
   */
  public void setQueries(QueryBuilder qb) {
    queries = new Vector();

    DefaultListModel dlm = qb.getQueryList();
    for (int i = 0; i < dlm.size(); i++) {

      XPathQueryExpression xpqe = (XPathQueryExpression) dlm.elementAt(i);

      StoredXPathQuery sxpq = new StoredXPathQuery();
      sxpq.xml = xpqe.toXML();

      queries.add(sxpq);
    }
  }

  /** Try to parse the config file */
  void readConfigFile() {
    try {
      DocumentBuilderFactory docbuildfact = DocumentBuilderFactory
          .newInstance();
      DocumentBuilder docbuild = docbuildfact.newDocumentBuilder();
      Document dom = docbuild.parse(new File(configFileName));
      Element root = dom.getDocumentElement();

      if (root.getNodeName().equals("browser_config")) {
        NodeList nl = root.getChildNodes();

        for (int i = 0; i < nl.getLength(); i++) {
          Node node = nl.item(i);
          String name = node.getNodeName();

          if (name.equals("query_language")) {
            node = node.getFirstChild();
            queryLanguage = node.getNodeValue();
          } else if (name.equals("result_handler")) {
            node = node.getFirstChild();
            resultHandler = node.getNodeValue();
          } else if (name.equals("namespace")) {
            node = node.getFirstChild();
            if (node != null) {
              nameSpace = node.getNodeValue();
            }
          } else if (name.equals("database_url")) {
            node = node.getFirstChild();
            if (node != null) {
              databaseUrl = node.getNodeValue();
            }
          } else if (name.equals("queries")) {
            NodeList nl2 = node.getChildNodes();
            for (int j = 0; j < nl2.getLength(); j++) {
              Node node2 = nl2.item(j);
              String name2 = node2.getNodeName();

              if (name2.equals("query")) {
                StoredXPathQuery sxpq = new StoredXPathQuery();
                NodeList nl3 = node2.getChildNodes();
                for (int k = 0; k < nl3.getLength(); k++) {
                  Node node3 = nl3.item(k);
                  String name3 = node3.getNodeName();

                  if (name3.equals("name")) {
                    node3 = node3.getFirstChild();
                    sxpq.name = node3.getNodeValue();

                  } else if (name3.equals("type")) {
                    node3 = node3.getFirstChild();
                    sxpq.type = node3.getNodeValue();
		  } else if (name3.equals("auto_named")) {
		    node3 = node3.getFirstChild();
		    sxpq.autoNamed = false;
		    if (node3.getNodeValue().equals("true")) {
		      sxpq.autoNamed = true;
		    }
                  } else if (name3.equals("atomic_query")) {
                    StoredAtomicXPathQuery saxpq = new StoredAtomicXPathQuery();
                    NodeList nl4 = node3.getChildNodes();
                    for (int l = 0; l < nl4.getLength(); l++) {
                      Node node4 = nl4.item(l);
                      String name4 = node4.getNodeName();

                      if (name4.equals("xpath")) {
                        node4 = node4.getFirstChild();
                        saxpq.xpath = node4.getNodeValue();
                      } else if (name4.equals("node")) {
                        node4 = node4.getFirstChild();
                        saxpq.node = node4.getNodeValue();
                        saxpq.xpath = null;
                      } else if (name4.equals("predicate")) {
                        node4 = node4.getFirstChild();
			if (node4 == null) {
			    saxpq.predicate = "";
			    saxpq.xpath = null;
			}
			else {
			    saxpq.predicate = node4.getNodeValue();
			    saxpq.xpath = null;
			}
                      } else if (!name4.equals("#text")) {
                        throw new Exception("Unknown node " + name4);
                      }
                    }
                    sxpq.contents.add(saxpq);
                  } else if (!name3.equals("#text")) {
                    throw new Exception("Unknown node " + name3);
                  }
                }
                queries.add(sxpq);
              } else if (!name2.equals("#text")) {
                throw new Exception("Unknown node " + name2);
              }
            }
          } else if (!name.equals("#text")) {
            throw new Exception("Unknown node " + name);
          }
        }
      } else {
        throw new Exception("Root node is not called browser_config");
      }
    } catch (Exception e) {
      System.err.println("Exception occurred while parsing config file: " + e);
    }
  }

  /** Write the user's config file. Called on exit to save any changes */
  public void writeConfigFile() {

    /* FIXME: probably should build this using DOM rather than just printing it */
    try {
      PrintWriter pw = new PrintWriter(new FileWriter(configFileName), true);

      pw.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
      pw.println("<browser_config>");
      pw.println("  <query_language>" + queryLanguage + "</query_language>");
      pw.println("  <result_handler>" + resultHandler + "</result_handler>");
      pw.println("  <namespace>" + nameSpace + "</namespace>");
      pw.println("  <database_url>" + databaseUrl + "</database_url>");
      pw.println("  <queries>");
      for (int i = 0; i < queries.size(); i++) {
        StoredXPathQuery sxpq = (StoredXPathQuery) queries.elementAt(i);
        pw.println("    " + sxpq.xml);
      }
      pw.println("  </queries>");
      pw.println("</browser_config>");
    } catch (Exception e) {
      System.err.println("Exception occurred while writing config file: " + e);
    }
  }

  /* get/set methods for simple properties */
  /**
   * @return a string specifying the current query language
   */
  public String getQueryLanguage() {
    return queryLanguage;
  }

  /**
   * Sets the current query language
   * 
   * @param ql
   *          a string specifying the new query language
   */
  public void setQueryLanguage(String ql) {
    queryLanguage = ql;
  }

  /**
   * @return a string specifying hostname and port where database is
   */
  public String getDatabaseUrl() {
    return databaseUrl;
  }

  /**
   * @param dbu
   *          hostname and port where database is
   */
  public void setDatabaseUrl(String dbu) {
    databaseUrl = dbu;
  }

  /**
   * @return the classname of the current result handler plugin
   */
  public String getResultHandler() {
    if (resultHandler != null && resultHandler.substring(0, 7).equals("browser")) {
      resultHandler = "uk.ac.ed.epcc.qcdgrid." + resultHandler;
    }
    else{
      resultHandler = 
        "uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers.QCDgridResultHandler";
    }
    return resultHandler;
  }

  /**
   * Sets the current result handler plugin
   * 
   * @param rh
   *          the classname of the new result handler
   */
  public void setResultHandler(String rh) {
    resultHandler = rh;
  }

  /**
   * @return the current namespace (prefix) string
   */
  public String getNamespace() {
    return nameSpace;
  }

  /**
   * Sets the namespace (prefix)
   * 
   * @param ns
   *          the new namespace (prefix) string
   */
  public void setNamespace(String ns) {
    nameSpace = ns;
  }
}
