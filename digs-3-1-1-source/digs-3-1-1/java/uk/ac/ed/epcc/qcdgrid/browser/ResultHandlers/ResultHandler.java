/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.util.*;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title:       ResultHandler
 * Description: An abstract base class for handling results returned
 * by queries. The idea is that concrete subclasses can be created
 * to handle results in an application-specific way
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public abstract class ResultHandler {
    /**
     * Handles the results of a query
     */
    public abstract void handleResults(QCDgridQueryResults qr);

    /**
     * @return a text description of this result handler, for the
     * plugin detector
     */
    public abstract String getDescription();
}
