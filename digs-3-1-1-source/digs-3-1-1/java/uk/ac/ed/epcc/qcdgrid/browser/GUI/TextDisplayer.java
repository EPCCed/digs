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
 * Title:        TextDisplayer
 * Description: A window for displaying (but not editing) texts.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class TextDisplayer extends JFrame {
    /** widgets, generated by JBuilder */
    JScrollPane jScrollPane1 = new JScrollPane();
    BorderLayout borderLayout1 = new BorderLayout();
    JMenuBar jMenuBar1 = new JMenuBar();
    JMenu jMenuFile = new JMenu();
    JMenuItem jMenuFileExit = new JMenuItem();
    JTextPane jTextPane1 = new JTextPane();
    JMenu jMenuClear = new JMenu();
    JMenuItem jMenuEditClear = new JMenuItem();

    /** constructor: creates a new TextDisplayer with no title or initial text */
    public TextDisplayer() {
	try {
	    jbInit();
	    pack();
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
    }

    /**
     * constructor: creates a new TextDisplayer
     * @param theText the initial content for the text displayer (may be null)
     * @param title the window title for the text displayer (may be null)
     */
    public TextDisplayer(String theText, String title) {
	try {
	    jbInit();
	    if (theText != null) jTextPane1.setText(theText);
	    if (title != null) setTitle(title);
	    pack();
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
    }

    /** 
     *  Appends a string to the contents of the text pane.
     *  @param theText text string to append
     */
    public void append(String theText) {
	try {
	    javax.swing.text.Document doc = jTextPane1.getDocument();
	    doc.insertString(doc.getLength(), theText, null);
	}
	catch (javax.swing.text.BadLocationException e) {
	}
    }

    /** Creates the GUI */
    private void jbInit() throws Exception {
	this.getContentPane().setLayout(borderLayout1);
	jMenuFile.setText("File");
	jMenuFileExit.setText("Exit");
	jMenuFileExit.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jMenuFileExit_actionPerformed(e);
		}
	    });
	jScrollPane1.setPreferredSize(new Dimension(400, 300));
	jTextPane1.setPreferredSize(new Dimension(400, 300));
	jTextPane1.setEditable(false);
	this.setJMenuBar(jMenuBar1);
	jMenuClear.setText("Edit");
	jMenuEditClear.setText("Clear");
	jMenuEditClear.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jMenuEditClear_actionPerformed(e);
		}
	    });
	this.getContentPane().add(jScrollPane1, BorderLayout.CENTER);
	jScrollPane1.getViewport().add(jTextPane1, null);
	jMenuBar1.add(jMenuFile);
	jMenuBar1.add(jMenuClear);
	jMenuFile.add(jMenuFileExit);
	jMenuClear.add(jMenuEditClear);
    }
    
    /** File | Exit */
    void jMenuFileExit_actionPerformed(ActionEvent e) {
	dispose();
    }

    /** Edit | Clear
     *  Clears the contents of the text pane.
     */
    void jMenuEditClear_actionPerformed(ActionEvent e) {
	jTextPane1.setText("");
    }
}
