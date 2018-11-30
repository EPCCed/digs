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
import java.util.*;
//import uk.ac.nesc.schemas.gxds.Constants;

/**
 * Title:        QueryNotationSelector
 * Description: A dialog box which allows the user to select a query notation
 * from a list
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class QueryNotationSelector extends JDialog {
    /** widgets */
    JPanel jPanel2 = new JPanel();
    JButton jOK = new JButton();
    JScrollPane jScrollPane1 = new JScrollPane();
    Vector queryNotationList = new Vector();
    JList jQueryNotationList;
    JLabel jLabel1 = new JLabel();

    /** the name of the currently selected query language */
    String selectedLanguage = null;

    /** class constructor
     *  Compares the list of query notations supported by the service with the
     *  available query notations of this client and constructs a list from the
     *  intersection. An exception is thrown if the intersection is empty.
     *  @param queryNotationsServer A vector holding the query notations supported
     *  by the XML database service
     */
    public QueryNotationSelector(Vector queryNotationsServer) throws Exception {
	if (queryNotationsServer.contains("http://www.w3.org/TR/1999/REC-xpath-19991116"))
	    queryNotationList.addElement("XPath");
	// add other supported query notations here...
	else throw new Exception("Server and client do not support common query languages.");
	jQueryNotationList = new JList(queryNotationList);
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
	jLabel1.setText("Please choose a query notation language:");
	jQueryNotationList.setSelectedIndex(0);
	jScrollPane1.setPreferredSize(new Dimension(200, 200));
	this.getContentPane().add(jPanel2, BorderLayout.SOUTH);
	jPanel2.add(jOK, null);
	this.getContentPane().add(jScrollPane1, BorderLayout.CENTER);
	jScrollPane1.getViewport().add(jQueryNotationList, null);
	this.getContentPane().add(jLabel1, BorderLayout.NORTH);
    }

    /** @return A string representation of the selected query notation */
    public String getSelectedLanguage() {
	return selectedLanguage;
    }

    /** OK button action event handler */
    void jOK_actionPerformed(ActionEvent e) {
	selectedLanguage = (String)jQueryNotationList.getSelectedValue();
	dispose();
    }
}
