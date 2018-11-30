/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.QueryHandler;

import javax.swing.*;
// import uk.ac.nesc.schemas.gxds.GridDataServiceSOAPBinding;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.*;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title: QueryBuilder Description: The abstract base class for building and
 * handling queries Copyright: Copyright (c) 2002 Company: The University of
 * Edinburgh
 * 
 * @author Amy Krause
 * @version 1.0
 */

public abstract class QueryBuilder {

  /**
   * The list of current queries. Associated with a list box in the main GUI
   * frame
   */
  protected DefaultListModel queryList = new DefaultListModel();

  /** The parent frame of this query builder */
  protected JFrame parentFrame;

  /**
   * Constructor: creates a new query builder
   * 
   * @param j
   *          the parent frame
   */
  public QueryBuilder(JFrame j) {
    parentFrame = j;
  }

  /**
   * @return the current list of queries
   */
  public DefaultListModel getQueryList() {
    return queryList;
  }

  /**
   * Creates a new query
   * 
   * @param namespace
   *          namespace prefix currently in use
   * @return true if operation succeeds, false if it fails
   */
  public boolean newQuery(String namespace) {
    return false;
  }

  /**
   * Edits an existing query
   * 
   * @param jQueryList
   *          list of queries - selection is the one to edit
   * @param namespace
   *          namespace prefix currently in use
   * @return true if query was edited, false if not
   */
  public boolean editQuery(JList jQueryList, String namespace) {
    return false;
  }

  /**
   * Creates a new plain text query
   * 
   * @param q
   *          the query string
   * @param n
   *          the name for the query
   * @return true if operation succeeds, false if it fails
   */
  public boolean newTextQuery(String q, String n) {
    return false;
  }

  /**
   * Combines the queries that are currently selected into a new one
   * 
   * @param jQueryList
   *          the listbox that holds the current selection
   * @return true if operation succeeds, false if it fails
   */
  public boolean combineQueries(JList jQueryList) {
    return false;
  }

  /**
   * Deletes the selected quer[y|ies]
   * 
   * @param jQueryList
   *          the listbox that holds the current selection
   * @return true if operation succeeds, false if it fails
   */
  public boolean deleteQuery(JList jQueryList) {
    return false;
  }

  public void exportLibrary(String filename, JList jQueryList) {
  }

  public void importLibrary(String filename) {
  }

  /**
   * Runs the selected query on the specified data service
   * 
   * @param jQueryList
   *          the listbox that holds the current selection
   * @param dd
   *          the database to connect to
   * @param namespace
   *          the namespace (prefix) to use
   * @return the resulting XML documents one after another, null on failure
   */
  public QCDgridQueryResults runQuery(JList jQueryList, DatabaseDriver dd,
      String namespace) throws Exception {
    return null;
  }
}
