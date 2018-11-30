/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.util.*;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title: StdoutResultHandler Description: A result handler which echoes the
 * query results on the standard output Copyright: Copyright (c) 2002 Company:
 * The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class StdoutResultHandler extends ResultHandler {

  /**
   * Handles the given results, echoing them to System.out
   * 
   * @param qr
   *          the results of a query
   */
  public void handleResults(QCDgridQueryResults qr) {
    Date queryTime = Calendar.getInstance().getTime();
    String results = qr.combinedResult();

    System.out.println("-New Query -------" + queryTime + "-\n");
    System.out.println(results);
    System.out.println("-End Query-------------------------------------\n\n");
  }

  /**
   * @return a text description of this result handler
   */
  public String getDescription() {
    return "Echoes query results to stdout";
  }
}
