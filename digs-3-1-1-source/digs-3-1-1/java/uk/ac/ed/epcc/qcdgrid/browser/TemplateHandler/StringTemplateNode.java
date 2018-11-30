/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;

/**
 * Title:       StringTemplateNode
 * Description: A TemplateNode subclass used for all nodes which are of
 * string types.
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class StringTemplateNode extends TemplateNode {

    /**
     *  Constructor
     *  @param name the name of the element that this node represents
     *  @param type the name of the element's type
     */
    public StringTemplateNode(String name, String type) {
	super(name, type);
    }

    /**
     *  @return a predicate builder panel for building
     *  predicates for string type elements
     */
    public PredicateBuilderPanel getPredicateBuilderPanel() {
	return new StringPredicateBuilderPanel();
    }
}
