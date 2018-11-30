/*
 * Dialogue that contains a progress bar, some status text and an optional cancel button. To use:
 *  1. Create using the constructor
 *  2. To set the status text and the progress bar position (in percent), call "update"
 *  3. To check for the user cancelling, call "isCancelled"
 *  4. When finished with, call "dispose" (this won't happen automatically even if user cancels)
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;

public class ProgressDialogue extends JFrame implements ActionListener {

    /** Button for cancelling the transfer */
    private JButton cancelButton;

    /** Actual progress bar widget */
    private JProgressBar progressBar;

    /** Progress label (i.e. 'Transferred 0 of 1000000 files') */
    private JLabel progressLabel;

    /** Status label (i.e. 'Connecting to grid...') */
    private JLabel statusLabel;

    /** Centre panel containing everything except cancel button */
    private JPanel panel;

    /** Button panel for at the bottom */
    private JPanel buttonPanel;

    /** Panel to stop status bar looking big and chunky */
    private JPanel barPanel;

    private boolean cancelled;

    public ProgressDialogue(String title, boolean cancel) {
        super(title);

        setSize(new Dimension(300, 150));

        progressBar = new JProgressBar();
        progressBar.setMaximum(100);
        progressBar.setValue(0);

        progressLabel = new JLabel("Processing...");
        statusLabel = new JLabel("");
        barPanel = new JPanel();
        barPanel.setLayout(new FlowLayout());
        barPanel.add(progressBar);
        panel = new JPanel();
        panel.setLayout(new GridLayout(3, 1, 8, 8));
        panel.add(barPanel);
        panel.add(progressLabel);
        panel.add(statusLabel);
        getContentPane().setLayout(new BorderLayout());
        getContentPane().add(panel, BorderLayout.CENTER);

        if (cancel) {
            cancelButton = new JButton("Cancel");
            cancelButton.addActionListener(this);
            
            buttonPanel = new JPanel();
            buttonPanel.setLayout(new FlowLayout());
            buttonPanel.add(cancelButton);
            getContentPane().add(buttonPanel, BorderLayout.SOUTH);
        }

        cancelled = false;

        setLocationRelativeTo(null);
        setVisible(true);
        repaint();        
    }

    public void update(String progress, String status, int percent) {
        progressLabel.setText(progress);
        statusLabel.setText(status);
        progressBar.setValue(percent);
        repaint();
    }

    public boolean isCancelled() {
        return cancelled;
    }

    public void actionPerformed(ActionEvent e) {
        Object source = e.getSource();
        if (source == cancelButton) {
            // do cancel
            cancelled = true;
        }
    }
}

