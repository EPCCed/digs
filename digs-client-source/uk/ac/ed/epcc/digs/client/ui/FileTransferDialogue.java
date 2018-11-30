/*
 * This dialogue appears when an upload or download has been requested. It allows the user to:
 *  1. Add a prefix and/or postfix to destination filenames
 *  2. Edit the destination folder path
 *  3. Cancel the transfer
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import java.util.Vector;

public class FileTransferDialogue extends JDialog implements ActionListener {

    private JButton okButton;
    private JButton cancelButton;

    private JTextField destField;
    private JTextField prefixField;
    private JTextField postfixField;

    private JList sourceList;
    private JScrollPane sourceListPane;

    private JLabel sourceLabel;
    private JLabel destLabel;
    private JLabel prefixLabel;
    private JLabel postfixLabel;

    private JLabel noteLabel;

    private JPanel leftPanel;
    private JPanel rightPanel;
    private JPanel toPanel;
    private JPanel prefixPanel;
    private JPanel postfixPanel;
    private JPanel buttonPanel;
    private JPanel flowPanel;

    private Vector sources;

    private String destination;
    private String prefix;
    private String postfix;

    private boolean isPut;

    public FileTransferDialogue(Frame parent, boolean isPut, Vector src, String dest) {
        super(parent, isPut ? "Upload file(s)" : "Download file(s)", true);

        this.isPut = isPut;
        sources = src;

        destination = null;
        prefix = null;
        postfix = null;

	okButton = new JButton("OK");
	okButton.addActionListener(this);
	cancelButton = new JButton("Cancel");
	cancelButton.addActionListener(this);

        destField = new JTextField();
        destField.setText(dest);
        prefixField = new JTextField(10);
        postfixField = new JTextField(10);
	destField.addActionListener(this);
	prefixField.addActionListener(this);
	postfixField.addActionListener(this);

        sourceList = new JList(src);
        sourceListPane = new JScrollPane();
        sourceListPane.getViewport().add(sourceList);
        
        if (!isPut) {
            sourceLabel = new JLabel("Getting files:");
        }
        else {
            sourceLabel = new JLabel("Putting files:");
        }
        destLabel = new JLabel("to:");
        prefixLabel = new JLabel("Add prefix:");
        postfixLabel = new JLabel("Add postfix:");

        /* do layout */
        leftPanel = new JPanel();
        rightPanel = new JPanel();
        leftPanel.setLayout(new BorderLayout());
        rightPanel.setLayout(new GridLayout(4, 1));

        leftPanel.add(sourceLabel, BorderLayout.NORTH);
        leftPanel.add(sourceListPane, BorderLayout.CENTER);

        toPanel = new JPanel();
        prefixPanel = new JPanel();
        postfixPanel = new JPanel();
        buttonPanel = new JPanel();
        toPanel.setLayout(new BorderLayout());
        prefixPanel.setLayout(new FlowLayout());
        postfixPanel.setLayout(new FlowLayout());
        buttonPanel.setLayout(new FlowLayout());

        toPanel.add(destLabel, BorderLayout.NORTH);
        toPanel.add(destField, BorderLayout.CENTER);

        prefixPanel.add(prefixLabel);
        prefixPanel.add(prefixField);
        postfixPanel.add(postfixLabel);
        postfixPanel.add(postfixField);

        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);

        rightPanel.add(toPanel);
        rightPanel.add(prefixPanel);
        rightPanel.add(postfixPanel);
        rightPanel.add(buttonPanel);

	noteLabel = new JLabel("Please note: uploaded files may take some time to appear on the grid");

	flowPanel = new JPanel();
	flowPanel.setLayout(new FlowLayout());
	flowPanel.add(leftPanel);
	flowPanel.add(rightPanel);

        this.getContentPane().setLayout(new BorderLayout());
        this.getContentPane().add(flowPanel, BorderLayout.CENTER);
	if (isPut) {
	    this.getContentPane().add(noteLabel, BorderLayout.SOUTH);
	}

        this.setSize(new Dimension(400, 300));
        pack();
        setLocationRelativeTo(null);
        setVisible(true);
    }

    /*
     * Getters for the editable values. These will return null
     * if the dialogue was cancelled, otherwise a valid (but
     * possibly empty) string
     */
    public String getPrefix() {
        return prefix;
    }

    public String getDestination() {
        return destination;
    }

    public String getPostfix() {
        return postfix;
    }

    public void actionPerformed(ActionEvent e) {
	Object src = e.getSource();
	if ((src == okButton) || (src == destField) || (src == prefixField) || (src == postfixField)) {
            destination = destField.getText();
            prefix = prefixField.getText();
            postfix = postfixField.getText();
            dispose();
        }
        else if (src == cancelButton) {
            dispose();
        }
    }

}
