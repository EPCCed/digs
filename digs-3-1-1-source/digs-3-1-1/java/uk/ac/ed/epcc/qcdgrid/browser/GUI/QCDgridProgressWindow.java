/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import uk.ac.ed.epcc.qcdgrid.observer.*;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridSubmitter;

/**
 * Title:        QCDgridProgressWindow
 * Description:  Displays a progress bar and status messages while files
 * are being transferred from the grid
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 * Changes:
 * Now implements QCDObserver as a means of being passed state changes. 
 * This allows the file upload component to notify it of changes to its 
 * status.
 */
public class QCDgridProgressWindow extends JFrame implements QCDgridObserver, ActionListener {

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

    /** object doing submission */
    private QCDgridSubmitter submitter_;


    /**
     *  Constructor: creates a new progress window
     *  @param qfr the file retriever to report progress of
     */
    public QCDgridProgressWindow(QCDgridSubmitter submitter) {
	super("QCDgrid Operation Progress");

	setSize(new Dimension(300, 150));
	submitter_ = submitter;

	cancelButton = new JButton("Cancel");
	cancelButton.addActionListener(this);

      	progressBar = new JProgressBar();

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

	buttonPanel = new JPanel();
	buttonPanel.setLayout(new FlowLayout());
	buttonPanel.add(cancelButton);
	getContentPane().add(buttonPanel, BorderLayout.SOUTH);

	setVisible(true);
	repaint();
    }

    /**
     *  Called to indicate a change in the file retrieving operation's
     *  status which should be reflected in the progress window
     *  @param state A FileSubmitState object detailing the current progress.
     *  @param type The type of state being notified.
     */
    public void update( Object state, ObservableStateType type ) {
	if( type == ObservableStateType.MULTI_FILE_ACTION_PROGRESS ){
	    MultiFileActionState fss = (MultiFileActionState) state;	   
	    if( fss.getNumFilesTotal() == fss.getNumFilesProcessed() ){
		//	System.out.println( "ProgressWindow::done()");
		dispose();
	    }
	    else{
		//	System.out.println( "FAS_PROG" );
		final int fTotal = fss.getNumFilesTotal();
		final int fProcessed = fss.getNumFilesProcessed();
		final String msgProgress = "Processed " + fss.getNumFilesProcessed() +
		    "/" + fss.getNumFilesTotal();
		final String msgStatus =  "Last file: " + fss.getLastProcessedFileName();
		try{
		    EventQueue.invokeLater( new Runnable(){
			    public void run(){
				progressBar.setMaximum( fTotal );
				progressBar.setValue( fProcessed );
				progressLabel.setText( msgProgress );
				statusLabel.setText( msgStatus );	
				repaint();
			    }
			} 
					      );
		}
		catch (Exception e) {
		    System.err.println("Exception occurred showing error dialogue: "+e);   
		}		    
	    }
	}
	else if( type == ObservableStateType.ERROR_STATE ){ 
	    final String errorMsg = (String) state;
	    try {
		EventQueue.invokeLater( new Runnable(){
			public void run(){
			    progressLabel.setText( "An error occured: " + errorMsg );
			    JOptionPane.showMessageDialog( null, errorMsg, 
							   "Grid error", JOptionPane.ERROR_MESSAGE );
			    dispose();
			}
		    }
					);
	    }
	    catch (Exception e) {
		System.err.println("Exception occurred showing error dialogue: "+e);

	    }	    

	}
	else if( type == ObservableStateType.GENERAL_MESSAGE ){
	    statusLabel.setText( (String) state );
	}	
	else if( type == ObservableStateType.FINISHED ){
	    dispose();
	}

    }
    
    /**
     *  Event handler: handles clicking on the cancel button
     */
    public void actionPerformed(ActionEvent e) {
	Object source = e.getSource();

	if (source==(Object)cancelButton) {
	    if( submitter_ != null ){
		// Cancel the underlying operation
		submitter_.cancelDirectorySubmit();
	    }
	    // Get rid of this window
	    this.dispose();
	}
    }
}
