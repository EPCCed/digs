/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.*;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryTypeManager;
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.FeatureConfiguration;
import uk.ac.ed.epcc.qcdgrid.metadata.QCDgridLogicalNameExtractor;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ResultInfo;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MetadataDocumentRetriever;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.WebServiceMetadataDocumentRetriever;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.MainGUIFrame;

import java.io.*;
import java.util.Vector;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.Attributes;

/**
 * Title: QCDgridFrame Description: The core of the QCDgrid result handler GUI.
 * Creates a window with a list of the XML documents that matched down the left
 * hand side, a tree view of the selected document on the right hand side and
 * some buttons for manipulating the results at the bottom Copyright: Copyright
 * (c) 2003 Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class QCDgridFrame extends JFrame implements ActionListener,
    ListSelectionListener {

    /** Button which closes the window */
    private JButton closeButton;
    
    /** Button which retrieves data from the grid */
    private JButton getDataButton;

    /** Button which removes data from the grid */
    private JButton removeDataButton;

    /** Button which registers interest in data from the grid */
    private JButton registerInterestButton;

    /** Button which searches within this result set */
    private JButton subQueryButton;

    /** Panel to keep the buttons in order */
    private JPanel buttonPanel;

    /** Centre panel for list and tree */
    private JPanel centrePanel;
    
    /** Panel holding document tree */
    private JPanel treePanel;
    
    /** Scroll pane holding list of matching documents */
    private JScrollPane listPane;
    
    /** Scroll pane holding document tree */
    private JScrollPane docPane;

    /** List of matching documents */
    private JList resultList;

    /** The list model that provides the data for the list */
    private QCDgridListModel listModel;

    /** Label which says how many documents were returned */
    private JLabel countLabel;

    /** Label that shows the source of the displayed metadata document */
    private JLabel sourceLabel_;

    /** The mode of the browser */
    
    private String browserMode_;

    /**
     * Thread which streams the results in from the database in the background and
     * adds them to the list as they come in
     */
    private QCDgridListBuilder listBuilder;
    
    /** Name of selected XML document */
    private ResultInfo currentResult_;
    
    /** Tree representation of selected XML document */
    private JTree documentTree;
    
    /** Radio button for working with selected documents only */
    private JRadioButton selectedButton;
    
    /** Radio button for working with all documents */
    private JRadioButton allButton;
    
    /** Button group to make selected/all buttons mutually exclusive */
    private ButtonGroup buttonGroup;
    
    /** A vector of names of documents retrieved from the database */
    private Vector documentNameCache;
    
    /** The type of the query results retrieved */
    private String queryType_;

    /** scope for sub-queries of this one */
    private String subQueryScope_;
    
    /**
     * A corresponding vector which holds the contents of documents retrieved from
     * the database (as strings)
     */
    private Vector documentContentCache;
    
    /**
     * Constructor: creates a new QCDgrid result handler GUI frame to handle the
     * specified results
     * 
     * @param qr
     *          results of the database query
     */
    public QCDgridFrame(QCDgridQueryResults qr) {
	super("QCDgrid Metadata Results");
	
	setSize(new Dimension(800, 600));
	
	getContentPane().setLayout(new BorderLayout());
	
	buttonPanel = new JPanel();
	buttonPanel.setLayout(new FlowLayout());
	getContentPane().add(buttonPanel, BorderLayout.SOUTH);
	
	centrePanel = new JPanel();
	centrePanel.setLayout(new GridLayout(1, 2));
	getContentPane().add(centrePanel, BorderLayout.CENTER);
	
	treePanel = new JPanel();
	treePanel.setLayout(new BorderLayout());
	
	listModel = new QCDgridListModel();
	resultList = new JList(listModel);
	resultList.addListSelectionListener(this);
	
	listPane = new JScrollPane(resultList);
	docPane = new JScrollPane(treePanel);
	centrePanel.add(listPane);
	centrePanel.add(docPane);
	
	buttonGroup = new ButtonGroup();
	selectedButton = new JRadioButton("Selected");
	allButton = new JRadioButton("All");
	buttonGroup.add(selectedButton);
	buttonGroup.add(allButton);
	buttonPanel.add(selectedButton);
	buttonPanel.add(allButton);
	allButton.setSelected(true);
	allButton.addActionListener(this);
	selectedButton.addActionListener(this);
	
	getDataButton = new JButton("Get Data");
	getDataButton.addActionListener(this);
	buttonPanel.add(getDataButton);
	
	registerInterestButton = new JButton("Register Interest");
	registerInterestButton.addActionListener(this);
	buttonPanel.add(registerInterestButton);
	
	removeDataButton = new JButton("Remove Data");
	removeDataButton.addActionListener(this);
	buttonPanel.add(removeDataButton);
	
	subQueryButton = new JButton("Sub-Query");
	subQueryButton.addActionListener(this);
	buttonPanel.add(subQueryButton);

	closeButton = new JButton("Close");
	closeButton.addActionListener(this);
	buttonPanel.add(closeButton);
	
	queryType_ = qr.getQueryTypeInfo();
	int c = qr.count();
	
	if (c == 1) {
	    countLabel = new JLabel("1 match found");
	} else {
	    countLabel = new JLabel("" + c + " matches found");
	}
	sourceLabel_ = new JLabel("");
	getContentPane().add(countLabel, BorderLayout.NORTH);
	getContentPane().add(sourceLabel_, BorderLayout.NORTH);
	
	/* Start up list builder thread */
	listBuilder = new QCDgridListBuilder(qr, listModel);
	
	currentResult_ = null;
	documentTree = null;
	
	documentNameCache = new Vector();
	documentContentCache = new Vector();
	
	if (c == 0) {
	    getDataButton.setEnabled(false);
	    registerInterestButton.setEnabled(false);
	    removeDataButton.setEnabled(false);
	}
	
	try {
	    FeatureConfiguration cfg = new FeatureConfiguration();
	    this.browserMode_ = cfg.getBrowserMode();
	} catch (FeatureConfiguration.FeatureConfigurationException fe) {
	    System.out
		.println("Feature configuration exception occured, defaulting to UKQCD.");
	    this.browserMode_ = FeatureConfiguration.UKQCD_MODE;
	}
	
	if (this.browserMode_.equals(FeatureConfiguration.ILDG_MODE)) {
	    getDataButton.setEnabled(false);
	    registerInterestButton.setEnabled(false);
	    removeDataButton.setEnabled(false);
	}

	/* Enable sub-query button if a sub-query scope is defined */
	QueryTypeManager qtm = QueryTypeManager.getInstance();
	QueryType qt = qtm.queryTypeFromName(queryType_);
	subQueryScope_ = qt.getSubQueryScope();
	if (subQueryScope_ != null) {
	    subQueryButton.setEnabled(true);
	}
	else {
	    subQueryButton.setEnabled(false);
	}

	setVisible(true);
    }
    
    /**
     * Action event handler - performs actions when the buttons at the bottom of
     * the window are used
     * 
     * @param e
     *          action event to be handled
     */
    public void actionPerformed(ActionEvent e) {
	Object source = e.getSource();
	
	if (source == closeButton) {
	    
	    // Stop list builder thread before closing window
	    if (listBuilder.isAlive()) {
		listBuilder.stop();
		while (listBuilder.isAlive())
		    ;
	    }
	    
	    dispose();
	} else if (source == subQueryButton) {
	    // Work out which file to use, based on state of list box
	    ResultInfo[] filesToGet;
	    
	    System.out.println("Running sub-query on selected files");
	    Object[] selected = resultList.getSelectedValues();
	    filesToGet = new ResultInfo[selected.length];
	    for (int i = 0; i < selected.length; i++) {
		// System.out.println(selected[i].getClass().toString());
		filesToGet[i] = (ResultInfo) selected[i];
	    }

	    if (filesToGet.length != 1) {
		JOptionPane.showMessageDialog(null,
					      "Cannot run sub-query on multiple results", "Error",
					      JOptionPane.ERROR_MESSAGE);
		return;
	    }
	    
	    String document;

	    try {
		document = getMetadataDocument(filesToGet[0]);
	    }
	    catch (Exception e2) {
		System.err.println("Error occurred getting metadata: " + e2);
		return;
	    }

	    String scopestr = XmlUtils.getValueFromPath(document, subQueryScope_);
	    
	    if (scopestr != null) {
		scopestr = scopestr.trim();
		//System.out.println("Scope string: " + scopestr);

		MainGUIFrame mgf = MainGUIFrame.getMainGUIFrame();
		mgf.setScope(scopestr);
	    }


	    // Stop list builder thread before closing window
	    if (listBuilder.isAlive()) {
		listBuilder.stop();
		while (listBuilder.isAlive())
		    ;
	    }
	    
	    dispose();

	} else if (source == getDataButton) {
	    
	    // Work out which files to get, based on state of list box and
	    // radio buttons
	    ResultInfo[] filesToGet;
	    
	    if (allButton.isSelected()) {
		// Have to wait for all results to be retrieved before we can
		// get all files
		while (listBuilder.isAlive())
		    ;
		System.out.println("Getting all files from grid...");
		filesToGet = new ResultInfo[listModel.getSize()];
		for (int i = 0; i < filesToGet.length; i++) {
		    filesToGet[i] = (ResultInfo) listModel.getElementAt(i);
		}
	    } else {
		// Selected files only. Make array of their names
		System.out.println("Getting selected files from grid...");
		Object[] selected = resultList.getSelectedValues();
		filesToGet = new ResultInfo[selected.length];
		for (int i = 0; i < selected.length; i++) {
		    // System.out.println(selected[i].getClass().toString());
		    filesToGet[i] = (ResultInfo) selected[i];
		}
	    }
	    
	    // Get user to select file/directory to save into
	    JFileChooser chooser = new JFileChooser();
	    
	    if (filesToGet.length == 1) {
		chooser.setDialogTitle("Save data as file...");
	    } else {
		chooser.setDialogTitle("Save data in directory...");
		chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
	    }
	    
	    int result = chooser.showSaveDialog(this);
	    
	    if (result == JFileChooser.APPROVE_OPTION) {
		
		// User approved 'get' operation
		File f = chooser.getSelectedFile();
		
		System.out.println("Saving as " + f);
		String[] filenames = new String[filesToGet.length];
		for (int i = 0; i < filesToGet.length; i++) {
		    filenames[i] = filesToGet[i].getResultIdentifier();
		}
		try {
		    // Retrieve all documents from database
		    String[] documents = new String[(filesToGet.length)];
		    for (int i = 0; i < filesToGet.length; i++) {
			documents[i] = getMetadataDocument(filesToGet[i]);
		    }
		    
		    // Retrieve corresponding data files from grid
		    QCDgridFileRetriever qfr = new QCDgridFileRetriever(filenames, f,
									documents);
		} catch (Exception ex) {
		    System.err.println("Exception occurred getting data: " + ex);
		}
	    } else {
		// User cancelled 'get'
		System.out.println("Aborted");
	    }
	} else if (source == registerInterestButton) {
	    
	    // Work out which files to get, based on state of list box and
	    // radio buttons
	    ResultInfo[] filesToGet;
	    
	    if (allButton.isSelected()) {
		// Have to wait for all results to be retrieved before we can
		// get all files
		while (listBuilder.isAlive())
		    ;
		System.out.println("Registering interest in all files from grid...");
		filesToGet = new ResultInfo[listModel.getSize()];
		for (int i = 0; i < filesToGet.length; i++) {
		    filesToGet[i] = (ResultInfo) listModel.getElementAt(i);
		}
	    } else {
		// Selected files only. Make array of their names
		System.out
		    .println("Registering interest in selected files from grid...");
		Object[] selected = resultList.getSelectedValues();
		filesToGet = new ResultInfo[selected.length];
		for (int i = 0; i < selected.length; i++) {
		    filesToGet[i] = (ResultInfo) selected[i];
		}
	    }
	    
	    try {
		// Retrieve all documents from database
		String[] documents = new String[(filesToGet.length)];
		for (int i = 0; i < filesToGet.length; i++) {
		    documents[i] = getMetadataDocument(filesToGet[i]);
		}
		
		// Register interest in corresponding data files on grid
		QCDgridFileToucher qft = new QCDgridFileToucher(documents);
	    } catch (Exception ex) {
		System.err.println("Exception occurred touching files: " + ex);
	    }
	    
	} else if (source == removeDataButton) {
	    
	    // Get confirmation before removing files
	    try {
		if (JOptionPane.showConfirmDialog(null,
						  "Delete files from grid: are you sure?", "Confirm file deletion",
						  JOptionPane.YES_NO_OPTION) != JOptionPane.YES_OPTION) {
		    return;
		}
	    } catch (Exception ex) {
		System.err
		    .println("Exception occurred while showing confirmation dialog: "
			     + ex);
		return;
	    }
	    
	    // Work out which files to get, based on state of list box and
	    // radio buttons
	    ResultInfo[] filesToGet;
	    
	    if (allButton.isSelected()) {
		// Have to wait for all results to be retrieved before we can
		// get all files
		while (listBuilder.isAlive())
		    ;
		System.out.println("Removing all files from grid...");
		filesToGet = new ResultInfo[listModel.getSize()];
		for (int i = 0; i < filesToGet.length; i++) {
		    filesToGet[i] = (ResultInfo) listModel.getElementAt(i);
		}
	    } else {
		// Selected files only. Make array of their names
		System.out.println("Removing selected files from grid...");
		Object[] selected = resultList.getSelectedValues();
		filesToGet = new ResultInfo[selected.length];
		for (int i = 0; i < selected.length; i++) {
		    filesToGet[i] = (ResultInfo) selected[i];
		}
	    }
	    
	    try {
		// Retrieve all documents from database
		String[] documents = new String[(filesToGet.length)];
		for (int i = 0; i < filesToGet.length; i++) {
		    documents[i] = getMetadataDocument(filesToGet[i]);
		}
		
		// Register interest in corresponding data files on grid
		QCDgridFileRemover qfr = new QCDgridFileRemover(documents);
	    } catch (Exception ex) {
		System.err.println("Exception occurred removing files: " + ex);
	    }
	}
	// When radio button status changes, make sure the 'Get' button is
	// in a suitable state
	else if (source == allButton) {
	    if (listModel.getSize() != 0) {
		if (this.browserMode_.equals(FeatureConfiguration.UKQCD_MODE)) {
		    getDataButton.setEnabled(true);
		    registerInterestButton.setEnabled(true);
		    removeDataButton.setEnabled(true);
		} else {
		    getDataButton.setEnabled(false);
		    registerInterestButton.setEnabled(false);
		    removeDataButton.setEnabled(false);
		}
	    }
	    
	    else {
		if (this.browserMode_.equals(FeatureConfiguration.UKQCD_MODE)) {
		    getDataButton.setEnabled(true);
		    registerInterestButton.setEnabled(true);
		    removeDataButton.setEnabled(true);
		} else {
		    getDataButton.setEnabled(false);
		    registerInterestButton.setEnabled(false);
		    removeDataButton.setEnabled(false);
		}
	    }
	} else if (source == selectedButton) {
	    if (resultList.getSelectedValue() != null) {
		if (this.browserMode_.equals(FeatureConfiguration.UKQCD_MODE)) {
		    getDataButton.setEnabled(true);
		    registerInterestButton.setEnabled(true);
		    removeDataButton.setEnabled(true);
		} else {
		    getDataButton.setEnabled(false);
		    registerInterestButton.setEnabled(false);
		    removeDataButton.setEnabled(false);
		}
	    } else {
		getDataButton.setEnabled(false);
		registerInterestButton.setEnabled(false);
		removeDataButton.setEnabled(false);
	    }
	}
    }
    
    private String getBrowserMode() {
	return browserMode_;
    }
    
    /**
     * Returns the content of an XML document given its name
     * 
     * @param currDoc
     *          name of document to retrieve
     * @return the content of the document, as a string
     */
    private String getMetadataDocument(ResultInfo currRes) throws Exception {
	String doc = null;
	// First see if it's cached already
	for (int i = 0; i < documentNameCache.size(); i++) {
	    if (((String) documentNameCache.elementAt(i)).equals(currRes
								 .getResultIdentifier())) {
		doc = (String) documentContentCache.elementAt(i);
		break;
	    }
	}
	
	if (doc == null) {
	    // Not cached, so get it from the database and cache it now
	    MetadataDocumentRetriever retriever = null;
	    // Figure out the type of retriever to use. Could use inversion of control
	    // patther here? This will do for now.
	    if (this.getBrowserMode().equals(FeatureConfiguration.ILDG_MODE)) {
		retriever = new WebServiceMetadataDocumentRetriever();
	    } else if (this.getBrowserMode().equals(FeatureConfiguration.UKQCD_MODE)) {
		QCDgridExistDatabase dd = QCDgridExistDatabase.getDriver();
		retriever = (MetadataDocumentRetriever) dd;
	    }
	    if (this.queryType_.equals(QueryType.CONFIGURATION_QUERY_TYPE)) {
		// Get configuration document
		////System.out.println("Getting configuration document.");
		doc = retriever.getConfigurationMetadataDocument(currRes
								 .getResultIdentifier(), currRes.getResultSource());
	    } else if (this.queryType_.equals(QueryType.ENSEMBLE_QUERY_TYPE)) {
		// Get ensemble document
		////System.out.println("Getting ensemble document.");
		doc = retriever.getEnsembleMetadataDocument(currRes
							    .getResultIdentifier(), currRes.getResultSource());
		
	    } else if (this.queryType_.equals(QueryType.CORRELATOR_QUERY_TYPE)) {

		doc = retriever.getCorrelatorMetadataDocument(currRes.getResultIdentifier(),
							      currRes.getResultSource());
	    }
	    documentNameCache.add(currRes.getResultIdentifier());
	    if (doc == null) {
		doc = "<couldNotRetrieveDocument/>";
	    }
	    documentContentCache.add(doc);
	}
	return doc;
    }
    
    /**
     * Called when a new document is selected, to build the tree representation of
     * it and make sure it's displayed
     */
    private void buildDocumentTree() {
	
	if (documentTree != null) {
	    treePanel.remove(documentTree);
	    documentTree = null;
	}
	
	if (currentResult_ == null) {
	    docPane.validate();
	    return;
	}
	
	try {
	    ////System.out.println("Getting document in buildDocumentTree");
	    String doc = getMetadataDocument(currentResult_);
	    if (doc == null) {
		System.out.println("document null in buildDocumentTree");
	    }
	    InstanceTreeBuilder itb = new InstanceTreeBuilder(
		new ByteArrayInputStream(doc.getBytes()), currentResult_
		.getResultIdentifier());
	    
	    documentTree = new JTree(itb.getTree());
	    treePanel.add(documentTree, BorderLayout.CENTER);
	} catch (Exception e) {
	    System.err.println("Exception in buildDocumentTree: " + e);
	    ////e.printStackTrace();
	}
	docPane.validate();
    }
    
    /**
     * List selection event handler
     * 
     * @param lse
     *          the event to handle
     */
    public void valueChanged(ListSelectionEvent lse) {
	
	Object sel = resultList.getSelectedValue();
	
	if (sel == null) {
	    // If nothing selected, disable 'get' button and set
	    // no current document
	    getDataButton.setEnabled(false);
	    registerInterestButton.setEnabled(false);
	    removeDataButton.setEnabled(false);
	    currentResult_ = null;
	} else {
	    if (this.browserMode_.equals(FeatureConfiguration.UKQCD_MODE)) {
		getDataButton.setEnabled(true);
		registerInterestButton.setEnabled(true);
		removeDataButton.setEnabled(true);
	    } else {
		getDataButton.setEnabled(false);
		registerInterestButton.setEnabled(false);
		removeDataButton.setEnabled(false);
	    }
	    if (currentResult_ == (ResultInfo) sel) {
		return;
	    }
	    currentResult_ = (ResultInfo) sel;
	    sourceLabel_.setText("Document source: "
				 + currentResult_.getResultSource());
	}
	
	// Update the document tree view
	buildDocumentTree();
    }
}
