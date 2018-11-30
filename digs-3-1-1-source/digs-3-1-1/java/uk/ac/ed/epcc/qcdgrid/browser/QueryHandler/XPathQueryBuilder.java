/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.QueryHandler;

import java.io.*;
import javax.swing.*;
import javax.swing.tree.*;
import java.awt.*;
import java.util.Vector;

import javax.xml.parsers.*;
import org.xml.sax.SAXException;
import org.w3c.dom.*;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;
// import uk.ac.nesc.schemas.gxds.*;
// import uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler.SAXTree;
// import java.util.Calendar;
// import java.util.TimeZone;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.*;
import uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers.*;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.UserQueryRunner;

/**
 * Title: XPathQueryBuilder Description: The XPathQuery Builder handles the
 * queries list. This includes constructing new XPath queries from user input,
 * combining and deleting existing XPath queries and sending XPath queries to
 * the XML database service. Copyright: Copyright (c) 2002 Company: The
 * University of Edinburgh
 * 
 * @author Amy Krause
 * @version 1.0
 */

public class XPathQueryBuilder extends QueryBuilder {

  /** the tree holding the template document */
  private QueryTypeManager queryTypes;

  private String scope_ = "";

  public void setScope(String scope) {
      scope_ = scope;
  }

  /**
   * Constructor: constructs a new XPathQueryBuilder
   * 
   * @param j
   *          the parent frame
   * @param templateDocument
   *          the template XML document, as a string
   * @exception Exception
   *              thrown if parsing template doc fails
   */
  public XPathQueryBuilder(JFrame j, QueryTypeManager qtm) throws Exception {
    super(j);
    queryTypes = qtm;
  }

  /**
   * Constructs a new XPathQueryExpression from user input and adds it to the
   * queryList
   * 
   * @param namespace
   *          the namespace prefix currently in use
   * @return true if a query was added, false if not
   */
  public boolean newQuery(String namespace) {
    //System.out.println("XPathQueryBuilder:newQuery");
    /* First select a type for the query */
    QueryTypeSelector qts = new QueryTypeSelector(queryTypes);
    Dimension dlgSize = qts.getPreferredSize();
    Dimension frmSize = parentFrame.getSize();
    Point loc = parentFrame.getLocation();
    qts.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
        (frmSize.height - dlgSize.height) / 2 + loc.y);
    qts.setModal(true);
    qts.show();

    QueryType qt = qts.getSelectedType();
    if(qt==null){
      //Window was closed
      return false;
    }
    /* Now select a node */
    QueryNodeSelector qdlg = new QueryNodeSelector(qt.getTree(), parentFrame,
        namespace);
    dlgSize = qdlg.getPreferredSize();
    frmSize = parentFrame.getSize();
    loc = parentFrame.getLocation();
    qdlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
        (frmSize.height - dlgSize.height) / 2 + loc.y);
    qdlg.setModal(true);
    qdlg.show();
    // if the user didn't select a node or pressed cancel
    if (qdlg.getSelectedNode() == null)
      return false;
    // construct a new query expression from user input
    XPathQueryExpression query = new XPathQueryExpression(qdlg
        .getSelectedNode(), qdlg.getPredicate(), qdlg.getQueryName(), qt
        .getName());
    queryList.addElement(query);
    return true;
  }

  /**
   * Edits an existing query expression
   * 
   * @param jQueryList
   *          list of queries - the selected query is the one to edit
   * @param namespace
   *          the namespace prefix currently in use
   * @return true if the query was edited, false if not
   */
  public boolean editQuery(JList jQueryList, String namespace) {
    //System.out.println("XPathQueryBuilder:editQuery");
    XPathQueryExpression query = (XPathQueryExpression) jQueryList
        .getSelectedValue();
    if (query == null) {
	return false;
    }

    Vector aqs = query.getQuery();
    if (aqs.size() != 1) {
	JOptionPane.showMessageDialog(parentFrame, "Combined queries cannot be edited",
				      "Error", JOptionPane.ERROR_MESSAGE);
	return false;
    }

    Dimension dlgSize, frmSize;
    Point loc;
    String origName = query.getName();
    String newName;

    if (query.isPlainText()) {

	QueryInputDialog dlg = new QueryInputDialog(query.toXPathString(), origName);
	dlgSize = dlg.getPreferredSize();
	frmSize = parentFrame.getSize();
	loc = parentFrame.getLocation();
	dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
			(frmSize.height - dlgSize.height) / 2 + loc.y);
	dlg.setModal(true);
	dlg.show();
	String q = dlg.getQueryString();
	if (q == null) {
	    return false;
	}
	
	query.getQuery().clear();
	query.getQuery().add(
	    new AtomicXPathQueryExpression(q));
	newName = dlg.getQueryName();

    }
    else {
	QueryType qt = queryTypes.queryTypeFromName(query.getType());
	
	QueryNodeSelector qdlg = new QueryNodeSelector(qt.getTree(), parentFrame,
						       namespace, query.toXPathString(), query.toString());
	dlgSize = qdlg.getPreferredSize();
	frmSize = parentFrame.getSize();
	loc = parentFrame.getLocation();
	qdlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
			 (frmSize.height - dlgSize.height) / 2 + loc.y);
	qdlg.setModal(true);
	qdlg.show();
	
	if (qdlg.getSelectedNode() == null)
	    return false;
	
	query.getQuery().clear();
	query.getQuery().add(
	    new AtomicXPathQueryExpression(qdlg.getSelectedNode(), qdlg
					   .getPredicate()));
	
	/*
	 * If query name originally defaulted to the XPath of the query (because
	 * the user didn't enter a name), update it to the new XPath. Unless the
	 * user manually changed it this time.
	 */
	newName = qdlg.getQueryName();
    }
	
    if ((newName.equals(origName)) && (query.isAutoNamed())) {
	query.setName(query.toXPathString(), true);
    }
    else {
	query.setName(newName);
    }

    return true;
  }

  /**
   * construct a new XPathQueryExpression from a text string and a query name
   * and adds the new query to the queryList
   * 
   * @param q
   *          an arbitrary XPath query string
   * @param n
   *          the name of the query
   * @return true
   */
  public boolean newTextQuery() {
    QueryTypeSelector qts = new QueryTypeSelector(queryTypes);
    Dimension dlgSize = qts.getPreferredSize();
    Dimension frmSize = parentFrame.getSize();
    Point loc = parentFrame.getLocation();
    qts.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
        (frmSize.height - dlgSize.height) / 2 + loc.y);
    qts.setModal(true);
    qts.show();

    QueryType qt = qts.getSelectedType();
    if(qt==null){
      //Window was closed
      return false;
    }

    QueryInputDialog dlg = new QueryInputDialog();
    dlgSize = dlg.getPreferredSize();
    dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
        (frmSize.height - dlgSize.height) / 2 + loc.y);
    dlg.setModal(true);
    dlg.show();
    String q = dlg.getQueryString();
    if (q == null) {
	return false;
    }

    //System.out.println("XPathQueryBuilder:newTextQuery");
    XPathQueryExpression query = new XPathQueryExpression(q, dlg.getQueryName(), qt.getName());
    queryList.addElement(query);
    return true;
  }

  /**
   * combines the selected queries in a list to a new XPathQueryExpression
   * 
   * @param jQueryList
   *          a list of queries
   * @return An XPathQueryExpression containing the combined query
   */
  private XPathQueryExpression combine(JList jQueryList) {
    //System.out.println("XPathQueryBuilder:combine");
    int minIndex = jQueryList.getMinSelectionIndex();
    int maxIndex = jQueryList.getMaxSelectionIndex();
    XPathQueryExpression combinedQuery = new XPathQueryExpression();
    XPathQueryExpression firstQueryExpression 
      = (XPathQueryExpression) queryList.get(minIndex);
    String firstType = firstQueryExpression.getType();
    for (int i = minIndex; i <= maxIndex; i++) {
      XPathQueryExpression currentQueryExpression 
        = (XPathQueryExpression) queryList.get(i); 
      if(!currentQueryExpression.getType().equals(firstType)){
        System.out.println("Combining queries of different types is not allowed.");
        
        return null;
      }
      if (jQueryList.isSelectedIndex(i)) {
        combinedQuery.add(currentQueryExpression);
      }
    }
    combinedQuery.setType(firstType);
    return combinedQuery;
  }

  public void importLibrary(String libName) {
    try {
      DocumentBuilderFactory docbuildfact = DocumentBuilderFactory
          .newInstance();
      DocumentBuilder docbuild = docbuildfact.newDocumentBuilder();
      Document dom = docbuild.parse(new File(libName));
      Element root = dom.getDocumentElement();

      if (root.getNodeName().equals("query_library")) {
        NodeList nl = root.getChildNodes();

        for (int i = 0; i < nl.getLength(); i++) {
          Node node = nl.item(i);
          String name = node.getNodeName();

          if (node instanceof Element) {

            /*
             * FIXME: this query XML parsing here actually duplicates similar
             * code in Configuration. Would be neater and more maintainable to
             * factor this out (probably into XPathQueryExpression)
             */
            if (name.equals("query")) {
              Vector atoms = new Vector();
              String queryName = null;
              String queryType = null;

              NodeList nl2 = node.getChildNodes();

              for (int j = 0; j < nl2.getLength(); j++) {
                Node node2 = nl2.item(j);
                String name2 = node2.getNodeName();

                if (node2 instanceof Element) {

                  if (name2.equals("name")) {
                    node2 = node2.getFirstChild();
                    queryName = node2.getNodeValue();
                  } else if (name2.equals("type")) {
                    node2 = node2.getFirstChild();
                    queryType = node2.getNodeValue();
                  } else if (name2.equals("atomic_query")) {
                    String queryNode = null;
                    String queryPredicate = null;
                    String queryXpath = null;
                    AtomicXPathQueryExpression axpqe = null;

                    NodeList nl3 = node2.getChildNodes();

                    for (int k = 0; k < nl3.getLength(); k++) {
                      Node node3 = nl3.item(k);
                      String name3 = node3.getNodeName();

                      if (node3 instanceof Element) {

                        if (name3.equals("xpath")) {
                          node3 = node3.getFirstChild();
                          queryXpath = node3.getNodeValue();
                        } else if (name3.equals("node")) {
                          node3 = node3.getFirstChild();
                          queryNode = node3.getNodeValue();
                        } else if (name3.equals("predicate")) {
                          node3 = node3.getFirstChild();
                          queryPredicate = node3.getNodeValue();
                        } else {
                          throw new QCDgridException("Unknown element " + name
                              + " in library " + libName);
                        }
                      }
                    }

                    if (queryXpath == null) {
                      if ((queryNode == null) || (queryPredicate == null)) {
                        throw new QCDgridException("Error in library "
                            + libName + ": non-raw queries must have both "
                            + "a node and a predicate");
                      }

                      if (queryType == null) {
                        throw new QCDgridException("Error in library "
                            + libName + ": can't determine tree node without "
                            + "type");
                      }

                      axpqe = new AtomicXPathQueryExpression(nodeFromPath(
                          queryNode, queryType), queryPredicate);
                    } else {
                      axpqe = new AtomicXPathQueryExpression(queryXpath);
                    }
                    atoms.add(axpqe);
                  } else {
                    throw new QCDgridException("Unknown element " + name
                        + " in library " + libName);
                  }
                }
              }

              XPathQueryExpression newQuery = new XPathQueryExpression(
                  queryName, atoms, queryType);
              queryList.addElement(newQuery);
            } else {
              throw new QCDgridException("Unknown element " + name
                  + " in library " + libName);
            }
          }
        }
      } else {
        throw new QCDgridException("Root node of " + libName
            + " is not query_library");
      }
    } catch (Exception e) {
      System.err.println("Error importing library " + libName + ": " + e);
    }
  }

  public void exportLibrary(String libName, JList jQueryList) {
    try {
      int minIndex = jQueryList.getMinSelectionIndex();
      int maxIndex = jQueryList.getMaxSelectionIndex();

      PrintWriter pw = new PrintWriter(new FileWriter(libName), true);

      pw.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
      pw.println("<query_library>");

      for (int i = minIndex; i <= maxIndex; i++) {
        if (jQueryList.isSelectedIndex(i)) {
          XPathQueryExpression xpqe = (XPathQueryExpression) queryList.get(i);

          pw.println(xpqe.toXML());
        }
      }

      pw.println("</query_library>");
    } catch (Exception e) {
      System.err.println("Error exporting query library: " + e);
    }
  }

  /**
   * combine selected queries in jQueryList and add the new query to the main
   * queryList
   * 
   * @param jQueryList
   *          a list of queries
   * @return A boolean indicating if combining and adding to the list was
   *         successful
   */
  public boolean combineQueries(JList jQueryList) {
    //System.out.println("XPathQueryBuilder:combineQueries");
    XPathQueryExpression qe = combine(jQueryList); 
    if(qe != null){
      queryList.addElement(qe);
    }
    jQueryList.clearSelection();
    return true;
  }

  /**
   * delete the selected queries in jQueryList
   * 
   * @param jQueryList
   *          the list of selected queries
   * @return a boolean indicating if the delete operation was successful
   */
  public boolean deleteQuery(JList jQueryList) {
    //System.out.println("XPathQueryBuilder:deleteQuery");
    int minIndex = jQueryList.getMinSelectionIndex();
    int maxIndex = jQueryList.getMaxSelectionIndex();
    for (int i = maxIndex; i >= minIndex; i--) {
      if (jQueryList.isSelectedIndex(i)) {
        queryList.removeElementAt(i);
      }
    }
    return true;
  }

  /**
   * converts a path string as saved from a query into a node in the template
   * tree
   * 
   * @param pathString
   *          path to the node, separated with slashes
   * @return actual node of template tree
   */
  public DefaultMutableTreeNode nodeFromPath(String pathString, String type) {

    QueryType qt = queryTypes.queryTypeFromName(type);

    TreeModel model = qt.getTree().getModel();
    DefaultMutableTreeNode node = (DefaultMutableTreeNode) model.getRoot();

    /* skip the template node */
    node = (DefaultMutableTreeNode) node.getChildAt(0);

    int pos = 0;
    int nextSlash;
    String nextStage;

    /* skip leading slash */
    if (pathString.charAt(pos) == '/')
      pos++;

    /* get first stage of path */
    nextSlash = pathString.indexOf('/', pos);
    if (nextSlash == -1)
      nextSlash = pathString.length();
    nextStage = pathString.substring(pos, nextSlash);

    /* check it begins with the root node name */
    if (!node.toString().equals(nextStage)) {
      return null;
    }

    DefaultMutableTreeNode child;
    DefaultMutableTreeNode nextNode;

    /* check we haven't hit the last stage */
    while (nextSlash < pathString.length()) {

      /* get next stage of path */
      pos = nextSlash + 1;
      nextSlash = pathString.indexOf('/', pos);
      if (nextSlash == -1)
        nextSlash = pathString.length();
      nextStage = pathString.substring(pos, nextSlash);

      nextNode = null;

      for (int i = 0; i < node.getChildCount(); i++) {
        child = (DefaultMutableTreeNode) node.getChildAt(i);

        if (child.toString().equals(nextStage)) {
          nextNode = child;
        }
      }

      if (nextNode == null)
        return null;

      node = nextNode;
    }

    return node;
  }

  private String getScopeQuery(String scopePath) {
      String result = "";

      if (!scope_.equals("")) {

	  result = "/" + scopePath + "[string(.) = '" + scope_ + "']";

      }

      return result;
  }

  /**
   * combine the selected queries in jQueryList and send the combined query to
   * the database
   * 
   * @param jQueryList
   *          a list of queries
   * @param runner
   *          driver for the database to query
   * @param namespace
   *          the namespace (prefix) to use
   * @return the results of the query
   */
  public QCDgridQueryResults runUserQuery(JList jQueryList, UserQueryRunner runner,
      String namespace) throws Exception {
    XPathQueryExpression query = combine(jQueryList);
    if(query==null){
      return null;
    }

    ////System.out.println("XPathQueryBuilder:query type is: " + query.getType());
    if (query.isEmpty())
      throw (new Exception("Empty query."));
    // wrap each "atomic" query to retrieve the whole record and not only the
    // node of the tree so we can combine the queries
    // String queryString = query.toXPathString("/self::node()[", "]",
    // namespace);
    String queryString; 

    QueryType qt = queryTypes.queryTypeFromName(query.getType());
    
    String scopePath = qt.getScopePath();

    if ((!scope_.equals("")) && (scopePath != null) && (!scopePath.equals(""))) {
	String scopeQuery = "";
	//System.out.println("Scope path: " + scopePath);
	scopeQuery = getScopeQuery(scopePath);
	//System.out.println("Scope query: " + scopeQuery);
	queryString = query.toXPathString("", "", namespace, scopeQuery);
    }
    else {
	queryString = query.toXPathString("", "", namespace);
    }

    QueryFormDialogue qfd = new QueryFormDialogue(queryString);
    queryString = qfd.getFinalQuery();

    if(query.getType().compareTo(QueryType.CONFIGURATION_QUERY_TYPE)==0){
      System.out.println("Running configuration query " + queryString + ".");
      return runner.runConfigurationQuery(queryString);
    }
    else if(query.getType().compareTo(QueryType.ENSEMBLE_QUERY_TYPE)==0){
      System.out.println("Running ensemble query " + queryString + ".");
      return runner.runEnsembleQuery(queryString);
    }
    else if(query.getType().compareTo(QueryType.CORRELATOR_QUERY_TYPE)==0){
      System.out.println("Running correlator query " +queryString + ".");
      return runner.runCorrelatorQuery(queryString);
    }
    else{
      return null;
    }
  }
}

