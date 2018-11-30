package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class ReplaceFileDialogue extends JDialog implements ActionListener {

    JLabel label;

    JButton yesButton;
    JButton yesToAllButton;
    JButton noButton;
    JButton noToAllButton;

    JPanel buttonPanel;

    int result;

    public static final int YES = 0;
    public static final int YES_TO_ALL = 1;
    public static final int NO = 2;
    public static final int NO_TO_ALL = 3;

    public ReplaceFileDialogue(JFrame parent, String lfn) {
        super(parent, "Confirm file replace", true);

        label = new JLabel("Are you sure you want to replace file " + lfn + "?");

        yesButton = new JButton("Yes");
        yesButton.addActionListener(this);
        yesToAllButton = new JButton("Yes to all");
        yesToAllButton.addActionListener(this);
        noButton = new JButton("No");
        noButton.addActionListener(this);
        noToAllButton = new JButton("No to all");
        noToAllButton.addActionListener(this);

        buttonPanel = new JPanel();
        buttonPanel.setLayout(new FlowLayout());
        buttonPanel.add(yesButton);
        buttonPanel.add(yesToAllButton);
        buttonPanel.add(noButton);
        buttonPanel.add(noToAllButton);

        this.getContentPane().setLayout(new BorderLayout());
        this.getContentPane().add(label, BorderLayout.CENTER);
        this.getContentPane().add(buttonPanel, BorderLayout.SOUTH);

        pack();

        setLocationRelativeTo(null);
        setVisible(true);
    }

    public void actionPerformed(ActionEvent e) {
        Object source = e.getSource();

        if (source == yesButton) {
            result = YES;
            dispose();
        }
        else if (source == yesToAllButton) {
            result = YES_TO_ALL;
            dispose();
        }
        else if (source == noButton) {
            result = NO;
            dispose();
        }
        else if (source == noToAllButton) {
            result = NO_TO_ALL;
            dispose();
        }
    }

    public int getResult() {
        return result;
    }
}
