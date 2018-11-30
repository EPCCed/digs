/*
 * Modal dialogue used to get user's OMERO credentials if necessary
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class OMEROLoginDialogue extends JDialog implements ActionListener {

    JButton okButton;
    JButton cancelButton;

    JTextField usernameField;
    JPasswordField passwordField;

    JPanel buttonPanel;
    JPanel fieldsPanel;

    String username;
    char[] password;

    public OMEROLoginDialogue(JFrame parent) {
        super(parent, "Please enter your OMERO credentials", true);

        okButton = new JButton("OK");
        okButton.addActionListener(this);
        cancelButton = new JButton("Cancel");
        cancelButton.addActionListener(this);
        buttonPanel = new JPanel();
        buttonPanel.setLayout(new FlowLayout());
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);

        usernameField = new JTextField();
        passwordField = new JPasswordField(40);
        fieldsPanel = new JPanel();
        fieldsPanel.setLayout(new GridLayout(4, 1));
        fieldsPanel.add(new JLabel("Username:"));
        fieldsPanel.add(usernameField);
        fieldsPanel.add(new JLabel("Password:"));
        fieldsPanel.add(passwordField);
        passwordField.addActionListener(this);

        this.getContentPane().setLayout(new BorderLayout());
        this.getContentPane().add(fieldsPanel, BorderLayout.CENTER);
        this.getContentPane().add(buttonPanel, BorderLayout.SOUTH);

        username = null;
        password = null;

        pack();

        setLocationRelativeTo(null);
        setVisible(true);
    }

    public void actionPerformed(ActionEvent e) {
        Object source = e.getSource();
        if ((source == okButton) || (source == passwordField)) {
            username = usernameField.getText();
            password = passwordField.getPassword();
            dispose();
        }
        else if (source == cancelButton) {
            dispose();
        }
    }

    /*
     * Getters for username and password. These return null if the
     * user cancelled the dialogue.
     */
    public String getUsername() {
        return username;
    }
    public char[] getPassword() {
        return password;
    }
}
