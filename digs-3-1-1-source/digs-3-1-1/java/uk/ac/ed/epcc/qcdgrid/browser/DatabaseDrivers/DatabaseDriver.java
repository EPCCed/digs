/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers;

import java.util.Vector;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title: DatabaseDriver
 * Description:  An abstract class representing the interface to
 * a database of some kind. Concrete subclasses will handle the
 * details of specific database types.
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
*/
abstract public class DatabaseDriver {

    /**
     *  Initialises the driver, connecting to the database
     *  @param url hostname/port of database
     */
    abstract public void init(String url, String username, String password) throws Exception;

    /**
     *  Runs an XPath query on the database
     *  @param query the query to run
     *  @return the results returned by the query
     */
    abstract public QCDgridQueryResults runQuery(String query) throws Exception;

    /**
     *  @return the schema document from the database
     */
    abstract public String getSchema() throws Exception;

    /**
     *  @return a vector of supported query notations
     */
    abstract public Vector getQueryNotations() throws Exception;

    abstract public void setScope(String sc) throws Exception;
}
