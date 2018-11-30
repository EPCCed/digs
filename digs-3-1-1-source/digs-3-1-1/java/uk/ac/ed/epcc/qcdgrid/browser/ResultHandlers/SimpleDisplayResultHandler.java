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
 * Title: SimpleDisplayResultHandler Description: A result handler which echoes
 * the query results to a text window Copyright: Copyright (c) 2002 Company: The
 * University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class SimpleDisplayResultHandler extends ResultHandler {

  /** The window which the results will appear in */
  private TextDisplayer resultWindow;

  /** Constructor: creates the window */
  public SimpleDisplayResultHandler() {
    resultWindow = new TextDisplayer(null, "Query Results");
  }

  /**
   * Handles the given results, echoing them to the window
   */
  public void handleResults(QCDgridQueryResults qr) {
    String results = qr.combinedResult();
    Date queryTime = Calendar.getInstance().getTime();

    resultWindow.append("-New Query -------" + queryTime + "-\n");
    resultWindow.append(results);
    resultWindow.append("-End Query-------------------------------------\n\n");
    resultWindow.setVisible(true);
  }

  /**
   * @return a text description of this result handler
   */
  public String getDescription() {
    return "Displays query results in a simple text window";
  }
}
