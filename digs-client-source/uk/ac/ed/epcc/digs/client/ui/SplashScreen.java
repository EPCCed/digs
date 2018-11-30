/*
 * Displays a splash screen during startup, with a logo, progress bar and status label
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;
import java.util.Observer;
import java.util.Observable;

import uk.ac.ed.epcc.digs.client.DigsClient;

public class SplashScreen extends JFrame implements Observer {

    /** Actual progress bar widget */
    private JProgressBar progressBar;

    /** Progress label (i.e. 'Transferred 0 of 1000000 files') */
    private JLabel progressLabel;

    private JLabel imageLabel;

    /** Centre panel containing everything except cancel button */
    private JPanel panel;

    private JPanel barPanel;

    public SplashScreen(ImageIcon img) {
        super("");

        // create window
        progressBar = new JProgressBar();
        progressBar.setMaximum(100);
        progressBar.setValue(0);

        progressLabel = new JLabel("Processing...");

        imageLabel = new JLabel(img);

        panel = new JPanel();
        panel.setLayout(new BorderLayout());

        barPanel = new JPanel();
        barPanel.setLayout(new BorderLayout());
        barPanel.add(progressBar, BorderLayout.CENTER);
        barPanel.add(progressLabel, BorderLayout.NORTH);

        panel.add(barPanel, BorderLayout.SOUTH);
        panel.add(imageLabel, BorderLayout.CENTER);

        getContentPane().add(panel);

        // remove title bar etc.
        setUndecorated(true);

        pack();
        setLocationRelativeTo(null);
        setVisible(true);
        repaint();
    }

    // call to update the progress bar and status string
    public void update(String progress, int percent) {
        progressLabel.setText(progress);
        progressBar.setValue(percent);
        repaint();
    }

    /*
     * Observer update method. Called by the DiGS client to inform us of its
     * progress during startup.
     */
    public void update(Observable obs, Object obj) {
        DigsClient.DigsClientStatus status = (DigsClient.DigsClientStatus) obj;

        // update the splash screen with new status
        update(status.status, status.percentage);
    }
}

