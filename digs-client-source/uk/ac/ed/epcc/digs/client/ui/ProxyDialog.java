/*
 * Modal dialogue that allows user to initialise a proxy.
 * Shows a progress dialogue while the proxy is actually being initialised
 */
package uk.ac.ed.epcc.digs.client.ui;

import uk.ac.ed.epcc.digs.ProxyUtil;
import uk.ac.ed.epcc.digs.DigsException;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import java.util.Observer;
import java.util.Observable;

import org.globus.common.CoGProperties;
import org.globus.gsi.GlobusCredential;

public class ProxyDialog extends JDialog implements ActionListener, ChangeListener, Observer, Runnable {

    /* UI components */
    private JButton okButton;
    private JButton cancelButton;

    private JPasswordField passwordField;

    private JSlider hoursSlider;
    private JLabel hoursLabel;

    private JComboBox voBox;
    private JLabel voLabel;

    private JPanel labelPanel;
    private JPanel buttonPanel;
    private JPanel sliderPanel;
    private JPanel voPanel;

    /* progress bar while proxy being created */
    private ProgressDialogue progressDialogue;

    /* hours proxy should be valid for */
    private int hours;

    /* passphrase */
    private String passphrase;

    /* VO that the user chose */
    private String voSelected;

    /* set to true when proxy initialisation is complete (or cancelled) */
    private boolean done;

    /* number of tries at entering the passphrase */
    private int tryCount;

    public ProxyDialog() throws DigsException {
	super((Frame) null, "Please enter your certificate passphrase", true);

        done = false;
	tryCount = 0;
	voSelected = null;

	/* Set system properties required by VOMS proxy code */
	System.setProperty("X509_USER_CERT", CoGProperties.getDefault().getUserCertFile());
	System.setProperty("X509_USER_KEY", CoGProperties.getDefault().getUserKeyFile());

	String cacert = CoGProperties.getDefault().getCaCertLocations();
	// just use first location if comma separated list
	int comma = cacert.indexOf(',');
        if (comma >= 0) {
	    cacert = cacert.substring(0, comma);
	}
	System.setProperty("CADIR", cacert);

	String vomsesloc = System.getenv("VOMSES_LOCATION");
	if (vomsesloc != null) {
	    System.setProperty("VOMSES_LOCATION", vomsesloc);
	}
	String vomsdir = System.getenv("VOMSDIR");
	if (vomsdir != null) {
	    System.setProperty("VOMSDIR", vomsdir);
	}


	/* get cert subject and check it exists */
        String dn = ProxyUtil.getCertDN();
        if (dn == null) {
            throw new DigsException("Unable to find user certificate");
        }

	okButton = new JButton("OK");
	okButton.addActionListener(this);

	cancelButton = new JButton("Cancel");
	cancelButton.addActionListener(this);

	passwordField = new JPasswordField(40);
	passwordField.addActionListener(this);

	hoursSlider = new JSlider(1, 168, 12);
	hoursSlider.setMinorTickSpacing(12);
	hoursSlider.setMajorTickSpacing(24);
	hoursSlider.setPaintTicks(true);
	hoursSlider.addChangeListener(this);
	hoursLabel = new JLabel("Proxy lifetime: 12 hours");
	
	sliderPanel = new JPanel();
	sliderPanel.setLayout(new BorderLayout(2, 2));
	sliderPanel.add(hoursSlider, BorderLayout.CENTER);
	sliderPanel.add(hoursLabel, BorderLayout.NORTH);

	labelPanel = new JPanel();
	labelPanel.setLayout(new BorderLayout(2, 2));
	labelPanel.add(new JLabel(" Your identity is:"), BorderLayout.NORTH);
	labelPanel.add(new JLabel("   " + dn), BorderLayout.CENTER);
	labelPanel.add(new JLabel(" Please enter your passphrase:"), BorderLayout.SOUTH);

	Object[] volist = ProxyUtil.getVOList();
	voBox = new JComboBox(volist);
	voPanel = new JPanel();
	voPanel.setLayout(new BorderLayout(2, 2));
	voLabel = new JLabel("VO:");
	voPanel.add(voBox, BorderLayout.CENTER);
	voPanel.add(voLabel, BorderLayout.NORTH);

	buttonPanel = new JPanel();
	buttonPanel.setLayout(new FlowLayout(FlowLayout.CENTER, 10, 10));
	buttonPanel.add(okButton);
	buttonPanel.add(cancelButton);
	buttonPanel.add(voPanel);
	buttonPanel.add(sliderPanel);

	this.getContentPane().setLayout(new BorderLayout(10, 10));
	this.getContentPane().add(labelPanel, BorderLayout.NORTH);
	this.getContentPane().add(passwordField, BorderLayout.CENTER);
	this.getContentPane().add(buttonPanel, BorderLayout.SOUTH);

	pack();

        setLocationRelativeTo(null);
	setVisible(true);
    }

    /*
     * Separate thread for actual proxy creation, so that progress dialogue
     * updates and doesn't freeze up
     */
    public void run() {
	/* hide proxy dialogue */
	setVisible(false);

	/* create progress dialogue */
	progressDialogue = new ProgressDialogue("Creating proxy", false);

	try {
	    /* create the proxy */
	    ProxyUtil.createProxy(passphrase, hours, this, voSelected);
	}
	catch (Exception ex) {
	    /* show error box if exception occurred */
	    JOptionPane.showMessageDialog(this, ex.toString(),
					  "Error creating proxy", JOptionPane.ERROR_MESSAGE);

	    /* can't go on running without proxy */
	    System.exit(1);
	}

	/* destroy progress dialogue and this dialogue */
	progressDialogue.dispose();

	dispose();
        done = true;
    }

    public void actionPerformed(ActionEvent e) {
	Object src = e.getSource();
	if ((src == okButton) || (src == passwordField)) {
	    /* get values from UI */
	    voSelected = (String)voBox.getSelectedItem();
	    if (voSelected.equals("None")) {
		voSelected = null;
	    }

            hours = hoursSlider.getValue();
            char[] pp = passwordField.getPassword();
            passphrase = new String(pp);

	    if (!ProxyUtil.isValidPassphrase(passphrase)) {
		tryCount++;

		if (tryCount >= 3) {
		    JOptionPane.showMessageDialog(this, "Invalid passphrase. DiGS Client will now exit",
						  "Invalid passphrase", JOptionPane.ERROR_MESSAGE);
		    System.exit(1);
		}
		else {
		    JOptionPane.showMessageDialog(this, "Invalid passphrase, please try again",
						  "Invalid passphrase", JOptionPane.ERROR_MESSAGE);
		    passwordField.setText("");
		    return;
		}
	    }

	    /* start thread to actually create proxy */
	    Thread t = new Thread(this);
	    t.start();
	}
	else if (src == cancelButton) {
	    dispose();
            done = true;
	}
    }

    /*
     * Called when proxy util updates its status
     */
    public void update(Observable obs, Object arg) {
	/* show new percentage on progress dialogue */
	Integer i = (Integer) arg;
	progressDialogue.update("Creating proxy...", "", i);
    }

    /*
     * Handle the slider being activated
     */
    public void stateChanged(ChangeEvent e) {
	Object src = e.getSource();
	if (src == hoursSlider) {
	    int val = hoursSlider.getValue();
	    hoursLabel.setText("Proxy lifetime: " + val + " hours");
	}
    }

    public boolean isDone() {
        return done;
    }
}
