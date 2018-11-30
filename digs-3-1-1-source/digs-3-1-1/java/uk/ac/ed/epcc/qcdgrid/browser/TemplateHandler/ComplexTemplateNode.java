/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import uk.ac.ed.epcc.qcdgrid.browser.GUI.*;

import javax.swing.tree.*;

/**
 * Title:       ComplexTemplateNode
 * Description: A TemplateNode subclass used for all nodes which have
 * subnodes
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class ComplexTemplateNode extends TemplateNode {

    /** reference to the position of this node in the tree */
    private DefaultMutableTreeNode node;

    /**
     *  Constructor
     *  @param name the name of the element which this node represents
     *  @param type the type of the element
     */
    public ComplexTemplateNode(String name, String type) {
	super(name, type);
    }

    /**
     *  Sets the tree position reference in this template. It can't be
     *  set in the constructor, because this object is created before
     *  the node is
     *  @param newNode reference to the position of this node in the tree
     */
    public void setNode(DefaultMutableTreeNode newNode) {
	node = newNode;
    }

    /**
     *  @return a predicate builder panel for building predicates for
     *  complex nodes
     */
    public PredicateBuilderPanel getPredicateBuilderPanel() {
	return new ComplexPredicateBuilderPanel(node);
    }
}
