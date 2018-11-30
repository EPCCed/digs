/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;

/**
 * Title:       ChoiceTemplateNode
 * Description: A TemplateNode subclass used for all nodes which have
 * their possible values enumerated
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class ChoiceTemplateNode extends TemplateNode {

    /** the possible values of this node */
    private String[] choices;

    /**
     *  Constructor
     *  @param name the name of the element that this node represents
     *  @param type the type of the element
     *  @param choices the possible values this node can take
     */
    public ChoiceTemplateNode(String name, String type, String[] choices) {
	super(name, type);
	this.choices = choices;
    }

    /**
     *  @return a predicate builder panel for generating predicates for
     *  enumerated type nodes
     */
    public PredicateBuilderPanel getPredicateBuilderPanel() {
	return new ChoicePredicateBuilderPanel(choices);
    }
}
