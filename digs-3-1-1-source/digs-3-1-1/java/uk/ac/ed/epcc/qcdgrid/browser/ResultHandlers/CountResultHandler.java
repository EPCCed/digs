/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.util.*;
import java.io.*;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title:        CountResultHandler
 * Description: A result handler which simply returns a count of 
 * documents in the result set. It's not designed to be used directly,
 * but is used internally by the browser to implement the 'Count'
 * function
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class CountResultHandler {

    private int numDocuments;

    /**
     * @return the number of documents in the result set
     */
    public int getCount() {
	return numDocuments;
    }

    /**
     * @return a description of this result handler
     */
    public String getDescription() {
	return "Counts the documents in the result set";
    }

    public void handleResults(QCDgridQueryResults qr) {
	numDocuments = qr.count();
    }
}
