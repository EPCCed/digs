/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;

/**
 * Title:       NumberTemplateNode
 * Description: A TemplateNode subclass used for all nodes which are of
 * numeric types.
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class NumberTemplateNode extends TemplateNode {

    /**
     *  Constructor
     *  @param name the name of the element this node represents
     *  @param type the type name of the element
     */
    public NumberTemplateNode(String name, String type) {
	super(name, type);
    }

    /**
     *  @return a predicate builder panel for building predicates for
     *  numeric nodes
     */
    public PredicateBuilderPanel getPredicateBuilderPanel() {
	return new NumberPredicateBuilderPanel();
    }
}
