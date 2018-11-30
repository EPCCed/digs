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
 * Title:        QueryInputDialog
 * Description: The QueryInputDialog allows the user to construct a new
 * query from an arbitrary string.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class QueryInputDialog extends JDialog {

    /** holds the arbitrary query string as specified by the user */
    String queryString = null;
    /** holds the name of the query */
    String queryName = null;

    /** widgets (JBuilder generated) */
    JPanel jPanel1 = new JPanel();
    JLabel jLabel1 = new JLabel();
    JScrollPane jScrollPane1 = new JScrollPane();
    JTextArea jQueryText = new JTextArea();
    JPanel jPanel2 = new JPanel();
    JPanel jPanel3 = new JPanel();
    JButton jCancel = new JButton();
    JButton jOK = new JButton();
    JLabel jLabel2 = new JLabel();
    JTextField jQueryName = new JTextField();
    BorderLayout borderLayout1 = new BorderLayout();
    BorderLayout borderLayout2 = new BorderLayout();
    
    /** Constructor: creates a new query input dialogue */
    public QueryInputDialog() {
	this("", "");
    }

    public QueryInputDialog(String xpath, String name) {
	try {
	    jbInit();
	    pack();
	    jQueryText.setText(xpath);
	    jQueryName.setText(name);
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
    }

    /** Initialises the widgets */
    private void jbInit() throws Exception {
	this.setTitle("New Query");
	jLabel1.setText("Please enter a query string:");
	jQueryText.setPreferredSize(new Dimension(600, 50));
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
	jLabel2.setText("Name:");
	jPanel3.setLayout(borderLayout1);
	jPanel1.setLayout(borderLayout2);
	this.getContentPane().add(jPanel1, BorderLayout.SOUTH);
	jPanel1.add(jPanel3, BorderLayout.NORTH);
	jPanel3.add(jLabel2, BorderLayout.WEST);
	jPanel3.add(jQueryName, BorderLayout.CENTER);
	jPanel1.add(jPanel2, BorderLayout.SOUTH);
	jPanel2.add(jOK, null);
	jPanel2.add(jCancel, null);
	this.getContentPane().add(jLabel1, BorderLayout.NORTH);
	this.getContentPane().add(jScrollPane1, BorderLayout.CENTER);
	jScrollPane1.getViewport().add(jQueryText, null);

	
    }

    /** @return the XPath string of this query */
    public String getQueryString() {
	return queryString;
    }
    
    /** @return the name of this query */
    public String getQueryName() {
	return queryName;
    }
    
    /** Button "OK" */
    void jOK_actionPerformed(ActionEvent e) {
	queryString = jQueryText.getText();
	queryName = jQueryName.getText();
	dispose();
    }

    /** Button "Cancel" */
    void jCancel_actionPerformed(ActionEvent e) {
	queryString = null;
	queryName = null;
	dispose();
    }
}
