/***********************************************************************
*
*   Filename:   QCDgridFileToucher
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Allows a set of files found through queries on the
*               metadata to be 'touched'
*
*   Contents:   QCDgridFileToucher class
*
*   Used in:    Java client tools
*
* Initial revision
***********************************************************************/
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import uk.ac.ed.epcc.qcdgrid.datagrid.*;
import uk.ac.ed.epcc.qcdgrid.metadata.*;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.QCDgridProgressWindow;
import uk.ac.ed.epcc.qcdgrid.observer.*;

class QCDgridFileToucher implements Runnable
{
    /** Contents of metadata documents */
    private String[] documents;

    /** The progress window to keep up to date with our status */
    private QCDgridObserver observer;

    /**
     *  Interface to the actual data grid. It uses a lot of static
     *  storage, so we should only have one instance of it
     */
    private static QCDgridClient grid = null;

    /** Thread object of file toucher */
    private Thread t;

    /** Whether the operation has been cancelled */
    private boolean doCancel;

    /**
     *  Constructor: starts off a new file toucher thread
     *  @param docs contents of metadata files
     */
    public QCDgridFileToucher(String[] docs) {
	documents = docs;
	observer = new QCDgridProgressWindow(null);
	doCancel = false;
	t = new Thread(this);
	t.start();
    }

    /**
     *  @return the number of files being touched
     */
    public int getNumFiles() {
	return documents.length;
    }

    /**
     *  Cancels the operation. Tells the thread to stop and waits for
     *  it to terminate
     */
    public void cancel() {
	doCancel = true;
	while (t.isAlive());
    }

    /**
     *  Core of the thread
     */
    public void run() {
	try {
	    
	    // Give an initial status report to the progress window
	    if (observer!=null) {
		observer.update( "Connecting to grid...", ObservableStateType.GENERAL_MESSAGE);
	    }

	    // Obtain an interface to the datagrid if we don't already have one
	    if (grid==null) {
		grid = QCDgridClient.getClient(true);
	    }
	    
	    MultiFileActionState status = new MultiFileActionState();
	    for (int i=0; i<documents.length; i++) {
		// Terminate if the user cancelled
		if (doCancel) {
		    return;
		}

		// Extract logical filename from metadata
		QCDgridLogicalNameExtractor qlne = new QCDgridLogicalNameExtractor(documents[i]);
		String lfn = qlne.getLogicalFileName();

		// Update progress bar
		if (observer!=null) {
		    status.setNumFilesProcessed( i+1 );
		    status.setLastProcessedFileName( lfn );
		    observer.update(status, ObservableStateType.MULTI_FILE_ACTION_PROGRESS);
		}

		// Actually register interest
		grid.touchFile(lfn);
	    }

	    // Final update for progress bar
	    observer.update( "Operation completed", ObservableStateType.GENERAL_MESSAGE );
	}
	catch (Exception e) {
	    System.err.println("Exception occurred: "+e);

	    // Send the error to the GUI to inform the user
	    observer.update(e.toString(), ObservableStateType.ERROR_STATE);
	}
    }

}
