/***********************************************************************
*
*   Filename:   QCDgridFileRemover.java
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Removes a set of files, and their corresponding metadata,
*               from the grid. Displays a progress window
*
*   Contents:   QCDgridFileRemover class
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
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

class QCDgridFileRemover implements Runnable
{
    /** Contents of metadata documents */
    private String[] documents;

    /** The progress window to keep up to date with our status */
    private QCDgridObserver observer;

    /** The submitter class with a method to remove data and metadata in one go */
    private QCDgridSubmitter submitter;

    /** Thread object of file remover */
    private Thread t;

    /** Whether the operation has been cancelled */
    private boolean doCancel;

    /**
     *  Constructor: starts off a new file remover thread
     *  @param docs contents of metadata files
     */
    public QCDgridFileRemover(String[] docs) throws QCDgridException {
	documents = docs;
	doCancel = false;
	submitter = new QCDgridSubmitter();
	observer = new QCDgridProgressWindow(null);
	t = new Thread(this);
	t.start();
    }

    public QCDgridFileRemover( String[] docs, QCDgridObserver observer) throws QCDgridException{
	documents = docs;
	doCancel = false;
	submitter = new QCDgridSubmitter();
	if(observer==null){
	    this.observer = new QCDgridProgressWindow(null);
	}
	else{
	    this.observer = observer;
	}
	t = new Thread(this);
	t.start();
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
		    status.setNumFilesProcessed(i+1);
		    status.setLastProcessedFileName(lfn);
		    observer.update(status, ObservableStateType.MULTI_FILE_ACTION_PROGRESS);
		}

		// Actually remove file
		submitter.removeFile(lfn);
	    }

	    // Final update for progress bar
	    observer.update( "Operation completed", ObservableStateType.GENERAL_MESSAGE);
	}
	catch (Exception e) {
	    System.err.println("Exception occurred: "+e);

	    // Send the error to the GUI to inform the user
	    observer.update(e.toString(), ObservableStateType.ERROR_STATE );
	}
    }

}
