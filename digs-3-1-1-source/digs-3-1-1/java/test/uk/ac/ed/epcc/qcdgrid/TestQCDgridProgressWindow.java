package test.uk.ac.ed.epcc.qcdgrid;

import uk.ac.ed.epcc.qcdgrid.browser.GUI.QCDgridProgressWindow;

import uk.ac.ed.epcc.qcdgrid.observer.MultiFileActionState;
import uk.ac.ed.epcc.qcdgrid.observer.ObservableStateType;


public class TestQCDgridProgressWindow
{

  public static void main ( String args[] )
  {
      try {
	  QCDgridProgressWindow win = new QCDgridProgressWindow( null );
	  win.setVisible( true );
	  MultiFileActionState fss = new MultiFileActionState();
	  fss.setNumFilesTotal( 5 );
	  fss.setNumFilesProcessed( 1 );
	  fss.setLastProcessedFileName( "file1.xml" );
	  win.update( fss, ObservableStateType.MULTI_FILE_ACTION_PROGRESS );
	  Thread.sleep( 3000 );
	  fss.setNumFilesProcessed( 3 );
	  fss.setLastProcessedFileName( "file2.xml" );
	  win.update( fss, ObservableStateType.MULTI_FILE_ACTION_PROGRESS );
	  Thread.sleep( 5600 );
	  fss.setNumFilesProcessed( 4 );
	  fss.setLastProcessedFileName( "file4.xml" );
	  win.update( fss, ObservableStateType.MULTI_FILE_ACTION_PROGRESS );
      } catch( Exception e ) { }

  }
}

