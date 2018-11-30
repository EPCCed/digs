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
import java.util.Vector;

import uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler.*;

/**
 * Title:        PredicateInputDialog
 * Description: A dialog box which allows the user to input a predicate string
 * and a name for an XPath query.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class PredicateInputDialog extends JDialog {

    /** holds the predicate string of the query */
    String predicate = null;
    /** holds the name of the query */
    String queryName = null;

    /** JBuilder widgets */
    JPanel jPanel1 = new JPanel();
    BorderLayout borderLayout1 = new BorderLayout();
    JPanel jPanel2 = new JPanel();
    JButton jCancel = new JButton();
    JButton jOK = new JButton();
    BorderLayout borderLayout2 = new BorderLayout();
    JPanel jPanel5 = new JPanel();
    JLabel jLabel2 = new JLabel();
    JTextField jTextField2 = new JTextField();
    FlowLayout flowLayout2 = new FlowLayout();

    JPanel jPanel9 = new JPanel();  // goes in the middle of the top level

    JPanel andOrPanel = new JPanel();
    JButton andButton = new JButton();
    JButton orButton = new JButton();
    GridLayout andOrLayout = new GridLayout();

    JScrollPane jsp;


    /** Holds all the PredicateInputPanels in this dialogue */
    Vector inputPanels = new Vector();

    /** Holds 'and' and 'or' strings that link the PredicateInputPanels */
    Vector combiners = new Vector();

    /** Holds delete buttons for all the panels except the first one */
    Vector deleteButtons = new Vector();

    /** Current namespace prefix in use */
    String namespace;

    /** Whether this dialogue allows query name input */
    boolean nameInput;
    
    /** Node of template tree for which predicate is being input */
    TemplateNode templateNode;

    /**
     *  Gets the index of the end quote matching the specified beginning quote
     *  @param str the string to search in
     *  @param idx the index of the start quote
     *  @return the index of the matching end quote
     */
    private int findMatchingQuote(String str, int idx) {
	idx++;

	while (idx<str.length()) {
	    if (str.charAt(idx)=='\'') {
		return idx;
	    }

	    idx++;
	}
	return -1;
    }

    /**
     *  Gets the index of the closed bracket matching the specified open bracket
     *  @param str the string to search in
     *  @param idx the index of the open bracket
     *  @return the index of the matching closed bracket
     */
    private int findMatchingBracket(String str, int idx) {
	idx++;
	int depth=0;

	while (idx<str.length()) {
	    if (str.charAt(idx)=='[') {
		depth++;
	    }
	    if (str.charAt(idx)=='(') {
		depth++;
	    }
	    if (str.charAt(idx)==']') {
		depth--;
	    }
	    if (str.charAt(idx)==')') {
		depth--;
	    }
	    if (depth<0) {
		return idx;
	    }

	    idx++;
	}
	return -1;
    }

    /**
     *  Parses the XPath predicate, building the correct GUI for editing it.
     *  @param pred the predicate to parse
     *  @param tn the template node of the current position in the document tree
     *  @param namespace the namespace prefix in use
     */
    private void parsePredicate(String pred, TemplateNode tn, String namespace) {
	int opAt = -1;
	int pos = 0;
	boolean isAnd = false;

	while (pos<pred.length()) {
	    if ((pos<(pred.length()-5))&&(pred.substring(pos, pos+5).equals(" and "))) {
		opAt = pos;
		isAnd = true;
		break;
	    }
	    if ((pos<(pred.length()-4))&&(pred.substring(pos, pos+4).equals(" or "))) {
		opAt = pos;
		isAnd = false;
		break;
	    }
	    if (pred.charAt(pos)=='\'') {
		pos = findMatchingQuote(pred, pos);
		if ((pos<0)||(pos>=pred.length())) {
		    break;
		}
	    }
	    if (pred.charAt(pos)=='[') {
		pos = findMatchingBracket(pred, pos);
		if ((pos<0)||(pos>=pred.length())) {
		    break;
		}
	    }
	    if (pred.charAt(pos)=='(') {
		pos = findMatchingBracket(pred, pos);
		if ((pos<0)||(pos>=pred.length())) {
		    break;
		}
	    }
	    pos++;
	}

	if (opAt<0) {
	    // No operator found - it's just a simple predicate
	    PredicateInputPanel panel = new PredicateInputPanel(tn.getPredicateBuilderPanel(), namespace, pred);
	    inputPanels.add(panel);
	}
	else {
	    // Contains an 'and' or an 'or' - create panel to deal with LHS of predicate, call
	    // self recursively to parse RHS
	    if (isAnd) {
		PredicateInputPanel panel = new PredicateInputPanel(tn.getPredicateBuilderPanel(),
								    namespace, pred.substring(0, opAt));
		inputPanels.add(panel);
		combiners.add("and");

		parsePredicate(pred.substring(opAt+5), tn, namespace);
	    }
	    else {
		PredicateInputPanel panel = new PredicateInputPanel(tn.getPredicateBuilderPanel(),
								    namespace, pred.substring(0, opAt));
		inputPanels.add(panel);
		combiners.add("or");

		parsePredicate(pred.substring(opAt+4), tn, namespace);
	    }
	}
    }

    /** 
     *  Constructor: creates a new predicate input dialogue
     *  @param tn the template tree node for which a predicate is being input
     *  @param title the title for the dialogue
     *  @param namespace the namespace prefix in use
     *  @param name whether to allow the query name to be input as well as the predicate
     *  @param pred an existing predicate to edit, null if none
     *  @param qname the existing query name, null if none
     */
    public PredicateInputDialog(TemplateNode tn, String title, String namespace, boolean name, String pred, String qname) {
	super((Frame)null, title);
	this.namespace = namespace;
	nameInput = name;

	templateNode=tn;

	if (pred!=null) {
	    // Editing an existing predicate
	    predicate = pred;

	    // This is probably the right place to parse out 'and's and 'or's in the predicate string
	    parsePredicate(pred, tn, namespace);
	}
	else {
	    // Creating a new predicate
	    PredicateInputPanel firstPanel = new PredicateInputPanel(tn.getPredicateBuilderPanel(), namespace);
	    inputPanels.add(firstPanel);
	}

	try {
	    jbInit();
	}
	catch(Exception e) {
	    e.printStackTrace();
	}

	if ((name)&&(qname!=null)) {
	    jTextField2.setText(qname);
	}

	pack();
    }

    /** Adds all the predicate input panels (and the 'and' and 'or' labels between them) to the GUI */
    private void displayInputPanels() {

	jPanel9.removeAll();
	deleteButtons.clear();

	for (int i=0; i<inputPanels.size(); i++) {

	    // This panel has the predicate input panel in the centre and the 'delete'
	    // button for it in the south
	    JPanel jp = new JPanel();
	    jp.setLayout(new BorderLayout());

	    // This panel has the delete button within a flow layout for better layout
	    JPanel dbp = new JPanel();
	    dbp.setLayout(new FlowLayout());
	    
	    // This is a delete button for the panel. The first panel doesn't have
	    // one, as there must always be at least one panel
	    if (i!=0) {
		JButton db = new JButton("Delete");
		db.addActionListener(new java.awt.event.ActionListener() {
			public void actionPerformed(ActionEvent e) {
			    jDelete_actionPerformed(e);
			}
		    });
		dbp.add(db);
		deleteButtons.add(db);
	    }

	    PredicateInputPanel p = (PredicateInputPanel)inputPanels.elementAt(i);

	    jp.add(p, BorderLayout.CENTER);
	    jp.add(dbp, BorderLayout.SOUTH);

	    jPanel9.add(jp);
	    if (i!=inputPanels.size()-1) {
		JLabel lbl = new JLabel((String)combiners.elementAt(i));
		jPanel9.add(lbl);
	    }
	}
    }

    /** Initialises the widgets */
    private void jbInit() throws Exception {
	this.getContentPane().setLayout(borderLayout1);
	jCancel.setText("Cancel");
	jCancel.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jCancel_actionPerformed(e);
		}
	    });
	jOK.setText("OK");
	jOK.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jOK_actionPerformed(e);
		}
	    });
	jPanel1.setLayout(borderLayout2);
	jLabel2.setText("Query Name:");
	jTextField2.setMinimumSize(new Dimension(150, 22));
	jTextField2.setPreferredSize(new Dimension(200, 22));
	jPanel5.setLayout(flowLayout2);
	flowLayout2.setAlignment(FlowLayout.LEFT);

	jPanel9.setLayout(new FlowLayout());
	this.getContentPane().add(jPanel1, BorderLayout.CENTER);

	jsp=new JScrollPane(jPanel9);

	jPanel1.add(jsp, BorderLayout.CENTER);
	displayInputPanels();

	if (nameInput) {
	    jPanel1.add(jPanel5, BorderLayout.SOUTH);
	    jPanel5.add(jLabel2, null);
	    jPanel5.add(jTextField2, null);
	}

	this.getContentPane().add(jPanel2, BorderLayout.SOUTH);
	jPanel2.add(jOK, null);
	jPanel2.add(jCancel, null);

	andButton.setText("AND");
	andButton.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jAnd_actionPerformed(e);
		}
	    });
	orButton.setText("OR");
	orButton.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jOr_actionPerformed(e);
		}
	    });
	andOrLayout.setRows(2);
	andOrLayout.setColumns(1);
	andOrLayout.setHgap(32);
	andOrPanel.setLayout(andOrLayout);
	andOrPanel.add(andButton);
	andOrPanel.add(orButton);
	jPanel1.add(andOrPanel, BorderLayout.EAST);
    }
    
    /** Checks if a string is valid as a predicate,
     *  i.e. for each opened bracket there is a corresponding closed bracket.
     *  @param s the string to validate
     *  @return true if the predicate is valid
     */
    private boolean isValid(String s) {
	int bracketcount = 0;
	for (int i=0; i<s.length(); i++) {
	    if (s.charAt(i) == '[') bracketcount++;
	    else if (s.charAt(i) == ']') bracketcount--;
	    if (bracketcount <0) return false;
	}
	if (bracketcount != 0) return false;
	return true;
    }
    
    /** @return the predicate string as given by the user */
    public String getPredicate() {
	return predicate;
    }
    
    /** @return the name of the query as given by the user*/
    public String getQueryName() {
	return queryName;
    }
    
    /** event handlers for the buttons and check boxes */
    void jOK_actionPerformed(ActionEvent e) {

	predicate="";

	for (int i=0; i<inputPanels.size(); i++) {
	    PredicateInputPanel p = (PredicateInputPanel)inputPanels.elementAt(i);
	    predicate += p.getPredicate();
	    if (i!=inputPanels.size()-1) {
		predicate += " "+(String)combiners.elementAt(i)+" ";
	    }
	}

	queryName = jTextField2.getText();
	if (!isValid(predicate)) {
	    JOptionPane.showMessageDialog(null,
					  "The predicate is not valid.",
					  "Predicate Input",
					  JOptionPane.ERROR_MESSAGE);
	}
	else dispose();
	//System.out.println("Predicate: " + predicate + ", queryName: " + queryName);
    }
    
    void jCancel_actionPerformed(ActionEvent e) {
	predicate = null;
	queryName = null;
	dispose();
    }

    void jAnd_actionPerformed(ActionEvent e) {
	PredicateInputPanel p = new PredicateInputPanel(templateNode.getPredicateBuilderPanel(), namespace);
	inputPanels.add(p);
	combiners.add("and");
	displayInputPanels();
	pack();
    }

    void jOr_actionPerformed(ActionEvent e) {
	PredicateInputPanel p = new PredicateInputPanel(templateNode.getPredicateBuilderPanel(), namespace);
	inputPanels.add(p);
	combiners.add("or");
	displayInputPanels();
	pack();
    }

    void jDelete_actionPerformed(ActionEvent e) {
	JButton db = (JButton)e.getSource();
	
	// Find out which delete button generated the event
	for (int i=0; i<deleteButtons.size(); i++) {
	    JButton db2 = (JButton)deleteButtons.elementAt(i);
	    if (db==db2) {
		combiners.removeElementAt(i);
		inputPanels.removeElementAt(i+1);
		displayInputPanels();
		pack();
		return;
	    }
	}
    }
}
