/***********************************************************************
 *
 *   Filename:   QCDgridSubmitter.java
 *
 *   Authors:    James Perry, Daragh Byrne     (jamesp, daragh)   EPCC.
 *               Radoslaw Ostrowski                 (radek)       EPCC.
 *   Purpose:    Submitting data and metadata to the grid in a single
 *               operation
 *
 *   Contents:   QCDgridSubmitter class
 *
 *   Used in:    Java client tools
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2003 The University of Edinburgh
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of the
 *   License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *   MA 02111-1307, USA.
 *
 *   As a special exception, you may link this program with code
 *   developed by the OGSA-DAI project without such code being covered
 *   by the GNU General Public License.
 *
 ***********************************************************************/
package uk.ac.ed.epcc.qcdgrid.metadata;

import java.io.*;
import java.util.*;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.datagrid.*;
import uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers.*;
import uk.ac.ed.epcc.qcdgrid.observer.*;
import uk.ac.ed.epcc.qcdgrid.metadata.PrefixHandler;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.DirectoryValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.MetadataValidator;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ConfigurationMetadataValidator;
import uk.ac.ed.epcc.qcdgrid.observer.MultiFileActionState;

/**
 * This class is used both directly from the command line, and also by the GUI
 * tools.
 * 
 * September 2005 This class was refactored to make use of the new
 * QCDgridObserver pattern when doing long-running directory uploads.
 * 
 * October 2005 Refactored substantially to make use of the metadata
 * verification code; also just generally "tidied up".
 * 
 */
public class QCDgridSubmitter implements Runnable {
  /** Reference to the actual data grid */
  private QCDgridClient dataGrid;

  /** Reference to the metadata catalogue */
  private QCDgridMetadataClient metaDataCatalogue;

  /*
   * These variables have to be class variables and not local variables because
   * they are accessed from multiple threads
   */
  /** set to true if the directory submission operation has been cancelled */
  private boolean cancelled;

  /**
   * set to true if the directory submission operation has finished (either
   * completed successfully, encountered an error or cancelled by the user)
   */
  private boolean finished;

  /** number of files in the directory being submitted */
  private int numFiles;

  /** local directory containing the metadata to be submitted */
  private File metadatadir;

  /** local directory containing the data to be submitted */
  private File datadir;

  /** Observer, usually a QCDgridProgressWindow */
  private QCDgridObserver observer_;

  /** Running thread */
  private Thread th;


  /** variables to deal with input argument parsing */	
	public static final String OPTION_HELP_SHORT= "-h";
	public static final String OPTION_HELP_LONG= "--help";
	public static final String OPTION_ENSEMBLE_SHORT= "-E";
	public static final String OPTION_ENSEMBLE_LONG= "--ensemble";
	public static final String OPTION_NOMETADATA_SHORT= "-N";
	public static final String OPTION_NOMETADATA_LONG= "--nometadata";
	public static final String OPTION_RECURSIVE_SHORT= "-R";
	public static final String OPTION_RECURSIVE_LONG= "--recursive";
	public static final String OPTION_PERMISSIONS_SHORT= "-P";
	public static final String OPTION_PERMISSIONS_LONG= "--permissions";
	
	public static final String OPTION_PERMISSIONS_PRIVATE= "private";
	public static final String OPTION_PERMISSIONS_PUBLIC= "public";
	
	public static int numberOfArgs = 0;
	public static int numberOfOptions = 0;
	
	public static boolean isHelp = false;
	public static boolean isEnsemble = false;
	public static boolean isNometadata = false;
	public static boolean isRecursive = false;
	public static boolean isPermissions = false;
	
	public static String permissionsValue = OPTION_PERMISSIONS_PRIVATE;


  /**
   * Constructor which opens new connections to both the data grid and the
   * metadata catalogue
   */
  public QCDgridSubmitter() throws QCDgridException {
    dataGrid = QCDgridClient.getClient(false);
    String connStr = null;
    try {
      connStr = dataGrid.getMetadataCatalogueLocation();
      if (connStr.indexOf(':') < 0) {
	connStr = connStr + ":8080";
      }
      /* System.out.println("Connecting to mdc: " + connStr);*/
      metaDataCatalogue = new QCDgridMetadataClient(connStr);
    } catch (Exception e) {
      String errStr = "Could not open metadata catalogue ";
      if (connStr != null) {
        errStr += " on " + connStr + " ";
      }
      errStr += "due to: " + e.getMessage();
      throw new QCDgridException(errStr);
    }
  }

  /**
   * Constructor which uses an existing connection to the data grid, but opens a
   * new connection to the metadata catalogue
   * 
   * @param dg
   *          existing reference to the data grid
   */
  public QCDgridSubmitter(QCDgridClient dg) throws QCDgridException {
    dataGrid = dg;
    String mdcLocation = null;
    try {
      mdcLocation = dataGrid.getMetadataCatalogueLocation();
      if (mdcLocation.indexOf(':') < 0) {
	mdcLocation = mdcLocation + ":8080";
      }
      System.out.println("Using mdc: " + mdcLocation);
      metaDataCatalogue = new QCDgridMetadataClient(mdcLocation);
    } catch (Exception e) {
      String errStr = "Could not open metadata catalogue ";
      if (mdcLocation != null) {
        errStr += " on " + mdcLocation + " ";
      }
      errStr += "due to: " + e.getMessage();
      throw new QCDgridException(errStr);
    }
  }

  /**
   * Constructor which uses an existing connection to the metadata catalogue but
   * opens a new connection to the data grid
   * 
   * @param mdc
   *          existing reference to the metadata catalogue
   */
  public QCDgridSubmitter(QCDgridMetadataClient mdc) throws QCDgridException {
    dataGrid = QCDgridClient.getClient(false);
    metaDataCatalogue = mdc;
  }

  /**
   * Constructor which uses existing connections to both the data grid and the
   * metadata catalogue
   * 
   * @param dg
   *          existing reference to the data grid
   * @param mdc
   *          existing reference to the metadata catalogue
   */
  public QCDgridSubmitter(QCDgridClient dg, QCDgridMetadataClient mdc) {
    dataGrid = dg;
    metaDataCatalogue = mdc;
  }

  /** thread function - actually does the directory submission */
  public void run() {
    try {
      File[] dataDirList;
      // Lists to store the actual files to submit
      List metadatalist = new ArrayList();
      List datafilelist = new ArrayList();

      // Default permissions for files added onto the grid
      final String defaultPermissions = "private";

      // Status reporting to observers
      MultiFileActionState uploadState = new MultiFileActionState();

      if (!datadir.isDirectory()) {
        throw new QCDgridException(datadir.getPath() + " is not a directory");
      }
      if (!metadatadir.isDirectory()) {
        throw new QCDgridException(metadatadir.getPath()
            + " is not a directory");
      }

      dataDirList = datadir.listFiles();
      uploadState.setNumFilesTotal(dataDirList.length);

      // Build the metadata file list and data file list
      for (int i = 0; i < dataDirList.length; i++) {
        // In case the data and metadata are in the same directory - skip all
        // .xml files
        int extstart = dataDirList[i].getName().lastIndexOf('.');
        if (extstart >= 0) {
          if (dataDirList[i].getName().substring(extstart).equals(".xml")) {
            continue;
          }

        }
        datafilelist.add(dataDirList[i]);
        // System.out.println( "Added file " + dataDirList[i].getName() + " to
        // list." );
        // Try concatenating '.xml'
        File metadatafile = new File(metadatadir, dataDirList[i].getName()
            + ".xml");
        if (!metadatafile.exists()) {
          // Try removing the existing extension and adding '.xml'
          if (extstart >= 0) {
            metadatafile = new File(metadatadir, dataDirList[i].getName()
                .substring(0, extstart)
                + ".xml");
          }
          if (!metadatafile.exists()) {
            throw new QCDgridException("Can't find metadata for file "
                + dataDirList[i].getPath());
          }
        }
        System.out.println("Adding file " + metadatafile.getName()
            + " to list.");
        metadatalist.add(metadatafile);
      }

      if (datafilelist.size() != metadatalist.size()) {
        System.out
            .println("Different numbers of metadata files and configuration files found "
                + datafilelist.size() + ".");
        throw new QCDgridException(
            "Different numbers of metadata files and configuration files found.");
      }

      DirectoryValidationException dve = null;
      MetadataValidator configValidator 
        = new ConfigurationMetadataValidator(this.metaDataCatalogue);
      // System.out.println( "Dealing with " + metadatalist.size() + " metadata
      // files." );

      // Verify the metadata files
      for (int i = 0; i < metadatalist.size(); i++) {
        File mdfile = (File) metadatalist.get(i);
        try {
          System.out.println("Validating: " + mdfile.getPath());
          configValidator.validateMetadataDocument(mdfile);
          System.out.println("Done validating " + mdfile.getPath());
        } catch (ValidationException ve) {
          if (dve == null) {
            // System.out.println( "File " + mdfile.getName() +" was invalid."
            // );
            dve = new DirectoryValidationException(
                "At least one metadata file was invalid.");
            dve.addFileToInvalidList(mdfile.getName() + ": " + ve.getMessage());
          }
        }
        if (cancelled) {
          break;
        }
      }
      if (dve != null) {
        throw dve;
      }

      // Do the submission of metadata and configs
      for (int i = 0; i < datafilelist.size(); i++) {
        File mdf = (File) metadatalist.get(i);
        File df = (File) metadatalist.get(i);
        String logicalFileName = QCDgridLogicalNameExtractor
            .getLFNFromMetadataDocument(mdf);

        // Do submission and alert observers.
        // metadatafile first
        submitValidatedMetadataFileToMetadataCatalogue(mdf);
        if (this.observer_ != null) {
          // Alert observer
          uploadState.setNumFilesProcessed(i + 1);
          uploadState.setLastProcessedFileName(df.getPath());
          uploadState.setLastProcessedMetadataFileName(mdf.getPath());
          observer_.update(uploadState,
              ObservableStateType.MULTI_FILE_ACTION_PROGRESS);
        }
        // datafile and MD backup next.
        submitDataFileAndMetadataBackupToGrid(df, mdf, defaultPermissions);
        // System.out.println("MD & data submitted.");
        if (this.observer_ != null) {
          // Alert observer
          uploadState.setNumFilesProcessed(i + 1);
          uploadState.setLastProcessedFileName(df.getPath());
          uploadState.setLastProcessedMetadataFileName(mdf.getPath());
          observer_.update(uploadState,
              ObservableStateType.MULTI_FILE_ACTION_PROGRESS);
        }
        if (cancelled) {
          break;
        }
      }
      finished = true;
    }

    catch (Exception e) {
      finished = true;
      if (observer_ != null) {
        observer_.update(e.toString(), ObservableStateType.ERROR_STATE);
      }
      return;
    }
    if (observer_ != null) {
      observer_.update(new Object(), ObservableStateType.FINISHED);
    }
    System.out
        .println("Successfully completed submitting directory to the grid.");
  }

  /**
   * Submits a whole directory of data and corresponding metadata to the grid,
   * optionally with a progress window.
   * 
   * @param dd
   *          directory containing data
   * @param mdd
   *          directory containing metadata. May be the same as dd
   * @param showProgressWindow
   *          whether to show a progress window or not. If no progress window,
   *          status messages will be sent to the console
   */
  public synchronized void submitConfigDirectoryAndMetadata(File dd, File mdd,
      QCDgridObserver observer) {
    datadir = dd;
    metadatadir = mdd;
    finished = false;
    cancelled = false;
    this.observer_ = observer;

    numFiles = dd.listFiles().length;

    // This has to be threaded in order for the progress window to ever update
    th = new Thread(this);
    th.start();
  }

  /**
   * Cancels a directory submission operation
   */
  public void cancelDirectorySubmit() {
    /* Tell the thread to stop */
    cancelled = true;
    // System.out.println("cancelled=true");
    /* Wait until it does */
    while (!finished)
      ;
  }

  /**
   * Submits an ensemble metadata file to the catalogue, validating it first.
   */
  public void submitEnsembleMetadataFileToMetadataCatalogue(File ensMetadataFile)
      throws QCDgridException {
    try {
      metaDataCatalogue.submitEnsembleMetadataFile(ensMetadataFile);
    } catch (ValidationException ve) {
      throw new QCDgridException(
          "There was a problem validating the metadata document: "
              + ve.getMessage() + "\n" + ve.toString());
    } catch (Exception e) {
      throw new QCDgridException("Error storing " + ensMetadataFile.getPath()
          + " in metadata catalogue: " + e);
    }
    System.out.println("Successfully stored " + ensMetadataFile.getName()
        + " in metadata catalogue.");
  }

  /**
   * Submits a metdata file to the cataloge, including validation.
   */
  private void submitConfigMetadataFileToMetadataCatalogue(File metadatafile)
      throws QCDgridException {
    // First try to store the metadata in the catalogue
    try {
      metaDataCatalogue.submitConfigMetadataFile(metadatafile);
    } catch (ValidationException ve) {
      throw new QCDgridException(
          "There was a problem validating the metadata document: "
              + ve.getMessage() + "\n" + ve.toString());
    } catch (Exception e) {
      throw new QCDgridException("Error storing " + metadatafile.getPath()
          + " in metadata catalogue: " + e);
    }
    System.out.println("Successfully stored " + metadatafile.getName()
        + " in metadata catalogue.");
  }

  /**
   * Submits a metadata file to the catalogue, without validation
   */
  private void submitValidatedMetadataFileToMetadataCatalogue(File metadatafile)
      throws QCDgridException {
    // First try to store the metadata in the catalogue
    try {
      metaDataCatalogue.submitPreviouslyValidatedMetadataFile(metadatafile,
          metadatafile.getName());
    } catch (Exception e) {
      throw new QCDgridException("Error storing " + metadatafile.getPath()
          + " in metadata catalogue: " + e);
    }
  }

  /**
   * Stores a data file and metadata file on the grid.
   */
  private void submitDataFileAndMetadataBackupToGrid(File datafile,
      File metadatafile, String permissions) throws QCDgridException {
    // Store actual file on the grid
    String logicalFileName = null;
    try {
      logicalFileName = QCDgridLogicalNameExtractor
          .getLFNFromMetadataDocument(metadatafile);
      submitDataFileToGrid(datafile, logicalFileName, permissions);
    } catch (Exception e) {
      // Transfer to grid failed. Remove metadata from catalogue to keep the two
      // consistent
      try {
        metaDataCatalogue.removeFile(metadatafile.getName());
      } catch (Exception e2) {
      }
      throw new QCDgridException("Error storing file " + datafile.getPath()
          + " on grid as " + logicalFileName + ": " + e);
    }
    // Store backup copy of metadata on data grid
    try {
      dataGrid.putFile(metadatafile.getPath(), logicalFileName + ".xml", permissions);
    } catch (Exception e) {
      System.err
          .println("Warning: couldn't store metadata backup on data grid due to: "
              + e);
    }
  }

  /**
   * Simply stores an arbitrary data file on the grid.
   */
  public void submitDataFileToGrid(File datafile, String logicalFileName, String permissions)
      throws Exception {
	//RADEK here change to include also permissions
    dataGrid.putFile(datafile.getPath(), logicalFileName, permissions);
  }

  public void submitDataDirectoryToGrid(File datadir, String logicalFileName, String permissions)
      throws Exception {
      dataGrid.putDirectory(datadir.getPath(), logicalFileName, permissions);
  }

  /**
   * Submits a single file and its corresponding metadata file to the grid
   * 
   * @param datafile
   *          the data to submit
   * @param metadatafile
   *          the corresponding metadata
   */
  public void submitConfigFileAndMetadata(File datafile, File metadatafile, String permissions)
      throws Exception {
    submitConfigMetadataFileToMetadataCatalogue(metadatafile);
    submitDataFileAndMetadataBackupToGrid(datafile, metadatafile, permissions);
  }

  /**
   * Removes a file and its corresponding metadata from the grid
   * 
   * @param lfn
   *          logical grid filename of data to remove
   */
  public void removeFile(String lfn) throws Exception {

    // Try to delete the actual real data
    dataGrid.deleteFile(lfn);

    // Try to delete the metadata backup copy from the grid
    dataGrid.deleteFile(lfn + ".xml");

    // Look up the corresponding metadata document
    metaDataCatalogue.removeFileByLogicalFileName(lfn);
  }

  /**
   * Command line interface to submission functionality.
   * 
   * Usage: java uk.ac.ed.epcc.qcdgrid.metadata.QCDgridSubmitter -R --recursive
   * -N --nometadata
   */
  public static void main(String[] args) throws Exception {
    boolean showUsage = false;
    boolean directorySubmit = false;
    boolean ignoreMetadata = false;
    final String defaultPermissions = "private";

    /**
     * Usage: Mode name: Action: <file> <metadatafile> singleFileWithMetatdata
     * submitConfigFileAndMetadata -N/--nometadata <file> <lfn>
     * singleFileNoMetadata submitDataFileToGrid -R/--recursive <directory>
     * <metatdatadirectory> directoryWithMetadata
     * submitConfigDirectoryAndMetadata -R/--recursive -N/--nometadata
     * <directory> <lfn> directoryNoMetadata parse directory, figure out LFNs
     * and submitDataFileToGrid -e/--ensemble <file>
     */

    if (parseInputArgs(args)) {
	QCDgridSubmitter submitter;

	if (isNometadata) {
	    submitter = new QCDgridSubmitter(QCDgridClient.getClient(false), null);
	}
	else {
	    submitter = new QCDgridSubmitter();
	}
	/*
	System.out.println("numberOfArgs = " + numberOfArgs);
	System.out.println("numberOfOptions = " + numberOfOptions);
	System.out.println("isEnsemble = " + isEnsemble);
	System.out.println("isRecursive = " + isRecursive);
	System.out.println("isNometadata = " + isNometadata);
	System.out.println("isPermissions = " + isPermissions);
	System.out.println("permissionsValue = " + permissionsValue);
	*/

	try {
	    if (isEnsemble && numberOfArgs == 2
		&& numberOfOptions == 1) {
		submitter
		    .submitEnsembleMetadataFileToMetadataCatalogue(new File(
								       args[1]));
	    }
	    // -R/--recursive <directory> <metatdatadirectory>
	    if (isRecursive && numberOfArgs == 3
		&& numberOfOptions == 1) {
		try {
		    submitter.submitConfigDirectoryAndMetadata(new File(
								   args[1]), new File(PrefixHandler
										      .handlePrefix(args[2])), new QCDgridObserver() {
											      public void update(Object state,
														 ObservableStateType stateType) {
												  if (stateType == ObservableStateType.MULTI_FILE_ACTION_PROGRESS) {
												      MultiFileActionState fss = (MultiFileActionState) state;
												      System.out
													  .println("Submitted file "
														   + fss
														   .getLastProcessedFileName()
														   + " with metadata file "
														   + fss
														   .getLastProcessedMetadataFileName()
														   + "("
														   + fss
														   .getNumFilesProcessed()
														   + "/"
														   + fss.getNumFilesTotal()
														   + ")");
												  }
												  if (stateType == ObservableStateType.ERROR_STATE) {
												      System.out.println(state);
												  }
											      }
											  });
		} catch (Exception e) {
		    System.out.println(e);
		    // e.printStackTrace();
		}

	    }
	    // -N/--nometadata <file> <lfn>
	    if (isNometadata && numberOfArgs == 3
		&& numberOfOptions == 1) {
		submitter.submitDataFileToGrid(new File(args[1]),
					       PrefixHandler.handlePrefix(args[2]),
					       defaultPermissions);
	    }
	    // -P/--permissions public|private <file> <metadatafile>
	    if (isPermissions && numberOfArgs == 4
		&& numberOfOptions == 2
		&& permissionsValue != null) {
		submitter.submitConfigFileAndMetadata(new File(args[2]),
						      new File(args[3]), permissionsValue);

	    }
	    // -N/--nometadata -P/--permissions public|private <file> <lfn>
	    if (isPermissions && isNometadata
		&& numberOfArgs == 5 && numberOfOptions == 3
		&& permissionsValue != null) {
		// handle prefix in appropriate way: no prefix or lfn: OK
		// else :
		// NOT OK
		submitter.submitDataFileToGrid(new File(args[3]),
					       PrefixHandler.handlePrefix(args[4]),
					       permissionsValue);

	    }
	    // -R/--recursive -N/--nometadata <directory> <lfn>
	    if (isRecursive && isNometadata
		&& numberOfArgs == 4 && numberOfOptions == 2) {
		// directoryNoMetadata
		System.out.println("directoryNoMetadata");
		System.out.println(args[2] + " " + args[3]);
		File submitDir = new File(args[2]);
		if (!submitDir.isDirectory()) {
		    throw new QCDgridException(args[2]
					       + " is not a directory.");
		}
		String prefix = args[3];
		submitter.submitDataDirectoryToGrid(submitDir, prefix, defaultPermissions);
	    }
	    // -R/--recursive -N/--nometadata -P/--permissions
	    // public|private
	    // <directory> <lfn>
	    if (isPermissions && isNometadata && isRecursive
		&& numberOfArgs == 6 && numberOfOptions == 4
		&& permissionsValue != null) {
		// directoryNoMetadata
		System.out.println("directoryNoMetadata");
		System.out.println(args[4] + " " + args[5]);
		File submitDir = new File(args[4]);
		if (!submitDir.isDirectory()) {
		    throw new QCDgridException(args[4]
					       + " is not a directory.");
		}
		String prefix = args[5];
		submitter.submitDataDirectoryToGrid(submitDir, prefix, permissionsValue);
	    }
	    // -h/--help
	    if (isHelp) {
	        System.out.println(usage());
	    }
	    // <file> <metadatafile>
	    if (numberOfOptions == 0 && numberOfArgs == 2) {
		submitter.submitConfigFileAndMetadata(new File(args[0]),
						      new File(PrefixHandler.handlePrefix(args[1])),
						      defaultPermissions);

	    }
	} catch (QCDgridException qe) {
	    System.out
		.println("A problem occurred when interacting with the grid: ");
	    System.out.println(qe.getMessage());
	}

    } else {
        System.out.println(usage());
	return;
       }

//    System.out.println(usage());
    
  }

	
	public static boolean parseInputArgs(String[] args)
	    {
		numberOfArgs = args.length;
		
		for(int i=0; i< numberOfArgs; i++)
		{
		    // if Strings are equal returnes 0, otherwhise -1 or +1
		    if(args[i].compareTo(OPTION_HELP_SHORT) == 0 || 
		       args[i].compareTo(OPTION_HELP_LONG) == 0)
		    {
			isHelp = true;
			numberOfOptions++;
		    }
		    if(args[i].compareTo(OPTION_ENSEMBLE_SHORT) == 0 || 
		       args[i].compareTo(OPTION_ENSEMBLE_LONG) == 0)
		    {
			isEnsemble = true;
			numberOfOptions++;
		    }
		    if(args[i].compareTo(OPTION_PERMISSIONS_SHORT) == 0 ||
		       args[i].compareTo(OPTION_PERMISSIONS_LONG) == 0
		       && numberOfArgs >= i+1)
		    {
			isPermissions = true;
			// one more for private|public
			numberOfOptions += 2;
			// here also check the following argument
			if(args[i+1].compareTo(OPTION_PERMISSIONS_PRIVATE) == 0)
			{
			    permissionsValue = OPTION_PERMISSIONS_PRIVATE;
			}
			else if(args[i+1].compareTo(OPTION_PERMISSIONS_PUBLIC) == 0)
			{
			    permissionsValue = OPTION_PERMISSIONS_PUBLIC;
			}
			else 
			{
			    permissionsValue = null;
			    return false;
			}
		    }
		    if(args[i].compareTo(OPTION_RECURSIVE_SHORT) == 0 ||
		       args[i].compareTo(OPTION_RECURSIVE_LONG) == 0)
		    {
			isRecursive = true;
			numberOfOptions++;
		    }
		    if(args[i].compareTo(OPTION_NOMETADATA_SHORT) == 0 ||
		       args[i].compareTo(OPTION_NOMETADATA_LONG) == 0)
		    {
			isNometadata = true;
			numberOfOptions++;
		    }
		}
		if(numberOfArgs < 2 || numberOfArgs > numberOfOptions + 2)
		{
		    // not enough or too many additional arguments
		    return false;
		}	
		return true;
	    }

    public static String usage()
	{
	    return "Usage: java uk.ac.ed.epcc.qcdgrid.metadata.QCDgridSubmitter\n" +
			"\t<file> <metadatafile>\n" +
			"\t-P/--permissions public|private <file> <metadatafile>\n" +
			"\t-N/--nometadata <file> <lfn>\n" +
			"\t-N/--nometadata -P/--permissions public|private <file> <lfn>\n" +
			"\t-R/--recursive <directory> <metatdatadirectory>\n" +
			"\t-R/--recursive -N/--nometadata <directory> <lfn>\n" +
			"\t-R/--recursive -N/--nometadata -P/--permissions public|private <directory> <lfn>\n" +
			"\t-E/--ensemble <ensemblemetadatafile>\n\n";
	    /*
		    String str = new String();
		    str = str.concat("Usage: java uk.ac.ed.epcc.qcdgrid.metadata.QCDgridSubmitter\n")
			.concat("\t<file> <metadatafile>\n")
			.concat("\t-P/--permissions public|private <file> <metadatafile>\n")
			.concat("\t-N/--nometadata <file> <lfn>\n")
			.concat("\t-N/--nometadata -P/--permissions public|private <file> <lfn>\n")
			.concat("\t-R/--recursive <directory> <metatdatadirectory>\n")
			.concat("\t-R/--recursive -N/--nometadata <directory> <lfn>\n")
			.concat("\t-R/--recursive -N/--nometadata -P/--permissions public|private <directory> <lfn>\n")
			.concat("\t-E/--ensemble <ensemblemetadatafile>\n\n");

			return str;*/

/* this is Java 5 
	    StringBuilder sb = new StringBuilder();
	    sb.append("Usage: java uk.ac.ed.epcc.qcdgrid.metadata.QCDgridSubmitter\n")
		.append("\t<file> <metadatafile>\n")
		.append("\t-P/--permissions public|private <file> <metadatafile>\n")
		.append("\t-N/--nometadata <file> <lfn>\n")
		.append("\t-N/--nometadata -P/--permissions public|private <file> <lfn>\n")
		.append("\t-R/--recursive <directory> <metatdatadirectory>\n")
		.append("\t-R/--recursive -N/--nometadata <directory> <lfn>\n")
		.append("\t-R/--recursive -N/--nometadata -P/--permissions public|private <directory> <lfn>\n")
		.append("\t-E/--ensemble <ensemblemetadatafile>\n\n");
	    
	    return sb.toString();

*/
	}


}   


