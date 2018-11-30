/*
 * Copyright (c) 2004 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

/**
 * Title:       SchemaSelector
 * Description: A dialog box which allows the user to select a schema
 *              from those stored in the database
 * Copyright:   Copyright (c) 2004
 * Company:     The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */


public class SchemaSelector extends JDialog {
    
    /** widgets */
    JPanel jPanel2 = new JPanel();
    JButton jOK = new JButton();
    JScrollPane jScrollPane1 = new JScrollPane();
    JList jSchemaList;
    JLabel jLabel1 = new JLabel();

    /** the name of the schema selected */
    String selectedSchema = null;

    /**
     *  class constructor
     *  Creates a new schema selection dialogue
     *  @param schemata contains all the schema names to choose from
     */
    public SchemaSelector(Vector schemata) throws Exception {
	jSchemaList = new JList(schemata);
	jbInit();
	pack();
    }

    /** creates the GUI */
    private void jbInit() throws Exception {
	jOK.setText("OK");
	jOK.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jOK_actionPerformed(e);
		}
	    });
	jLabel1.setText("Please choose a schema:");
	jSchemaList.setSelectedIndex(0);
	jScrollPane1.setPreferredSize(new Dimension(200, 200));
	this.getContentPane().add(jPanel2, BorderLayout.SOUTH);
	jPanel2.add(jOK, null);
	this.getContentPane().add(jScrollPane1, BorderLayout.CENTER);
	jScrollPane1.getViewport().add(jSchemaList, null);
	this.getContentPane().add(jLabel1, BorderLayout.NORTH);
    }

    /** OK button action event handler */
    void jOK_actionPerformed(ActionEvent e) {
	selectedSchema = (String)jSchemaList.getSelectedValue();
	dispose();
    }

    /** @return the name of the schema selected */
    public String getSelectedSchema() {
	return selectedSchema;
    }
}
