/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

/**
 * Title:        PredicateInputPanel
 * Description: A panel which allows the user to input a predicate string
 * and a name for an XPath query.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */

public class PredicateInputPanel extends JPanel
{
    JRadioButton jExists = new JRadioButton();
    ButtonGroup predicateButtonGroup = new ButtonGroup();
    GridLayout gridLayout1 = new GridLayout();
    JRadioButton jPredicate = new JRadioButton();
    JPanel jPanel7 = new JPanel();
    JPanel jPanel3 = new JPanel();
    JTextField jTextField1 = new JTextField();
    JLabel jLabel1 = new JLabel();
    FlowLayout flowLayout1 = new FlowLayout();

    /** Added widgets to cope with node-specific auto predicate generation */
    JRadioButton jNodeSpecific = new JRadioButton();
    PredicateBuilderPanel jPanel8;

    String namespace;


    /**
     *  Constructor: creates a predicate input panel for editing an existing predicate
     *  @param pbp type-specific predicate builder panel
     *  @param namespace namespace prefix in use
     *  @param pred current text of predicate
     */
    public PredicateInputPanel(PredicateBuilderPanel pbp, String namespace, String pred) {
	this(pbp, namespace);
	
	/*
	 * Editing an existing predicate: first we try to edit it as a type-
	 * specific predicate, if that fails, just dump it in the text box as
	 * raw XPath
	 */
	jNodeSpecific.setSelected(true);
	jPanel8.enable();
	if (!jPanel8.setPredicate(pred)) {
	    // If the node specific panel couldn't parse the predicate, put it
	    // in the predicate text box instead
	    jPredicate.setSelected(true);
	    jPanel8.disable();
	    jTextField1.setEnabled(true);
	    jLabel1.setEnabled(true);
	    jTextField1.setText(pred);
	}
    }

    /**
     *  Constructor: creates a predicate input panel for entering a new predicate
     *  @param pbp type-specific predicate builder panel
     *  @param namespace namespace prefix in use
     */
    public PredicateInputPanel(PredicateBuilderPanel pbp, String namespace) {
	this.namespace=namespace;
	jPanel8=pbp;

	/* Hack because the Complex panel needs to know the namespace */
	if (pbp instanceof ComplexPredicateBuilderPanel) {
	    ((ComplexPredicateBuilderPanel)pbp).setNamespace(namespace);
	}

	gridLayout1.setRows(3);
	gridLayout1.setColumns(2);
	gridLayout1.setHgap(5);
	gridLayout1.setVgap(5);
	this.setLayout(gridLayout1);

	jExists.setText("exists");
	jExists.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jExists_actionPerformed(e);
		}
	    });
	jExists.setSelected(true);

	jPredicate.setText("enter predicate");
	jPredicate.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jPredicate_actionPerformed(e);
		}
	    });
	jPanel3.setLayout(flowLayout1);
	jTextField1.setMinimumSize(new Dimension(150, 22));
	jTextField1.setPreferredSize(new Dimension(100, 22));
	jTextField1.setEnabled(false);
	jLabel1.setText("Predicate:");
	jLabel1.setEnabled(false);
	jLabel1.setToolTipText("");
	flowLayout1.setAlignment(FlowLayout.LEFT);

	jNodeSpecific.setText("choose predicate");
	jNodeSpecific.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jNodeSpecific_actionPerformed(e);
		}
	    });
	this.add(jExists, null);
	this.add(jPanel7, null);
	this.add(jPredicate, null);
	this.add(jPanel3, null);
	this.add(jNodeSpecific, null);
	this.add(jPanel8, null);

	jPanel3.add(jLabel1, null);
	jPanel3.add(jTextField1, null);
	predicateButtonGroup.add(jExists);
	predicateButtonGroup.add(jPredicate);
	predicateButtonGroup.add(jNodeSpecific);

	jPanel8.disable();
    }
    
    void jPredicate_actionPerformed(ActionEvent e) {
	jTextField1.setEnabled(true);
	jLabel1.setEnabled(true);
	jPanel8.disable();
    }
    
    void jExists_actionPerformed(ActionEvent e) {
	jTextField1.setEnabled(false);
	jLabel1.setEnabled(false);
	jPanel8.disable();
    }

    void jNodeSpecific_actionPerformed(ActionEvent e) {
	jTextField1.setEnabled(false);
	jLabel1.setEnabled(false);
	jPanel8.enable();
    }

    /**
     *  @return the XPath predicate input or edited in this dialogue
     */
    public String getPredicate() {
	String predicate;

	if (jExists.isSelected()) {
	    predicate = "";
	}
	else if (jPredicate.isSelected()) {
	    predicate = jTextField1.getText();
	}
	else {
	    predicate = jPanel8.getXPath();
	}

	return predicate;
    }
}
