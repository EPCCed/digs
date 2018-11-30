/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.io.File;

import uk.ac.ed.epcc.qcdgrid.datagrid.*;
import uk.ac.ed.epcc.qcdgrid.metadata.*;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.QCDgridProgressWindow;
import uk.ac.ed.epcc.qcdgrid.observer.*;
import uk.ac.ed.epcc.qcdgrid.QCDgridException;

/**
 * Title: QCDgridFileRetriever Description: A thread which retrieves files from
 * the grid in the background while the main GUI thread displays a progress bar
 * Copyright: Copyright (c) 2003 Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class QCDgridFileRetriever implements Runnable {

  /** List of names of metadata documents specifying which data to get */
  // FIXME: I don't think this really needs to be here
  private String[] files;

  /** Contents of metadata documents */
  private String[] documents;

  /** Destination file (or directory if more than one) */
  private File destination;

  /** The progress window to keep up to date with our status */
  private QCDgridObserver observer;

  /**
   * Interface to the actual data grid. It uses a lot of static storage, so we
   * should only have one instance of it
   */
  private static QCDgridClient grid = null;

  /** Thread object of file retriever */
  private Thread t;

  /** Whether the operation has been cancelled */
  private boolean doCancel;

  /**
   * Constructor: starts off a new file retriever thread
   * 
   * @param f
   *          list of metadata filenames
   * @param dest
   *          destination file or directory on local machine
   * @param docs
   *          contents of metadata files
   */
  public QCDgridFileRetriever(String[] f, File dest, String[] docs) {
    destination = dest;
    files = f;
    documents = docs;
    observer = new QCDgridProgressWindow(null);
    doCancel = false;
    t = new Thread(this);
    t.start();
  }

  /**
   * Cancels the transfer. Tells the thread to stop and waits for it to
   * terminate. Doesn't remove any files already transferred though.
   */
  public void cancel() {
    doCancel = true;
    while (t.isAlive())
      ;
  }

  /**
   * Core of the thread
   */
  public void run() {
    try {

      // Give an initial status report to the progress window
      if (observer != null) {
        observer.update("Connecting to grid...",
            ObservableStateType.GENERAL_MESSAGE);
      }
      // Obtain an interface to the datagrid if we don't already have one
      if (grid == null) {
        grid = QCDgridClient.getClient(true);
      }
      if (files.length == 1) {
        // If transferring one file, just use the name of the destination as the
        // physical filename. Logical filename is extracted from metadata
        QCDgridLogicalNameExtractor qlne = new QCDgridLogicalNameExtractor(
            documents[0]);
        // System.out.println("Created LNE");
        String lfn = qlne.getLogicalFileName();
        String pfn = destination.getPath();

        // Update progress bar
        if (observer != null) {
          observer.update("Transferring file " + lfn,
              ObservableStateType.GENERAL_MESSAGE);
        }

        // Actually do the transfer
        grid.getFile(lfn, pfn);
      } else {
        MultiFileActionState status = new MultiFileActionState();
        for (int i = 0; i < files.length; i++) {
          // Terminate if the user cancelled
          if (doCancel) {
            return;
          }

          // Extract logical filename from metadata
          QCDgridLogicalNameExtractor qlne = new QCDgridLogicalNameExtractor(
              documents[i]);
          // System.out.println("Getting file corresponding to "+files[i]+":
          // "+qlne.getLogicalFileName());
          String lfn = qlne.getLogicalFileName();

          // Make physical filename by appending filename component of logical
          // filename
          // to the destination directory
          String pfn;
          int k = lfn.lastIndexOf('/');
          if (k < 0) {
            pfn = destination.getPath() + lfn;
          } else {
            pfn = destination.getPath() + lfn.substring(k);
          }
          // System.out.println(lfn+" -> "+pfn);

          // Update progress bar
          if (observer != null) {
            status.setNumFilesProcessed(i + 1);
            status.setLastProcessedFileName(lfn);
            observer.update(status,
                ObservableStateType.MULTI_FILE_ACTION_PROGRESS);
          }

          // Actually do the transfer
          grid.getFile(lfn, pfn);
        }
      }

      // Final update for observer
      observer
          .update("Transfer completed", ObservableStateType.GENERAL_MESSAGE);
    } catch (Exception e) {
      System.err.println("Exception occurred: " + e);

      // Send the error to the GUI to inform the user
      observer.update(e.toString(), ObservableStateType.GENERAL_MESSAGE);
    }
  }
}
