/***********************************************************************
*
*   Filename:   QCDgridObservable.java
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    An interface which is implemented by any QCDgrid class
*               which may be monitored by a progress window
*
*   Contents:   QCDgridObservable interface
*
*   Used in:    Java client tools
*
* Initial revision
***********************************************************************/
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

public interface QCDgridObservable {
    
    /**
     *  @return the total number of files involved in the operation
     */
    public int getNumFiles();

    /**
     *  Cancels the operation in progress
     */
    public void cancel();
}
