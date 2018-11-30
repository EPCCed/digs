/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

/**
 * Title:        TemplateParserException
 * Description: This exception should be thrown when a fatal error occurs
 * when parsing the template document.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class TemplateParserException extends Exception {

    /**
     *  Constructor: creates a new TemplateParserException
     *  @param msg the message to associate with the exception
     */
    public TemplateParserException(String msg) {
	super(msg);
    }
}
