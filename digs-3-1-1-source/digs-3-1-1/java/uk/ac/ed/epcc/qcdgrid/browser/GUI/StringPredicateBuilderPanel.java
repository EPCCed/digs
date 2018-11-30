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
 * Title:       StringPredicateBuilderPanel
 * Description: GUI panel for generating predicates to operate on
 * string type nodes
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class StringPredicateBuilderPanel extends PredicateBuilderPanel {

    /** a list of possible comparison operations */
    private JList comparators;

    /** text field to hold the value to compare to */
    private JTextField valueInput;

    /** panel to keep the layout looking nice */
    private JPanel panel;

    /**
     *  Constructor: creates a new one of these panels
     */
    public StringPredicateBuilderPanel() {
	super();

	panel = new JPanel();
	panel.setLayout(new FlowLayout());

	String[] listData = { "equals", "contains", "starts with", "ends with" };
	comparators = new JList(listData);
	panel.add(comparators);

	comparators.setSelectedIndex(0);

	valueInput = new JTextField(10);
	panel.add(valueInput);

	add(panel, BorderLayout.CENTER);
    }

    /**
     *  @return the XPath string of the predicate input
     */
    public String getXPath() {
	String xpath;

	String op = (String)comparators.getSelectedValue();

	if (op.equals("equals")) {
	    xpath = "string(.) = '"+valueInput.getText()+"'";
	}
	else if (op.equals("contains")) {
	    xpath = "contains(., '"+valueInput.getText()+"')";
	}
	else if (op.equals("starts with")) {
	    xpath = "starts-with(., '"+valueInput.getText()+"')";
	}
	else if (op.equals("ends with")) {
	    xpath = "ends-with(., '"+valueInput.getText()+"')";
	}
	else {
	    xpath = "";
	}

	if (notCheckBox.isSelected()) {
	    xpath = "not("+xpath+")";
	}

	return xpath;
    }

    /** disables all the GUI components on this panel */
    public void disable() {
	super.disable();

	comparators.setEnabled(false);
	valueInput.setEnabled(false);
    }

    /** enables all the GUI components on this panel */
    public void enable() {
	super.enable();

	comparators.setEnabled(true);
	valueInput.setEnabled(true);
    }

    /**
     *  Sets the predicate to be edited
     *  @param pred the XPath predicate to edit
     *  @return true if the predicate was set correctly, false if not
     */
    public boolean setPredicate(String pred) {

	/* Deal with a negated predicate first */
	if (pred.substring(0, 4).equals("not(")) {
	    notCheckBox.setSelected(true);
	    pred = pred.substring(4, pred.length()-1);
	}

	/* Work out which comparator was used */
	if (pred.substring(0, 13).equals("string(.) = '")) {
	    comparators.setSelectedIndex(0);
	}
	else if (pred.substring(0, 21).equals("contains(string(.), '")) {
	    comparators.setSelectedIndex(1);
	}
	else if (pred.substring(0, 24).equals("starts-with(string(.), '")) {
	    comparators.setSelectedIndex(2);
	}
	else if (pred.substring(0, 22).equals("ends-with(string(.), '")) {
	    comparators.setSelectedIndex(3);	    
	}
	else {
	    /* This predicate was not input using this panel */
	    return false;
	}

	/* Work out value */
	int valstart = pred.indexOf("'");
	int valend = pred.indexOf("'", valstart+1);
	if ((valstart<0)||(valend<0)) {
	    return false;
	}
	String value = pred.substring(valstart+1, valend);
	valueInput.setText(value);

	return true;
    }
}
