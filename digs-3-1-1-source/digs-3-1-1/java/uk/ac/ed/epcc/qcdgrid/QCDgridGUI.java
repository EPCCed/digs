/***********************************************************************
 *
 *   Filename:   QCDgridGUI.java
 *
 *   Authors:    James Perry            (jamesp)   EPCC.
 *
 *   Purpose:    Provides a single unified GUI for accessing QCDgrid
 *               functionality
 *
 *   Contents:   QCDgridGUI class which implements a "main menu" window
 *               and some of its simpler sub-functions
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
package uk.ac.ed.epcc.qcdgrid;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import java.io.*;

import uk.ac.ed.epcc.qcdgrid.browser.*;
import uk.ac.ed.epcc.qcdgrid.metadata.*;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.QCDgridProgressWindow;
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;

/**
 * QCDgridGUI class - implements a main menu window for accessing various
 * QCDgrid functions Refactored slightly to take account of QCDgridProgressBar
 * changes.
 */
public class QCDgridGUI extends JFrame implements ActionListener {

  /** Widgets and stuff */
  GridLayout topLevelLayout;

  JButton browserButton;

  JPanel browserButtonPanel;

  JButton submissionButton;

  JPanel submissionButtonPanel;

  JButton adminButton;

  JPanel adminButtonPanel;

  JPanel padding;

  JButton quitButton;

  JPanel quitButtonPanel;

  Browser browser = null;

  private QCDgridSubmitter submitter = null;

  /**
   * Constructor: creates the main menu window and displays it.
   * 
   * Change June 2006: This now only creates and shows the 
   */
  public QCDgridGUI() {
    super("QCDgrid Main Menu");
    System.out.println();
    FeatureConfiguration fc;
    String browserMode;
    try{
      fc = new FeatureConfiguration();
      browserMode = fc.getBrowserMode();
    }
    catch(FeatureConfiguration.FeatureConfigurationException fce){
      System.out.println("Browser feature configuration file contains invalid value, "
          + "defaulting to UKQCD.");
      browserMode = FeatureConfiguration.UKQCD_MODE;
    }
    
    System.out.println("The browser is currently operating in "
        + browserMode
        + " mode." );
    
    topLevelLayout = new GridLayout(5, 1);
    getContentPane().setLayout(topLevelLayout);

    // Standard UKQCD and ILDG feature
    browserButton = new JButton("Search Metadata Catalogue");
    browserButtonPanel = new JPanel();
    browserButton.addActionListener(this);
    browserButtonPanel.add(browserButton); 
    getContentPane().add(browserButtonPanel);
    if(browserMode.equals(FeatureConfiguration.UKQCD_MODE)){
      //System.out.println("Adding UKQCD buttons");
      // Just for UKQCD
      submissionButton = new JButton("Submit Data");
      submissionButtonPanel = new JPanel();
      adminButton = new JButton("Administer Grid");
      adminButtonPanel = new JPanel();
      
      submissionButton.addActionListener(this);
  
      submissionButtonPanel.add(submissionButton);
      adminButtonPanel.add(adminButton);
      
      getContentPane().add(submissionButtonPanel);
      getContentPane().add(adminButtonPanel);      
    }
    quitButton = new JButton("Quit");
    quitButtonPanel = new JPanel();
    quitButton.addActionListener(this);
    quitButtonPanel.add(quitButton);   
    getContentPane().add(quitButtonPanel);
    
    padding = new JPanel();
    getContentPane().add(padding);    
    pack();
    setVisible(true);
  }

  /**
   * Called when the submission button is pressed. Prompts the user for
   * file/directory names for data/metadata, then submit it to grid
   */
  private void doSubmission() {
    try {
      JFileChooser dataChooser = new JFileChooser();
      JFileChooser metadataChooser = new JFileChooser();

      File dataFile;
      File metadataFile;
      
      String defaultPermissions = "private";

      // prompt for file name/directory
      dataChooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
      dataChooser
          .setDialogTitle("Select file/directory containing data to submit");

      if (dataChooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {

        dataFile = dataChooser.getSelectedFile();

        if (!dataFile.exists()) {
          throw new QCDgridException("Selected file does not exist");
        }

        if (dataFile.isDirectory()) {

          // If user selected a directory for the data, need a directory for the
          // metadata too
          metadataChooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
          metadataChooser
              .setDialogTitle("Select directory containing metadata");

          if (metadataChooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {

            metadataFile = metadataChooser.getSelectedFile();

            if (!metadataFile.exists()) {
              throw new QCDgridException(
                  "Selected metadata directory does not exist");
            }
            // System.out.println ("doing submission");
            // Submit directories to grid
            if (submitter == null) {
              submitter = new QCDgridSubmitter();
            }
            QCDgridProgressWindow pw = new QCDgridProgressWindow(submitter);
            pw.setVisible(true);
            submitter.submitConfigDirectoryAndMetadata(dataFile, metadataFile,
                pw);
          }
        } else {

          // User chose a single file for the data, need a single file for the
          // metadata also
          metadataChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
          metadataChooser.setDialogTitle("Select file containing metadata");
          metadataChooser
              .setFileFilter(new javax.swing.filechooser.FileFilter() {
                public boolean accept(File f) {
                  if (f.getPath().endsWith(".xml")) {
                    return true;
                  }
                  // Without this, it's impossible to browser the directory
                  // structure
                  if (f.isDirectory()) {
                    return true;
                  }
                  return false;
                }

                public String getDescription() {
                  return "XML Documents";
                }
              });

          if (metadataChooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {

            metadataFile = metadataChooser.getSelectedFile();

            if (!metadataFile.exists()) {
              throw new QCDgridException(
                  "Selected metadata file does not exist");
            }

            // Submit the two files to the grid
            if (submitter == null) {
              submitter = new QCDgridSubmitter();
            }
            submitter.submitConfigFileAndMetadata(dataFile, metadataFile, defaultPermissions);
          }
        }
      }
    } catch (Exception e) {

      // Handle any errors
      try {
        JOptionPane.showMessageDialog(this, e, "Exception occurred",
            JOptionPane.ERROR_MESSAGE);
      } catch (Exception e2) {
        System.err.println("Exception occurred: " + e);
        System.err
            .println("Additional exception occurred while trying to display error dialogue: "
                + e2);
      }
    }
  }

  void exit() {
      if (browser != null) {
	  browser.exit();
      }
      System.exit(0);
  }

  /** Overridden so we can exit when window is closed */
  protected void processWindowEvent(WindowEvent e) {
    super.processWindowEvent(e);
    if (e.getID() == WindowEvent.WINDOW_CLOSING) {
	exit();
    }
  }

  /**
   * Handles clicking on the buttons
   */
  public void actionPerformed(ActionEvent e) {
    Object source = e.getSource();

    if (source == quitButton) {
	exit();
    } else if (source == browserButton) {
	if ((browser == null) || (browser.isFinished())) {
	    browser = new Browser();
	}
    } else if (source == submissionButton) {
      doSubmission();
    }
  }

  /**
   * Main function: creates the GUI window
   */
  public static void main(String[] args) {
    QCDgridGUI qg = new QCDgridGUI();
    // System.out.println( "library path is" + System.getProperty(
    // "java.library.path" ) );
  }
}
