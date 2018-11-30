/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;

/**
 * Title:       TemplateNode
 * Description: A TemplateNode is the user object stored in the template
 * tree for each element. It encapsulates the element's name and type, and
 * it also takes care of creating a predicate builder panel if the
 * element is selected.
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
abstract public class TemplateNode {

    /** name of node */
    private String nodeName;

    /** type given in the schema */
    private String typeName;

    /** 
     *  Constructor
     *  @param name the name of the element this represents
     *  @param type the type name of the element
     */
    protected TemplateNode(String name, String type) {
	nodeName = name;
	typeName = type;
    }

    /**
     *  @return the element's type name
     */
    public String getTypeName() {
	return typeName;
    }

    /**
     *  @return the node's name
     */
    public String getNodeName() {
	return nodeName;
    }

    /**
     *  @return a PredicateBuilderPanel that can be used in a GUI for
     *  building predicates for this node
     */
    abstract public PredicateBuilderPanel getPredicateBuilderPanel();

    /**
     *  @return a string representation of the node, its name
     */
    public String toString() {
	return nodeName;
    }
}
