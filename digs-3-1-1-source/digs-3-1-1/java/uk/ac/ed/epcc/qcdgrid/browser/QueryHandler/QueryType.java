package uk.ac.ed.epcc.qcdgrid.browser.QueryHandler;

import java.io.ByteArrayInputStream;
import javax.swing.*;
import javax.swing.tree.*;

import javax.xml.parsers.*;
import org.xml.sax.SAXException;
import org.w3c.dom.*;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.*;
import uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler.SchemaTreeBuilder;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

public class QueryType {

  
  /** Ensemble query type identifier */
  public static final String ENSEMBLE_QUERY_TYPE = "ensemble";
  
  /** Configuration query type identifier */
  public static final String CONFIGURATION_QUERY_TYPE = "gaugeconfig";

  /** Correlator query type identifier */
  public static final String CORRELATOR_QUERY_TYPE = "correlator";
  
  /** Used for any other type of query*/
  public static final String QUERY_TYPE_UNKNOWN = "unknown";
  
  /** Short name of query type */
  private String name;

  /** User-friendly description of query type */
  private String description;

  /** Filename of schema to which this query applies */
  private String schemaName;

  /** Name of the schema's root node */
  private String schemaRootNode;

  /** Tree built from schema */
  private JTree tree;

  private DefaultTreeModel defaultTreeModel;

  /*
   * Will also need result handler and scope descriptors in here later
   */
  private String subQueryScope;

  private String scopePath;

  private String namespace;

  /**
   * Constructor: parses the XML fragment passed in and sets up the query type
   * object correctly
   * 
   * @param qt
   *          XML fragment rooted at queryType
   */
  public QueryType(Element qt) throws QCDgridException {

    /* First get the name attribute */
    name = qt.getAttribute("name");
    if ((name == null) || (name.equals(""))) {
      throw new QCDgridException("Query type has no name!");
    }

    /* Initialise to null so we can catch missing values */
    description = null;
    schemaName = null;
    schemaRootNode = null;
    subQueryScope = null;
    scopePath = null;
    namespace = null;

    /* Loop over sub-elements parsing out useful info */
    NodeList nl = qt.getChildNodes();
    for (int i = 0; i < nl.getLength(); i++) {
      Node node = nl.item(i);
      String nodeName = node.getNodeName();

      if (node instanceof Element) {
        if (nodeName.equals("description")) {
          node = node.getFirstChild();
          description = node.getNodeValue();
        } else if (nodeName.equals("schema")) {
          NodeList nl2 = node.getChildNodes();

          for (int j = 0; j < nl2.getLength(); j++) {
            Node node2 = nl2.item(j);
            String nodeName2 = node2.getNodeName();

            if (node2 instanceof Element) {
              if (nodeName2.equals("fileName")) {
                node2 = node2.getFirstChild();
                schemaName = node2.getNodeValue();
                //System.out.println("Set schema name to " + schemaName);
              } else if (nodeName2.equals("rootNode")) {
                node2 = node2.getFirstChild();
                schemaRootNode = node2.getNodeValue();
              } else {
                throw new QCDgridException("Unknown element " + nodeName2
                    + "in query type " + name + "'s schema");
              }
            }
          }

        } else if (nodeName.equals("defaultScope")) {
	    node = node.getFirstChild();
	    if (node != null) {
		namespace = node.getNodeValue();
		if (namespace != null) {
		    namespace = namespace.trim();
		}
	    }
        } else if (nodeName.equals("scopePath")) {

	    node = node.getFirstChild();
	    scopePath = node.getNodeValue();
	    if (scopePath != null) {
		scopePath = scopePath.trim();
	    }

        } else if (nodeName.equals("resultHandler")) {
	    NodeList nl2 = node.getChildNodes();

	    for (int j = 0; j < nl2.getLength(); j++) {
		Node node2 = nl2.item(j);
		String nodeName2 = node2.getNodeName();

		if (node2 instanceof Element) {
		    if (nodeName2.equals("subQuery")) {
			NodeList nl3 = node2.getChildNodes();
			for (int k = 0; k < nl3.getLength(); k++) {
			    Node node3 = nl3.item(k);
			    String nodeName3 = node3.getNodeName();

			    if (node3 instanceof Element) {
				if (nodeName3.equals("scope")) {

				    if (subQueryScope != null) {
					System.err.println("Warning: multiple sub-query types in " +
							   "query type " + name + "; not yet supported");
				    }

				    node3 = node3.getFirstChild();
				    subQueryScope = node3.getNodeValue();
				}
				else {
				    throw new QCDgridException("Unknown element " + nodeName3
							       + " in query type " + name + "'s schema");
				}
			    }
			}

		    }
		    else {
			// ignore others for now
		    }
		}
	    }
        } else {
          throw new QCDgridException("Unknown element " + nodeName
              + " in queryType " + name);
        }
      }
    }

    /* Check we found everything we need */
    if (description == null) {
      throw new QCDgridException("Query type " + name + " has no description");
    }
    if (schemaName == null) {
      throw new QCDgridException("Query type " + name + " has no schema name");
    }
    if (schemaRootNode == null) {
      throw new QCDgridException("Query type " + name
          + " has no schema root node");
    }

/*    if (subQueryScope != null) {
	System.out.println("Sub-query scope for " + name + ": " + subQueryScope);
	}*/

    /* Now try to read the schema specified */
    QCDgridExistDatabase db = QCDgridExistDatabase.getDriver();
    String schemaText = null;
    try{
      schemaText = db.getSchema(schemaName);
    }
    catch(XMLDBException xdbe){
      throw new QCDgridException("Could not load schema information for " + schemaName 
          + " from the catalogue.");
    }
    if (schemaText == null) {
      throw new QCDgridException("Schema " + schemaName
          + ", needed by query type " + name + ", does not exist in database");
    }

    /* set namespace in database driver for ensemble or configuration */
    if (namespace != null) {
	if (name.equals("ensemble")) {
	    db.setEnsembleNamespace(namespace);
	}
	else if (name.equals("gaugeconfig")) {
	    db.setConfigurationNamespace(namespace);
	}
	else if (name.equals("correlator")) {
	    db.setCorrelatorNamespace(namespace);
	}
    }

    /* Build a tree from the schema */
    byte[] schemaAsBytes = schemaText.getBytes();
    SchemaTreeBuilder stb = new SchemaTreeBuilder(new ByteArrayInputStream(
        schemaAsBytes));
    DefaultMutableTreeNode treeRoot = stb.buildTree(schemaRootNode);

    DefaultMutableTreeNode base = new DefaultMutableTreeNode(schemaName);
    base.add(treeRoot);

    /* Build a tree model for the tree */
    defaultTreeModel = new DefaultTreeModel(base);
    tree = new JTree(defaultTreeModel);
    DefaultTreeSelectionModel s = new DefaultTreeSelectionModel();
    s.setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
    tree.setSelectionModel(s);
  }

  public String getDescription() {
    return description;
  }

  public JTree getTree() {
    return tree;
  }

  public String getName() {
    return name;
  }
  
  public String getSchemaName(){
    return schemaName;
  }

  public String getSubQueryScope() {
      return subQueryScope;
  }

  public String getScopePath() {
      return scopePath;
  }
}
