/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title: QCDgridResultHandler Description: A result handler specifically for
 * QCDgrid. Creates a QCDgridFrame window which provides the functionality.
 * Copyright: Copyright (c) 2003 Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class QCDgridResultHandler extends ResultHandler {

  /**
   * Handles the results of a query
   * 
   * @param res
   *          the query results object
   */
  public void handleResults(QCDgridQueryResults res) {
    System.out.println("Query returned " + res.count() + " results");
    QCDgridFrame f = new QCDgridFrame(res);
  }

  /**
   * @return a description of the result handler
   */
  public String getDescription() {
    return "QCD-specific result handler GUI";
  }
}
