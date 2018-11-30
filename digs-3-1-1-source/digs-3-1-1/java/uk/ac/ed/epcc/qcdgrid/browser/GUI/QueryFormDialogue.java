/*
 * Copyright (c) 2004 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.*;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;

/**
 * Title: QueryFormDialogue
 * Description: A dialogue which allows the user to fill in editable
 *              values into an XPath query
 * Copyright:   Copyright (c) 2004
 * Company:     The University of Edinburgh
 * @author James Perry
 * @version 1.0
*/

public class QueryFormDialogue extends JDialog implements ActionListener {

    /**
     *  Parses the query string looking for editable values (editable values
     *  consist of a name between two '%' signs, which is replaced by whatever
     *  the user enters before the query is run)
     *  @param query XPath query string to parse
     *  @return an array of the editable values' names, null if none
     */
    private String[] findEditableValues(String query) throws QCDgridException {
	int i, ed;
	int numEditables=0;
	String[] result;

	// count number of editable values. Each is prefixed and suffixed by
	// '%', so half the number of '%' signs
	for (i=0; i<query.length(); i++) {
	    if (query.charAt(i)=='%') {
		numEditables++;
	    }
	}

	// must be an even number or something's gone wrong
	if ((numEditables&1)!=0) {
	    throw new QCDgridException("Malformed query, extra `%' symbol");
	}

	// if none, return a null array
	if (numEditables==0) {
	    return null;
	}

	numEditables=numEditables/2;
	result=new String[numEditables];

	// now actually extract them
	int startIdx=-1;
	int endIdx=-1;

	ed=0;
	for (i=0; i<query.length(); i++) {
	    if (query.charAt(i)=='%') {
		if (startIdx<0) {
		    startIdx=i;
		}
		else {
		    endIdx=i;

		    result[ed]=query.substring(startIdx+1, endIdx);
		    ed++;

		    startIdx=-1;
		    endIdx=-1;
		}
	    }
	}
	
	return result;
    }

    /**
     *  The initial query, with editable values
     */
    private String templateQuery;

    /**
     *  The final query, with the editable values replaced by whatever
     *  the user entered
     */
    private String finalQuery;

    /**
     *  An array of the editable values' names, in the order in which
     *  they appear in the query
     */
    private String[] editables;

    /**
     *  Array of text fields, one for editing each editable value
     */
    private JTextField[] editableBoxes;

    /**
     *  GUI widgets
     */
    private JPanel mainPanel;
    private JPanel buttonPanel;

    private JButton okButton;
    private JButton cancelButton;

    /**
     *  Parses the template query and substitutes in what the user entered
     *  for the editable values
     *  @return the final query, ready to run
     */
    private String createFinalQuery() {
	int i;
	int ed;
	int startIdx;
	int endIdx;
	int lastEndIdx;
	String fq;

	ed=0;
	startIdx=-1;
	endIdx=-1;
	lastEndIdx=0;
	
	fq="";

	for (i=0; i<templateQuery.length(); i++) {
	    if (templateQuery.charAt(i)=='%') {
		if (startIdx<0) {
		    startIdx=i;
		}
		else {
		    endIdx=i;

		    fq=fq+templateQuery.substring(lastEndIdx, startIdx)+
			editableBoxes[ed].getText();
		    ed++;

		    lastEndIdx=endIdx+1;
		    startIdx=-1;
		    endIdx=-1;
		}
	    }
	}

	if (lastEndIdx<templateQuery.length()) {
	    fq = fq+templateQuery.substring(lastEndIdx);
	}

	return fq;
    }

    /**
     *  Constructor: creates a modal dialogue box where the user can
     *  enter values to substitute into the template query. If the
     *  given query has no editable values, the dialogue destroys
     *  itself straight away and the user never sees it
     *  @param query an XPath query string, possibly containing
     *               editable values
     */
    public QueryFormDialogue(String query) throws QCDgridException {
	super((Frame)null, "Query Form...", true);

	templateQuery=query;
	finalQuery=null;

	editables = findEditableValues(query);

	if (editables==null) {
	    // no editable values in this query
	    finalQuery=query;
	    dispose();
	    return;
	}

	editableBoxes = new JTextField[editables.length];
	for (int i=0; i<editableBoxes.length; i++) {
	    editableBoxes[i]=new JTextField();
	}

	mainPanel = new JPanel();
	mainPanel.setLayout(new GridLayout(editables.length, 2));
	
	for (int i=0; i<editableBoxes.length; i++) {
	    mainPanel.add(new JLabel(editables[i]+":"));
	    mainPanel.add(editableBoxes[i]);
	}

	buttonPanel = new JPanel();

	okButton = new JButton("OK");
	okButton.addActionListener(this);
	buttonPanel.add(okButton);

	cancelButton = new JButton("Cancel");
	cancelButton.addActionListener(this);
	buttonPanel.add(cancelButton);

	this.getContentPane().setLayout(new BorderLayout());

	this.getContentPane().add(mainPanel, BorderLayout.CENTER);
	this.getContentPane().add(buttonPanel, BorderLayout.SOUTH);

	pack();
	setVisible(true);
    }

    /**
     *  @return the final query, with all editable values substituted
     */
    public String getFinalQuery() {
	return finalQuery;
    }

    /**
     *  Event handler for the OK and cancel buttons
     *  @param e an event generated by the user clicking the OK or
     *           cancel button
     */
    public void actionPerformed(ActionEvent e) {
	Object source = e.getSource();

	if (source==okButton) {
	    finalQuery = createFinalQuery();
	    dispose();
	}
	else if (source==cancelButton) {
	    dispose();
	}
    }
}
