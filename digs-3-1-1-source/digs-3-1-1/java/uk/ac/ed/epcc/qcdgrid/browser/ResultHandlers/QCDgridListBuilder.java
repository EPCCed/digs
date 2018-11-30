/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ILDGQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MetadataDocumentRetriever;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ResultInfo;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.WebServiceMetadataDocumentRetriever;

import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;


/**
 * Title: QCDgridListBuilder Description: A thread which fetches the names of
 * result documents from the database in the background to avoid slowing down
 * the user interface Copyright: Copyright (c) 2003 Company: The University of
 * Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class QCDgridListBuilder implements Runnable {

  /** The set of results to retrieve */
  private QCDgridQueryResults queryResults;

  /** The list model to put the names into */
  private QCDgridListModel listModel;

  /** The thread which is actually doing this */
  private Thread t;

  /** Whether the thread should now stop */
  private boolean stopping;

  /**
   * Constructor: starts a new list builder thread.
   * 
   * @param qr
   *          the set of results to retrieve
   * @param lm
   *          the list model to put the names into
   */
  public QCDgridListBuilder(QCDgridQueryResults qr, QCDgridListModel lm) {
    queryResults = qr;
    listModel = lm;
    stopping = false;

    t = new Thread(this);
    t.start();
  }

  /**
   * Tells the thread to stop as soon as possible
   */
  public void stop() {
    stopping = true;
  }

  /**
   * @return true if the thread is still running, false if it's stopped
   */
  public boolean isAlive() {
    return t.isAlive();
  }

  /**
   * The core of the thread
   */
  public void run() {

    // Have to handle this subclass first otherwise it's caught by the 
    // next conndition
    if(queryResults instanceof ILDGQueryResults){
      try{        
        ResultInfo[] results = queryResults.getIndividualResultInfoObjects();
        for(int i=0; i<results.length; i++){
          listModel.addItem(results[i]);
          if (stopping) {
            return;
          }
        }
      }
      catch(QCDgridException qe){
        System.out.println("There was an exception: " + qe.getMessage());
      }
    }
    
    else if(queryResults instanceof QCDgridQueryResults) {
      try {

        // Get the individual results (database must support this)
        ResourceSet rs = queryResults.resultsAsXMLDBResourceSet();
        if (rs == null) {
          System.err
              .println("The selected database backend is incompatible with this result handler");
          return;
        }

        // For each result, get its name and add that to the list model
        for (int i = 0; i < rs.getSize(); i++) {
          Resource res = rs.getResource(i);
          XMLResource xres = (XMLResource) res;
          listModel.addItem(new ResultInfo(xres.getDocumentId()));

          // Stop now if something told us to
          if (stopping) {
            return;
          }
        }
      } catch (Exception e) {
        System.err.println("Exception: " + e);
        e.printStackTrace();
      }

    }
  }

}
