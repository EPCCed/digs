/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import javax.swing.*;
import java.awt.*;

/**
 * Title:       ChoicePredicateBuilderPanel
 * Description: GUI panel for generating predicates to operate on
 * enumerated type nodes
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class ChoicePredicateBuilderPanel extends PredicateBuilderPanel {

    /** a list of the allowed values for the node */
    private JList choiceList;

    /**
     *  Constructor
     *  @param choices a list of the allowed values for the node
     */
    public ChoicePredicateBuilderPanel(String[] choices) {
	super();

	choiceList = new JList(choices);
	add(choiceList, BorderLayout.CENTER);
    }

    /**
     *  @return the XPath string of this predicate input
     */
    public String getXPath() {
	String xpath = "string(.) = '"+choiceList.getSelectedValue()+"'";

	if (notCheckBox.isSelected()) {
	    xpath = "not("+xpath+")";
	}

	return xpath;
    }

    /**
     *  disables all the GUI elements on this panel
     */
    public void disable() {
	super.disable();

	choiceList.setEnabled(false);
    }

    /**
     *  enables all the GUI elements on this panel
     */
    public void enable() {
	super.enable();

	choiceList.setEnabled(true);
    }

    public boolean setPredicate(String pred) {
	/* FIXME: this should do something more useful, but there doesn't seem a lot
	 * of point since this class is never used yet. */
	return false;
    }
}
