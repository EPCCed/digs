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
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.Configuration;

/*
 * Title: SettingsDialogue
 * Description: The general settings window for the browser
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
*/
public class SettingsDialogue extends JDialog implements ActionListener {

    /** The configuration that's being edited */
    private Configuration configuration;

    /** Namespace panel widgets */
    private JPanel namespacePanel;
    private JLabel namespaceLabel;
    private JTextField namespaceField;

    /** General dialogue widgets */
    private JButton okButton;
    private JButton cancelButton;
    private JPanel buttonPanel;

    private JPanel mainPanel;

    /**
     * Constructor. Creates a new settings dialogue
     * @param conf the configuration being edited
     * @param owner the main window creating this dialogue
     */
    public SettingsDialogue(Configuration conf, Frame owner) {
	super(owner, "Browser Settings", true);
	
	configuration = conf;

	/* Sort out hierarchy of panels */
	namespacePanel = new JPanel();

	mainPanel = new JPanel();
	mainPanel.setLayout(new GridLayout(1,1));

	buttonPanel = new JPanel();

	getContentPane().setLayout(new BorderLayout());
	getContentPane().add(mainPanel, BorderLayout.CENTER);
	getContentPane().add(buttonPanel, BorderLayout.SOUTH);
	
	mainPanel.add(namespacePanel);

	/* Buttons for OK and Cancel */
	okButton = new JButton("OK");
	okButton.addActionListener(this);
	cancelButton = new JButton("Cancel");
	cancelButton.addActionListener(this);
	buttonPanel.add(okButton);
	buttonPanel.add(cancelButton);

	/* Namespace Panel */
	namespaceLabel = new JLabel("Namespace");
	namespaceField = new JTextField(configuration.getNamespace(), 20);
	namespacePanel.add(namespaceLabel);
	namespacePanel.add(namespaceField);
	pack();
    }

    /** Handles clicks on OK/Cancel buttons */
    public void actionPerformed(ActionEvent e) {
	Object source = e.getSource();

	if (source==okButton) {
	    /* OK button pressed - update configuration with any
	     * changes the user made */
	    configuration.setNamespace(namespaceField.getText());
	    dispose();
	}
	else if (source==cancelButton) {
	    /* Cancel button pressed - just close the dialogue */
	    dispose();
	}
    }
}
