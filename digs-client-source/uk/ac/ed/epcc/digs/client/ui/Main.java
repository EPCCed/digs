/*
 * Main window of DiGS client
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;
import java.io.File;
import java.util.Vector;
import java.util.Observer;
import java.util.Observable;
import java.util.Date;

import uk.ac.ed.epcc.digs.ProxyUtil;
import uk.ac.ed.epcc.digs.DigsException;
import uk.ac.ed.epcc.digs.client.DigsClient;
import uk.ac.ed.epcc.digs.client.LogicalFile;
import uk.ac.ed.epcc.digs.client.LogicalDirectory;
import uk.ac.ed.epcc.digs.client.FileListCache;
import uk.ac.ed.epcc.digs.client.StorageNode;

public class Main extends JFrame implements ActionListener, MouseListener,
				 DocumentListener, TreeExpansionListener,
				 TreeWillExpandListener, TreeSelectionListener {

    /* 
     * number of hours proxy must be valid for at startup. If less than this,
     * proxy initialisation dialogue will be shown immediately
     */
    static final int PROXY_MIN_TIME = 4;

    /*
     * String currently filtering filenames in the grid pane
     */
    String filterString;

    /*
     * Logical filename currently selected in grid pane
     */
    String selectedLfn;
    
    /*
     * Filename currently selected in local pane
     */
    String selectedFilename;

    /*
     * Whether the currently displayed context menu is on the local file tree
     */
    boolean contextLocal;

    /*
     * UI components
     */
    /* upload and download buttons */
    JButton uploadButton;
    JButton downloadButton;

    /* panels for getting layout to look right */
    JPanel uploadPanel;
    JPanel downloadPanel;

    JPanel middlePanel;
    JPanel bottomPanel;
    JPanel proxyPanel;

    JPanel tqPanel;
    JPanel localPanel;
    JPanel localTitlePanel;
    JPanel gridPanel;
    JPanel gridTitlePanel;
    JPanel searchPanel;

    JPanel flowPanel;

    /* grid filter field and button to clear it */
    JButton clearButton;
    JTextField filterField;

    /* path fields for local and grid trees */
    JTextField localPathField;
    JTextField gridPathField;

    /* button to refresh grid tree */
    JButton refreshButton;
    JButton refreshButton2;

    /* proxy bar components */
    JButton regenerateProxyButton;
    JLabel proxyLabel1;
    JLabel proxyLabel2;

    /* main tree components */
    JScrollPane localScrollPane;
    JScrollPane gridScrollPane;
    JTree localTree;
    JTree gridTree;

    /* transfer queue */
    JScrollPane transferScrollPane;
    JList transferQueue;

    /* DiGS logo in middle */
    JLabel logoLabel;

    /* main menu bar */
    JMenuBar menuBar;
    JMenu fileMenu;
    JMenu toolsMenu;
    JMenu helpMenu;
    JMenuItem quitItem;
    JMenuItem searchItem;
    JMenuItem refreshItem;
    JMenuItem helpItem;
    JMenuItem aboutItem;
    JMenuItem nodesItem;
    JMenuItem regenerateProxyItem;
    JMenuItem destroyProxyItem;

    /* popup menu items */
    JMenuItem deleteItem;
    JMenuItem propertiesItem;
    JMenuItem newFolderItem;
    JMenuItem lockFileItem;
    JMenuItem lockFolderItem;
    JMenuItem unlockFileItem;
    JMenuItem unlockFolderItem;
    JMenuItem viewMetadataItem;
    JMenuItem setReplicaCountItem;

    /* tree builder for local file tree */
    LocalFileTreeBuilder localTreeBuilder;

    /* tree builder for grid file tree */
    GridFileTreeBuilder gridTreeBuilder;

    /* whether to destroy the proxy when the UI exits */
    boolean destroyProxyOnExit;

    /* client for access to grid */
    DigsClient digsClient;

    /* path that is currently expanded in grid tree */
    private String gridPathOpened;

    /* same for the local tree */
    private String localPathOpened;

    /* thread that refreshes proxy info and grid tree periodically */
    private RefresherThread refresherThread;

    private String imagePath;

    private TransferTable transferTable;

    //GridBagLayout gbl;

    /*
     * Updates the proxy status panel on the window to reflect the current proxy
     * status.
     */
    public void updateProxyStatus() {
        long timeleft = ProxyUtil.getProxyTimeleft();
        String dn = ProxyUtil.getProxyDN();
        if (dn == null) {
            proxyLabel1.setText("You are not signed in");
            proxyLabel2.setText("");
        }
        else {
            proxyLabel1.setText("You are signed in as: " + dn + ", ");
            timeleft /= 60;
            proxyLabel2.setText("proxy has " + (timeleft/60) + " hours " + (timeleft%60) +
				" minutes remaining");        
        }

	if (timeleft < 119) {
	    // less than 2 hours left, time to regenerate proxy
	    initialiseProxy(false);
	    updateProxyStatus();
	}
    }

    /*
     * Show a dialogue to initialise the user's proxy
     */
    public static void initialiseProxy(boolean wait) {
        try {
            ProxyDialog dlg = new ProxyDialog();

	    if (wait) {
		while (!dlg.isDone()) {
		    Thread.sleep(10);
		}
	    }
        }
        catch (Exception e) {
            JOptionPane.showMessageDialog(null, e.toString(),
                                          "Error creating proxy", JOptionPane.ERROR_MESSAGE);
        }
    }

    /*
     * Constructor. Builds the main window and connects to DiGS grid
     */
    public Main(String imgpath, DigsClient dc) {

        String logoname = imgpath + File.separator + "digs-logo-small.jpeg";

        imagePath = imgpath;

        destroyProxyOnExit = false;

        gridPathOpened = "";
	localPathOpened = "";

        digsClient = dc;

        /*
         * Create menu bar
         */
        menuBar = new JMenuBar();
        fileMenu = new JMenu("File");
        toolsMenu = new JMenu("Tools");
        helpMenu = new JMenu("Help");

        searchItem = new JMenuItem("Search...");
        searchItem.addActionListener(this);
        refreshItem = new JMenuItem("Refresh");
        refreshItem.addActionListener(this);
        quitItem = new JMenuItem("Quit");
        quitItem.addActionListener(this);
        helpItem = new JMenuItem("Help...");
        helpItem.addActionListener(this);
        aboutItem = new JMenuItem("About...");
        aboutItem.addActionListener(this);
        nodesItem = new JMenuItem("Node info...");
        nodesItem.addActionListener(this);
        regenerateProxyItem = new JMenuItem("Regenerate proxy...");
        regenerateProxyItem.addActionListener(this);
        destroyProxyItem = new JMenuItem("Destroy proxy");
        destroyProxyItem.addActionListener(this);

        fileMenu.add(searchItem);
        fileMenu.add(refreshItem);
        fileMenu.add(quitItem);
        toolsMenu.add(nodesItem);
        toolsMenu.add(regenerateProxyItem);
        //toolsMenu.add(destroyProxyItem);
        helpMenu.add(helpItem);
        helpMenu.add(aboutItem);

        menuBar.add(fileMenu);
        menuBar.add(toolsMenu);
        menuBar.add(helpMenu);
        this.setJMenuBar(menuBar);

        /*
         * Items for popup menus
         */
        deleteItem = new JMenuItem("Delete...");
        deleteItem.addActionListener(this);
        propertiesItem = new JMenuItem("Properties...");
        propertiesItem.addActionListener(this);
        newFolderItem = new JMenuItem("New folder...");
        newFolderItem.addActionListener(this);
        lockFileItem = new JMenuItem("Lock file");
        lockFileItem.addActionListener(this);
        lockFolderItem = new JMenuItem("Lock folder");
        lockFolderItem.addActionListener(this);
        unlockFileItem = new JMenuItem("Unlock file");
        unlockFileItem.addActionListener(this);
        unlockFolderItem = new JMenuItem("Unlock folder");
        unlockFolderItem.addActionListener(this);
        viewMetadataItem = new JMenuItem("View metadata...");
        viewMetadataItem.addActionListener(this);
	setReplicaCountItem = new JMenuItem("Set replica count...");
	setReplicaCountItem.addActionListener(this);

        /*
         * Create file trees
         */
        localTreeBuilder = new LocalFileTreeBuilder();
        TreeNode rootNode, gridRoot;

	// FIXME: make initial directory a setting
	String userhome = System.getProperty("user.home");

        /*
         * populate file trees
         */
	try {
	    //System.out.println("Creating file list cache " + (new Date()).toString());
	    FileListCache.create(digsClient);
	    //System.out.println("Building local tree " + (new Date()).toString());

            rootNode = localTreeBuilder.buildTree();

	    //System.out.println("Creating grid tree builder " + (new Date()).toString());

            gridTreeBuilder = new GridFileTreeBuilder(digsClient);

	    //System.out.println("Building grid tree " + (new Date()).toString());
            gridRoot = gridTreeBuilder.buildTree();
	    //System.out.println("Done " + (new Date()).toString());

            /*
             * If there's an OMERO node on the grid, pass its hostname to the OMERO interface
             */
            Vector omeronodes = digsClient.getNodesByType(StorageNode.TYPE_OMERO);
            if (omeronodes.size() > 0) {
                String omerohost = (String)omeronodes.elementAt(0);
                OMEROInterface omero = OMEROInterface.getInstance();
                omero.setHostname(omerohost);
            }
	}
	catch (Exception ex) {
            /* exit if connecting to grid failed */
	    JOptionPane.showMessageDialog(this, ex.toString(),
					  "Error building file trees", JOptionPane.ERROR_MESSAGE);
            dispose();
            System.exit(1);
            return;
	}

        /*
         * Create window layout
         */
        ImageIcon digsLogo = new ImageIcon(logoname);
        logoLabel = new JLabel(digsLogo);

        proxyPanel = new JPanel();
        bottomPanel = new JPanel();
        bottomPanel.setLayout(new BorderLayout());
        //proxyPanel.setLayout(new GridLayout(1, 3, 50, 2));
	proxyPanel.setLayout(new FlowLayout());

        proxyLabel1 = new JLabel("You are signed in as: ");
        proxyLabel2 = new JLabel("Proxy has  hours remaining");

        regenerateProxyButton = new JButton("Regenerate Proxy");
        regenerateProxyButton.addActionListener(this);
        proxyPanel.add(proxyLabel1);
        proxyPanel.add(proxyLabel2);
        proxyPanel.add(regenerateProxyButton);

        transferTable = new TransferTable(digsClient, 3);

        //transferScrollPane = new JScrollPane();
        //transferScrollPane.setPreferredSize(new Dimension(400, 100));
        //transferScrollPane.getViewport().add(transferTable.getJTable());

        tqPanel = new JPanel();
        tqPanel.setLayout(new BorderLayout());
        //tqPanel.add(transferScrollPane, BorderLayout.CENTER);
        tqPanel.add(transferTable.getScrollPane(), BorderLayout.CENTER);
        tqPanel.add(new JLabel("Transfers:"), BorderLayout.NORTH);

        bottomPanel.add(tqPanel, BorderLayout.NORTH);
        bottomPanel.add(proxyPanel, BorderLayout.WEST);
	this.getContentPane().add(bottomPanel, BorderLayout.SOUTH);

	uploadButton = new JButton("Upload >");
	downloadButton = new JButton("Download <");
        uploadButton.addActionListener(this);
        downloadButton.addActionListener(this);

        uploadPanel = new JPanel();
        uploadPanel.setLayout(new FlowLayout());
        downloadPanel = new JPanel();
        downloadPanel.setLayout(new FlowLayout());

        uploadPanel.add(uploadButton);
        downloadPanel.add(downloadButton);

	middlePanel = new JPanel();
	middlePanel.setLayout(new BorderLayout());
	middlePanel.add(uploadPanel, BorderLayout.NORTH);
	middlePanel.add(logoLabel, BorderLayout.CENTER);
	middlePanel.add(downloadPanel, BorderLayout.SOUTH);

	localTree = new JTree(rootNode);
        localTree.setDragEnabled(true);
	localTree.setDropMode(DropMode.ON);
	localTree.setTransferHandler(new TreeTransferHandler(false, this));
	localTree.addMouseListener(this);
        localTree.addTreeExpansionListener(this);
	localTree.addTreeWillExpandListener(this);
        localTree.addTreeSelectionListener(this);
        localTree.setCellRenderer(new TreeRenderer(imgpath));

	gridTree = new JTree(gridRoot);
        gridTree.addMouseListener(this);
        gridTree.setDragEnabled(true);
	gridTree.setDropMode(DropMode.ON);
	gridTree.setTransferHandler(new TreeTransferHandler(true, this));
        gridTree.addTreeExpansionListener(this);
        gridTree.addTreeWillExpandListener(this);
        gridTree.addTreeSelectionListener(this);
        gridTree.setCellRenderer(new TreeRenderer(imgpath));

	refreshButton2 = new JButton("Refresh");
	refreshButton2.addActionListener(this);

	localTitlePanel = new JPanel();
	localTitlePanel.setLayout(new BorderLayout());
	localTitlePanel.add(new JLabel("Local files"), BorderLayout.WEST);
	localPathField = new JTextField();
	localPathField.setText(userhome);
	localPathField.addActionListener(this);
	localTitlePanel.add(localPathField, BorderLayout.CENTER);
	localTitlePanel.add(refreshButton2, BorderLayout.EAST);

	localScrollPane = new JScrollPane();
	localScrollPane.setPreferredSize(new Dimension(500, 400));
	localScrollPane.getViewport().add(localTree);

        localPanel = new JPanel();
        localPanel.setLayout(new BorderLayout());
        localPanel.add(localScrollPane, BorderLayout.CENTER);
        localPanel.add(localTitlePanel, BorderLayout.NORTH);

	gridScrollPane = new JScrollPane();
	gridScrollPane.setPreferredSize(new Dimension(500, 400));
	gridScrollPane.getViewport().add(gridTree);

        refreshButton = new JButton("Refresh");
        refreshButton.addActionListener(this);

        gridTitlePanel = new JPanel();
        gridTitlePanel.setLayout(new BorderLayout());
        gridTitlePanel.add(new JLabel("Grid files"), BorderLayout.WEST);
	gridPathField = new JTextField();
	gridPathField.setText("");
	gridPathField.addActionListener(this);
	gridTitlePanel.add(gridPathField, BorderLayout.CENTER);
        gridTitlePanel.add(refreshButton, BorderLayout.EAST);

        filterField = new JTextField();
        filterField.getDocument().addDocumentListener(this);

        clearButton = new JButton("Clear");
        clearButton.addActionListener(this);

        searchPanel = new JPanel();
        searchPanel.setLayout(new BorderLayout());
        searchPanel.add(new JLabel("Filter: "), BorderLayout.WEST);
        searchPanel.add(filterField, BorderLayout.CENTER);
        searchPanel.add(clearButton, BorderLayout.EAST);

        gridPanel = new JPanel();
        gridPanel.setLayout(new BorderLayout());
        gridPanel.add(gridScrollPane, BorderLayout.CENTER);
        gridPanel.add(gridTitlePanel, BorderLayout.NORTH);
        gridPanel.add(searchPanel, BorderLayout.SOUTH);


        /*flowPanel = new JPanel();
        gbl = new GridBagLayout();
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.fill = GridBagConstraints.BOTH;
        flowPanel.setLayout(gbl);

        gbc.gridwidth = 2;
        gbl.setConstraints(localPanel, gbc);
        flowPanel.add(localPanel);

        gbc.gridwidth = 1;
        gbl.setConstraints(middlePanel, gbc);
        flowPanel.add(middlePanel);

        gbc.gridwidth = 2;
        gbl.setConstraints(gridPanel, gbc);
        flowPanel.add(gridPanel);*/

        middlePanel.setMaximumSize(new Dimension(300, 1000));

	this.getContentPane().add(localPanel, BorderLayout.WEST);
	this.getContentPane().add(gridPanel, BorderLayout.EAST);
	this.getContentPane().add(middlePanel, BorderLayout.CENTER);
        
	this.setMinimumSize(new Dimension(1100, 480));

        //this.getContentPane().add(flowPanel, BorderLayout.CENTER);

        filterString = "";
        selectedLfn = "";
	selectedFilename = "";
	contextLocal = false;

        updateProxyStatus();

        /* Allow us to catch window closing and exit properly */
        enableEvents(AWTEvent.WINDOW_EVENT_MASK | AWTEvent.COMPONENT_EVENT_MASK);

        this.setSize(new Dimension(800, 600));

	pack();

        /* Centre window in screen */
        setLocationRelativeTo(null);

	setVisible(true);

        /* The first tree expansion doesn't seem to do anything but the second one works */
	selectLocalTreePath(userhome);
	selectLocalTreePath(userhome);
        selectGridTreePath("");
        enableTransferButtons();

        /* Start thread to refresh proxy info and grid tree contents */
        refresherThread = new RefresherThread(this);
    }

    /* used to pass the DigsClient object to another thread */
    private static DigsClient dclient;

    public static void main(final String[] args) {
        dclient = null;

        /*
         * Initialise a proxy if one doesn't exist already
         */
	if ((ProxyUtil.getProxyTimeleft() / 3600) < PROXY_MIN_TIME) {
	    initialiseProxy(true);
	}

        /*
         * Connect to DiGS
         */
	try {
            /* show the splash screen */
            SplashScreen splashScreen = new SplashScreen(new ImageIcon(args[0] + File.separator + "digs-logo-splash.jpeg"));

            splashScreen.update("Connecting to DiGS", 0);
	    dclient = new DigsClient(splashScreen);

            splashScreen.update("Done", 100);
            splashScreen.dispose();
	}
	catch (Exception ex) {
            /* exit if connecting to grid failed */
	    JOptionPane.showMessageDialog(null, ex.toString(),
					  "Error connecting to grid", JOptionPane.ERROR_MESSAGE);
            System.exit(1);
            return;
	}

        /*
         * Create the user interface
         */
        SwingUtilities.invokeLater(new Runnable() {
                public void run() {
                    new Main(args[0], dclient);
                }
            });
    }

    /*
     * Called when the filter field changes value
     */
    private void updateFilter() {
        String newFilter = filterField.getText();
        if (!newFilter.equals(filterString)) {
            if (newFilter.equals("")) {
                // No filter: set grid tree back to white
                gridTree.setBackground(Color.WHITE);
            }
            else {
                // Filter active: set yellow background on grid tree
                gridTree.setBackground(new Color(0xff, 0xff, 0xbb));
            }
            filterString = newFilter;
            refreshGridTree(false);

            // ensure that first 2 levels are always expanded
            selectGridTreePath("");
        }
    }

    /*
     * Recursively deletes an entire local directory
     */
    private void recursiveDelete(File file) throws Exception {
	if (!file.exists()) throw new DigsException("File " + file + "doesn't exist");
	if (file.isDirectory()) {
	    File[] contents = file.listFiles();
	    if (contents != null) {
		for (int i = 0; i < contents.length; i++) {
		    recursiveDelete(contents[i]);
		}
	    }
	}
	if (!file.delete()) {
	    throw new DigsException("Error deleting " + file);
	}
    }

    /*
     * Refreshes the local file tree with the latest file list from disk.
     */
    public void refreshLocalTree() {
	int i, j, k;

	File fl = new File(localPathOpened);
	while (!fl.exists()) {
	    fl = fl.getParentFile();
	    if (fl == null) {
		return;
	    }
	    localPathOpened = fl.getPath();
	    selectLocalTreePath(fl.getPath());
	}

	TreePath tpath = filenameToLocalTreePath(localPathOpened);

	for (i = 1; i < tpath.getPathCount(); i++) {
	    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) tpath.getPathComponent(i);

	    Object ftn = dmtn.getUserObject();

	    // If this isn't the last path element, determine next one in case it gets deleted
	    DefaultMutableTreeNode nextnode = null;
	    boolean nextdeleted = false;
	    if (i != (tpath.getPathCount() - 1)) {
		nextnode = (DefaultMutableTreeNode) tpath.getPathComponent(i+1);
	    }

	    if (ftn instanceof LocalDirectoryTreeNode) {
		LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) ftn;
		File ld = ldtn.getFile();
		boolean treechanged = false;

		// List files in this directory
		File[] files;
		try {
		    files = ld.listFiles();
		    if (files == null) {
			files = new File[0];
		    }
		}
		catch (Exception ex) {
		    // bail out if list failed
		    return;
		}

		// Create a "found" list
		boolean[] foundlist = new boolean[files.length];
		for (j = 0; j < foundlist.length; j++) {
		    foundlist[j] = false;
		}

		// Iterate over tree children
		for (j = 0; j < dmtn.getChildCount(); j++) {
		    boolean childFound = false;
		    DefaultMutableTreeNode child = (DefaultMutableTreeNode) dmtn.getChildAt(j);
		    Object uo2 = child.getUserObject();
		    if (uo2 instanceof LocalDirectoryTreeNode) {
			// handle directory children
			LocalDirectoryTreeNode ldtn2 = (LocalDirectoryTreeNode) uo2;

			// look for it in the file list
			for (k = 0; k < files.length; k++) {
			    if (files[k].isDirectory()) {
				if (files[k].getPath().equals(ldtn2.getFile().getPath())) {
				    childFound = true;
				    foundlist[k] = true;
				    break;
				}
			    }
			}
		    }
		    else if (uo2 instanceof LocalFileTreeNode) {
			// handle file children
			LocalFileTreeNode lftn2 = (LocalFileTreeNode) uo2;

			// look for it in the file list
			for (k = 0; k < files.length; k++) {
			    if (!files[k].isDirectory()) {
				if (files[k].getPath().equals(lftn2.getFile().getPath())) {
				    childFound = true;
				    foundlist[k] = true;
				    break;
				}
			    }
			}
		    }
		    
		    if (!childFound) {

			// check here if this is the next level directory being nuked
			if (nextnode == child) {

			    nextdeleted = true;
			}

			// If it is not found, delete it from the tree
			dmtn.remove(j);

			// make sure this doesn't mess up the iteration we're in the middle of
			j--;

			treechanged = true;
		    }
		}

		// Now look for any files in the file list that weren't in the tree
		for (j = 0; j < foundlist.length; j++) {
		    if (foundlist[j] == false) {

			// Add them to the tree as new nodes
			if (!files[j].isDirectory()) {
			    dmtn.add(localTreeBuilder.buildTreeForFile(files[j]));
			}
			else {
			    dmtn.add(localTreeBuilder.fakeTreeForDirectory(files[j]));
			}
			
			treechanged = true;
		    }
		}

		// deal with next level node having been deleted
		if (nextdeleted) {
		    localPathOpened = ldtn.getFile().getPath();
		    selectLocalTreePath(ldtn.getFile().getPath());
		}

		if (treechanged) {
		    // Make sure to tell the tree its structure has changed so that it refreshes
		    DefaultTreeModel model = (DefaultTreeModel) localTree.getModel();
		    model.nodeStructureChanged(dmtn);
		}

		if (nextdeleted) {
		    break;
		}
	    }
	}
    }


    /*
     * Refreshes the grid tree with the latest logical file list from the grid.
     * This is also used (with fullRefresh set to false to make it run quickly) when the filter
     * string changes, to update the grid tree according to the new filter.
     */
    public void refreshGridTree(boolean fullRefresh) {
	int i, j, k;

	// Make sure grid file list is up to date
        if (fullRefresh) {
            FileListCache.getInstance().refresh();
        }

	TreePath tpath = filenameToGridTreePath(gridPathOpened);
	for (i = 1; i < tpath.getPathCount(); i++) {
	    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) tpath.getPathComponent(i);
	    Object ftn = dmtn.getUserObject();
	    
	    // If this isn't the last path element, determine next one in case it gets deleted
	    DefaultMutableTreeNode nextnode = null;
	    boolean nextdeleted = false;
	    if (i != (tpath.getPathCount() - 1)) {
		nextnode = (DefaultMutableTreeNode) tpath.getPathComponent(i+1);
	    }

	    if (ftn instanceof GridDirectoryTreeNode) {
		GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) ftn;
		LogicalDirectory ld = gdtn.getDirectory();
		boolean treechanged = false;

		// List files in this logical directory
		Vector files;
		try {
		    files = ld.getFiles();
		}
		catch (Exception ex) {
		    // bail out if list failed
		    return;
		}

		// Create a "found" list
		boolean[] foundlist = new boolean[files.size()];
		for (j = 0; j < foundlist.length; j++) {
		    foundlist[j] = false;
		}

		// Iterate over tree children.
		for (j = 0; j < dmtn.getChildCount(); j++) {
		    boolean childFound = false;
		    DefaultMutableTreeNode child = (DefaultMutableTreeNode) dmtn.getChildAt(j);
		    Object uo2 = child.getUserObject();
		    if (uo2 instanceof GridDirectoryTreeNode) {
			// handle directory children
			GridDirectoryTreeNode gdtn2 = (GridDirectoryTreeNode) uo2;

			// look for it in the file list
			for (k = 0; k < files.size(); k++) {
			    Object flo = files.elementAt(k);
			    if (flo instanceof LogicalDirectory) {
				LogicalDirectory ld2 = (LogicalDirectory) flo;
				if (ld2.getName().equals(gdtn2.getDirectory().getName())) {
                                    if ((filterString.equals("")) || (gdtn2.getDirectory().matches(filterString))) {
                                        childFound = true;
                                    }
				    foundlist[k] = true;
				    break;
				}
			    }
			}
		    }
		    else if (uo2 instanceof GridFileTreeNode) {
			// handle file children
			GridFileTreeNode gftn2 = (GridFileTreeNode) uo2;

                        // look for it in the file list
                        for (k = 0; k < files.size(); k++) {
                            Object flo = files.elementAt(k);
                            LogicalFile lf2 = (LogicalFile) flo;
                            if (!lf2.isDirectory()) {
                                if (lf2.getName().equals(gftn2.getFile().getName())) {
                                    if ((filterString.equals("")) || (gftn2.getFile().matches(filterString))) {
                                        childFound = true;
                                    }
                                    foundlist[k] = true;
                                    break;
                                }
                            }
                        }
		    }

		    if (!childFound) {

			// check here if this is the next level directory being nuked
			if (nextnode == child) {
			    nextdeleted = true;
			}

			// If it is not found, delete it from the tree
			dmtn.remove(j);

			// make sure this doesn't mess up the iteration we're in the middle of
			j--;

			treechanged = true;
		    }
		}

		// Now look for any files in the file list that weren't in the tree
		for (j = 0; j < foundlist.length; j++) {
		    if (foundlist[j] == false) {
			// Add them to the tree as new nodes
			Object flo = files.elementAt(j);
                        LogicalFile lf2 = (LogicalFile) flo;

                        if (!lf2.isDirectory()) {
                            if ((filterString.equals("")) || (lf2.matches(filterString))) {
                                //dmtn.add(gridTreeBuilder.buildTreeForFile(lf2));
                                gridTreeBuilder.orderedInsert(dmtn, gridTreeBuilder.buildTreeForFile(lf2));
                            }
			}
			else {
			    LogicalDirectory ld2 = (LogicalDirectory) flo;
                            if ((filterString.equals("")) || (ld2.matches(filterString))) {
                                //dmtn.add(gridTreeBuilder.fakeTreeForDirectory(ld2));
                                gridTreeBuilder.orderedInsert(dmtn, gridTreeBuilder.fakeTreeForDirectory(ld2));
                            }
			}

			treechanged = true;
		    }
		}

		if (treechanged) {
		    // Make sure to tell the tree its structure has changed so that it refreshes
		    DefaultTreeModel model = (DefaultTreeModel) gridTree.getModel();
		    model.nodeStructureChanged(dmtn);
		}

		// deal with next level node having been deleted
		if (nextdeleted) {
		    
		    gridPathOpened = gdtn.getDirectory().getPath();
    
		    break;
		}
	    }
	}

	transferTable.addRefresh();
    }

    /*
     * Refreshes the given path in the local file tree, displaying any changes
     * Don't call unless path is a directory!
     */
    private void refreshLocalTreePath(String path) {
	// refresh tree to reflect changes
	TreePath tpath = filenameToLocalTreePath(path);
	if (tpath != null) {
	    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) tpath.getLastPathComponent();
	    LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) dmtn.getUserObject();
	    localTreeBuilder.populateDirectoryChildren(dmtn);
	    
	    DefaultTreeModel model = (DefaultTreeModel) localTree.getModel();
	    model.nodeStructureChanged(dmtn);
	    
	    localTree.scrollPathToVisible(tpath);
	}
    }

    /*
     * Exit the GUI
     */
    public void exit() {
        // stop threads that run in background
        refresherThread.stop();
        FileListCache.getInstance().stop();

        // destroy the main window
        dispose();

        // destroy the proxy if user wanted to
        if (destroyProxyOnExit) {
            try {
                ProxyUtil.destroyProxy();
            }
            catch (Exception ex) {
                JOptionPane.showMessageDialog(this, ex.toString(),
                                              "Error destroying proxy", JOptionPane.ERROR_MESSAGE);                    
            }
        }

	System.exit(0);
    }

    /*
     * Returns a vector of Files from the local tree that should be acted upon by
     * a context menu item. If there is a selection, this will be all the contents of
     * the selection. If not, it will be the single item clicked on.
     */
    private Vector getLocalTargets() {
	Vector v = new Vector();

	TreePath[] selections = localTree.getSelectionPaths();
	if ((selections != null) && (selections.length > 0)) {
	    for (int i = 0; i < selections.length; i++) {
		DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) selections[i].getLastPathComponent();
		Object uo = dmtn.getUserObject();
		if (uo instanceof LocalDirectoryTreeNode) {
		    LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) uo;
		    v.add(ldtn.getFile());
		}
		else if (uo instanceof LocalFileTreeNode) {
		    LocalFileTreeNode lftn = (LocalFileTreeNode) uo;
		    v.add(lftn.getFile());
		}
	    }
	}
	else {
	    // just one item, the one clicked
	    File file = new File(selectedFilename);
	    v.add(file);
	}

	return v;
    }

    /*
     * Returns a vector of LogicalFiles from the grid tree that should be acted upon by
     * a context menu item. If there is a selection, this will be all the contents of
     * the selection. If not, it will be the single item clicked on.
     */
    private Vector getGridTargets() {
	Vector v = new Vector();

	TreePath[] selections = gridTree.getSelectionPaths();
	if ((selections != null) && (selections.length > 0)) {
	    for (int i = 0; i < selections.length; i++) {
		DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) selections[i].getLastPathComponent();
		Object uo = dmtn.getUserObject();
		if (uo instanceof GridDirectoryTreeNode) {
		    GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;
		    v.add(gdtn.getDirectory());
		}
		else if (uo instanceof GridFileTreeNode) {
		    GridFileTreeNode gftn = (GridFileTreeNode) uo;
		    v.add(gftn.getFile());
		}
	    }
	}
	else {
	    // just the one that was clicked on
	    TreePath sel = filenameToGridTreePath(selectedLfn);
	    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) sel.getLastPathComponent();
	    Object uo = dmtn.getUserObject();
	    if (uo instanceof GridDirectoryTreeNode) {
		GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;
		v.add(gdtn.getDirectory());
	    }
	    else if (uo instanceof GridFileTreeNode) {
		GridFileTreeNode gftn = (GridFileTreeNode) uo;
		v.add(gftn.getFile());
	    }
	}
	return v;
    }

    public void actionPerformed(ActionEvent e) {
	Object src = e.getSource();
        if (src == regenerateProxyButton) {
            initialiseProxy(false);
            updateProxyStatus();
        }
        else if (src == regenerateProxyItem) {
            initialiseProxy(false);
            updateProxyStatus();
        }
        else if (src == quitItem) {
            exit();
        }
        else if (src == searchItem) {
            MetadataSearchDialogue msd = new MetadataSearchDialogue(this);
            String str = msd.getSearchString();
            if (str != null) {
                filterField.setText(str);
                updateFilter();
            }
        }
        else if (src == helpItem) {
            // FIXME: implement
        }
        else if (src == aboutItem) {
            JOptionPane.showMessageDialog(this, "DiGS Client v0.9, (c) University of Edinburgh 2008-2009",
                                          "About DiGS Client", JOptionPane.INFORMATION_MESSAGE);
        }
        else if (src == nodesItem) {
            // FIXME: implement
        }
        else if (src == destroyProxyItem) {
            try {
                ProxyUtil.destroyProxy();
            }
            catch (Exception ex) {
                JOptionPane.showMessageDialog(this, ex.toString(),
                                              "Error destroying proxy", JOptionPane.ERROR_MESSAGE);
            }
            updateProxyStatus();
        }
        else if (src == refreshButton) {
            refreshGridTree(true);
        }
	else if (src == refreshButton2) {
	    refreshLocalTree();
	}
        else if (src == clearButton) {
            filterField.setText("");
            updateFilter();
        }
        else if (src == deleteItem) {
	    if (contextLocal) {
		int result;

		Vector v = getLocalTargets();
		if (v.size() == 1) {

		    File file = (File) v.elementAt(0);
		    String parent = file.getParent();

		    if (file.isDirectory()) {
			result = JOptionPane.showConfirmDialog(this, "Are you sure you want to delete the folder " + file.getPath() + "?",
							       "Confirm delete folder", JOptionPane.YES_NO_OPTION);
		    }
		    else {
			result = JOptionPane.showConfirmDialog(this, "Are you sure you want to delete the file " + file.getPath() + "?",
							       "Confirm delete file", JOptionPane.YES_NO_OPTION);
		    }
		    if (result == JOptionPane.YES_OPTION) {
			try {
			    recursiveDelete(file);
			}
			catch (Exception ex) {
			    JOptionPane.showMessageDialog(this, "Error deleting item: " + ex, "Error deleting item",
							  JOptionPane.ERROR_MESSAGE);			
			}
			
			// refresh tree to reflect changes
			refreshLocalTreePath(parent);
		    }
		}
		else {
		    // local delete of multiple items
		    result = JOptionPane.showConfirmDialog(this, "Are you sure you want to delete the selected items?",
							   "Confirm delete multiple items", JOptionPane.YES_NO_OPTION);
		    if (result == JOptionPane.YES_OPTION) {
			try {
			    for (int i = 0; i < v.size(); i++) {
				File file = (File) v.elementAt(i);
				String parent = file.getParent();
				recursiveDelete(file);
				refreshLocalTreePath(parent);
			    }
			}
			catch (Exception ex) {
			    JOptionPane.showMessageDialog(this, "Error deleting items: " + ex, "Error deleting items",
							  JOptionPane.ERROR_MESSAGE);						    
			}
		    }
		}
	    }
	    else {
		int result;
		Vector v = getGridTargets();
		if (v.size() == 1) {
		    // grid delete of single item
		    LogicalFile lf = (LogicalFile) v.elementAt(0);
		    if (lf.isDirectory()) {
			result = JOptionPane.showConfirmDialog(this, "Are you sure you want to delete the folder " + lf.getPath() + "?",
							       "Confirm delete folder", JOptionPane.YES_NO_OPTION);
		    }
		    else {
			result = JOptionPane.showConfirmDialog(this, "Are you sure you want to delete the file " + lf.getPath() + "?",
							       "Confirm delete file", JOptionPane.YES_NO_OPTION);
		    }
		    if (result == JOptionPane.YES_OPTION) {
			try {
			    if (lf.isDirectory()) {
				digsClient.deleteDirectory(lf.getPath());
                                LogicalDirectory.removeNewFolder(lf.getPath());
			    }
			    else {
				digsClient.deleteFile(lf.getPath());
			    }
			}
			catch (Exception ex) {
			    JOptionPane.showMessageDialog(this, "Error deleting item: " + ex, "Error deleting item",
							  JOptionPane.ERROR_MESSAGE);
			}
		    }
		}
		else {
		    // grid delete of multiple items
		    result = JOptionPane.showConfirmDialog(this, "Are you sure you want to delete the selected items?",
							   "Confirm delete multiple items", JOptionPane.YES_NO_OPTION);
		    if (result == JOptionPane.YES_OPTION) {
			try {
			    for (int i = 0; i < v.size(); i++) {
				LogicalFile lf = (LogicalFile) v.elementAt(i);
				if (lf.isDirectory()) {
				    digsClient.deleteDirectory(lf.getPath());
                                    LogicalDirectory.removeNewFolder(lf.getPath());
				}
				else {
				    digsClient.deleteFile(lf.getPath());
				}
			    }
			}
			catch (Exception ex) {
			    JOptionPane.showMessageDialog(this, "Error deleting items: " + ex, "Error deleting items",
							  JOptionPane.ERROR_MESSAGE);
			}
		    }
		}
	    }
        }
        else if (src == propertiesItem) {
	    if (contextLocal) {
		// local file properties
                File file = new File(selectedFilename);
                new FilePropertiesDialogue(this, file, imagePath);
	    }
	    else {
                TreePath sel = filenameToGridTreePath(selectedLfn);
		// grid file properties
                if (sel != null) {
                    String dest = "";
                    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)sel.getLastPathComponent();
                    Object uo = dmtn.getUserObject();
                    if (uo instanceof GridFileTreeNode) {
                        GridFileTreeNode gftn = (GridFileTreeNode) uo;
                        LogicalFile lf = gftn.getFile();

                        new FilePropertiesDialogue(this, lf, imagePath);
                    }
                    else if (uo instanceof GridDirectoryTreeNode) {
                        GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;

                        new FilePropertiesDialogue(this, gdtn.getDirectory(), imagePath);
                    }
                }
	    }
        }
        else if (src == newFolderItem) {
	    if (contextLocal) {
		String name = JOptionPane.showInputDialog(this, "Enter folder name");
		if (name != null) {
		    File newfolder = new File(selectedFilename + File.separator + name);
		    String parent = newfolder.getParent();
		    try {
			if (!newfolder.mkdir()) {
			    throw new DigsException("Creating folder failed");
			}
		    }
		    catch (Exception ex) {
			JOptionPane.showMessageDialog(this, "Error creating folder: " + ex, "Error creating folder",
						      JOptionPane.ERROR_MESSAGE);
		    }
		    refreshLocalTreePath(parent);
		}
	    }
	    else {
		String name = JOptionPane.showInputDialog(this, "Enter folder name");
		if (name != null) {

                    String fullname;
                    if (selectedLfn.equals("")) {
                        fullname = name;
                    }
                    else {
                        fullname = selectedLfn + "/" + name;
                    }
                    LogicalDirectory.addNewFolder(fullname);

                    refreshGridTree(false);
		}
	    }
        }
        else if ((src == lockFileItem) || (src == lockFolderItem)) {
	    Vector v = getGridTargets();
	    try {
		for (int i = 0; i < v.size(); i++) {
		    LogicalFile lf = (LogicalFile) v.elementAt(i);
		    if (lf.isDirectory()) {
			digsClient.lockDirectory(lf.getPath());
		    }
		    else {
			digsClient.lockFile(lf.getPath());
		    }
		}
	    }
	    catch (Exception ex) {
		JOptionPane.showMessageDialog(this, "Error locking: " + ex, "Error locking",
					      JOptionPane.ERROR_MESSAGE);
	    }
        }
        else if ((src == unlockFileItem) || (src == unlockFolderItem)) {
	    Vector v = getGridTargets();
	    try {
		for (int i = 0; i < v.size(); i++) {
		    LogicalFile lf = (LogicalFile) v.elementAt(i);
		    if (lf.isDirectory()) {
			digsClient.unlockDirectory(lf.getPath());
		    }
		    else {
			digsClient.unlockFile(lf.getPath());
		    }
		}
	    }
	    catch (Exception ex) {
		JOptionPane.showMessageDialog(this, "Error unlocking: " + ex, "Error unlocking",
					      JOptionPane.ERROR_MESSAGE);
	    }
        }
	else if (src == setReplicaCountItem) {
	    Vector v = getGridTargets();
	    String currentrc = "";
	    // get initial value if single file
	    if (v.size() == 1) {
		LogicalFile lf = (LogicalFile) v.elementAt(0);
		if (!lf.isDirectory()) {
		    int rc = lf.getReplicaCount();
		    if (rc != 0) {
			currentrc = "" + rc;
		    }
		}
	    }
	    String result = JOptionPane.showInputDialog(this, "Enter replica count", currentrc);
	    if (result != null) {
		try {
		    int rc = Integer.parseInt(result);
		    for (int i = 0; i < v.size(); i++) {
			LogicalFile lf = (LogicalFile) v.elementAt(i);
			if (lf.isDirectory()) {
			    digsClient.setReplicaCountDirectory(lf.getPath(), rc);
			}
			else {
			    digsClient.setReplicaCount(lf.getPath(), rc);
			}
		    }
		}
		catch (Exception ex) {
		    JOptionPane.showMessageDialog(this, "Error setting replica count: " + ex, "Error setting replica count",
						  JOptionPane.ERROR_MESSAGE);
		}
	    }
	}
        else if (src == viewMetadataItem) {
            // FIXME: implement
        }
	else if (src == localPathField) {
	    // called when user presses enter in local path field
	    selectLocalTreePath(localPathField.getText());
	}
	else if (src == gridPathField) {
	    // called when user presses enter in grid path field
            selectGridTreePath(gridPathField.getText());
	}
        else if (src == uploadButton) {
            TreePath[] sels = gridTree.getSelectionPaths();
            if ((sels != null) && (sels.length >= 1)) {
                String dest = "";
                DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)sels[0].getLastPathComponent();
                Object uo = dmtn.getUserObject();
                if (uo instanceof GridFileTreeNode) {
                    GridFileTreeNode gftn = (GridFileTreeNode) uo;
                    String destfile = gftn.getFile().getPath();
                    int slash = destfile.lastIndexOf('/');
                    if (slash >= 0) {
                        dest = destfile.substring(0, slash);
                    }
                }
                else if (uo instanceof GridDirectoryTreeNode) {
                    GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;
                    dest = gdtn.getDirectory().getPath();
                }

                startGridPut(dest);
            }
        }
        else if (src == downloadButton) {
            TreePath[] sels = localTree.getSelectionPaths();
            if ((sels != null) && (sels.length >= 1)) {
                String dest = "";
                DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)sels[0].getLastPathComponent();
                Object uo = dmtn.getUserObject();
                if (uo instanceof LocalFileTreeNode) {
                    LocalFileTreeNode lftn = (LocalFileTreeNode) uo;
                    dest = lftn.getFile().getParent();
                }
                else if (uo instanceof LocalDirectoryTreeNode) {
                    LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) uo;
                    dest = ldtn.getFile().getPath();
                }

                startGridGet(dest);
            }
        }


    }

    public void mouseClicked(MouseEvent e) {
        /* respond to right clicks */
        if (e.getButton() == MouseEvent.BUTTON3) {

            if (e.getSource() == gridTree) {
                // open context menu on grid tree
                TreePath path = gridTree.getPathForLocation(e.getX(), e.getY());
                if (path != null) {
                    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)path.getLastPathComponent();
                    Object userobj = dmtn.getUserObject();

                    JPopupMenu popup = new JPopupMenu();
                    popup.add(deleteItem);
                    popup.add(propertiesItem);
		    popup.add(setReplicaCountItem);

                    contextLocal = false;

                    if (userobj instanceof GridFileTreeNode) {
                        GridFileTreeNode gftn = (GridFileTreeNode) userobj;
                        selectedLfn = gftn.getFile().getPath();
                        // FIXME: add optional view metadata item
                        if (gftn.getFile().isLocked()) {
                            popup.add(unlockFileItem);
                        }
                        else {
                            popup.add(lockFileItem);
                        }
                    }
                    else if (userobj instanceof GridDirectoryTreeNode) {
                        GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) userobj;
                        selectedLfn = gdtn.getDirectory().getPath();

                        popup.add(newFolderItem);
                        if (gdtn.getDirectory().isLocked()) {
                            popup.add(unlockFolderItem);
                        }
                        else {
                            popup.add(lockFolderItem);
                        }
                    }

                    popup.show(gridTree, e.getX(), e.getY());

		    // if clicked on a non-selected node, deselect any that are selected
		    boolean isselected = false;
		    TreePath[] selections = gridTree.getSelectionPaths();
		    if (selections != null) {
			for (int i = 0; i < selections.length; i++) {
			    if (selections[i].getLastPathComponent() == dmtn) {
				isselected = true;
			    }
			}
			if (!isselected) {
			    gridTree.clearSelection();
			}
		    }
                }
            }
	    else if (e.getSource() == localTree) {
                // open context menu on local tree
                TreePath path = localTree.getPathForLocation(e.getX(), e.getY());
                if (path != null) {
		    DefaultMutableTreeNode node = null;
                    Object[] pathobj = path.getPath();
                    selectedFilename = "";
                    for (int i = 1; i < pathobj.length; i++) {
                        node = (DefaultMutableTreeNode) pathobj[i];
                        selectedFilename = selectedFilename + node.toString();
                        if (i != (pathobj.length-1)) {
                            selectedFilename = selectedFilename + File.separator;
                        }
                    }
		    contextLocal = true;
		    File file = new File(selectedFilename);
		    if (file.exists()) {
			JPopupMenu popup = new JPopupMenu();
			popup.add(deleteItem);
			if (file.isDirectory()) {
			    popup.add(newFolderItem);
			}
                        popup.add(propertiesItem);
			popup.show(localTree, e.getX(), e.getY());

			// If right clicked a non-selected node, deselect any that were selected
			boolean isselected = false;
			TreePath[] selections = localTree.getSelectionPaths();
			if (selections != null) {
			    for (int i = 0; i < selections.length; i++) {
				if (selections[i].getLastPathComponent() == node) {
				    isselected = true;
				}
			    }
			    if (!isselected) {
				localTree.clearSelection();
			    }
			}
		    }
		}
	    }
        }
    }

    public void mouseEntered(MouseEvent e) {
    }

    public void mouseExited(MouseEvent e) {
    }

    public void mousePressed(MouseEvent e) {
    }

    public void mouseReleased(MouseEvent e) {
    }

    /*
     * Update the grid tree whenever user edits filter string
     */
    public void insertUpdate(DocumentEvent e) {
        updateFilter();
    }

    public void removeUpdate(DocumentEvent e) {
        updateFilter();
    }

    public void changedUpdate(DocumentEvent e) {
    }


    public void treeExpanded(TreeExpansionEvent event) {
    }

    public void treeCollapsed(TreeExpansionEvent event) {

    }

    /*
     * Converts a logical filename to its corresponding path in the grid tree.
     * Returns null if it doesn't refer to a valid tree path.
     */
    private TreePath filenameToGridTreePath(String path) {
        int i, j;
        Vector vpath = new Vector();

        TreeModel tm = gridTree.getModel();
        DefaultMutableTreeNode rootNode = (DefaultMutableTreeNode)tm.getRoot();
        vpath.add(rootNode);
        DefaultMutableTreeNode rootNode2 = (DefaultMutableTreeNode)rootNode.getChildAt(0);
        vpath.add(rootNode2);

        // just select the root for an empty string
        if (!path.equals("")) {
            DefaultMutableTreeNode dirNode = rootNode2;
            DefaultMutableTreeNode nextNode;
            boolean foundFile = false;
            String[] components = path.split("/");
            for (i = 0; i < components.length; i++) {
                nextNode = null;
                for (j = 0; j < dirNode.getChildCount(); j++) {
                    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)dirNode.getChildAt(j);
                    Object uo = dmtn.getUserObject();
                    if (uo instanceof GridDirectoryTreeNode) {
                        GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;
                        if (gdtn.toString().equals(components[i])) {
                            nextNode = dmtn;
                            vpath.add(dmtn);
                            if (!gdtn.isExpanded()) {
                                gdtn.expand();
                                try {
                                    gridTreeBuilder.populateDirectoryChildren(dmtn);
                                }
                                catch (Exception ex) {
                                    return null;
                                }
                            }
                        }
                    }
                    else {
                        // found a file, reached end of path
                        GridFileTreeNode gftn = (GridFileTreeNode) uo;
                        if (gftn.toString().equals(components[i])) {
                            foundFile = true;
                            vpath.add(dmtn);
                        }
		    }
		}
                if (nextNode != null) {
                    dirNode = nextNode;
                }
                else {
                    break;
                }
            }
        }

	Object[] opath = new Object[vpath.size()];
	for (i = 0; i < vpath.size(); i++) {
	    opath[i] = vpath.elementAt(i);
	}
	TreePath tpath = new TreePath(opath);
	return tpath;
    }

    /*
     * Converts a local physical filename to a path in the local file tree.
     * Returns null if it doesn't refer to a valid tree path.
     */
    private TreePath filenameToLocalTreePath(String path) {
	boolean caseSensitive = false;

	File file = new File(path);
	if (!file.exists()) {
	    return null;
	}

	int i, j;
	String separatorStr;
	if (File.separatorChar == '\\') {
	    // split needs escaped backslash for regex
	    separatorStr = "\\\\";
	}
	else {
	    separatorStr = "/";
	    caseSensitive = true;
	}
	String[] components = path.split(separatorStr);

	/*if (components.length > 1) {
	    selectLocalTreePath(file.getParent());
            }*/

	Vector vpath = new Vector();

	TreeModel tm = localTree.getModel();
	DefaultMutableTreeNode rootNode = (DefaultMutableTreeNode)tm.getRoot();
	vpath.add(rootNode);
	DefaultMutableTreeNode driveNode = null;

	if (rootNode.getChildCount() > 1) {
	    // Windows, with multiple drives
	    String drivename = components[0] + "\\";
	    for (i = 0; i < rootNode.getChildCount(); i++) {
		DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)rootNode.getChildAt(i);
		LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode)dmtn.getUserObject();
		if (drivename.equalsIgnoreCase(ldtn.toString())) {
		    driveNode = dmtn;
		}
	    }
	}
	else {
	    // just a single root
	    driveNode = (DefaultMutableTreeNode)rootNode.getChildAt(0);
	}

	// path is on a non-existent drive
	if (driveNode == null) return null;

	vpath.add(driveNode);

	DefaultMutableTreeNode dirNode = driveNode;
	DefaultMutableTreeNode nextNode;
	boolean foundFile = false;
	for (i = 1; i < components.length; i++) {

	    nextNode = null;
	    for (j = 0; j < dirNode.getChildCount(); j++) {
		DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)dirNode.getChildAt(j);
		Object uo = dmtn.getUserObject();
		if (uo instanceof LocalDirectoryTreeNode) {
		    LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) uo;
		    if (((ldtn.toString().equalsIgnoreCase(components[i])) && (!caseSensitive)) ||
			((ldtn.toString().equals(components[i])) && (caseSensitive))) {
			nextNode = dmtn;
			vpath.add(dmtn);
			if (!ldtn.isExpanded()) {
			    ldtn.expand();
			    localTreeBuilder.populateDirectoryChildren(dmtn);
			}
		    }
		}
		else {
		    // found a file, reached end of path
		    LocalFileTreeNode lftn = (LocalFileTreeNode) uo;
		    if (((lftn.toString().equalsIgnoreCase(components[i])) && (!caseSensitive)) ||
			((lftn.toString().equals(components[i])) && (caseSensitive))) {
			foundFile = true;
			vpath.add(dmtn);
		    }
		}
	    }
	    if (nextNode != null) {
		dirNode = nextNode;
	    }
	    else {
		break;
	    }
	}

	Object[] opath = new Object[vpath.size()];
	for (i = 0; i < vpath.size(); i++) {
	    opath[i] = vpath.elementAt(i);
	}
	TreePath tpath = new TreePath(opath);
	return tpath;
    }

    /*
     * Expands the specified file path in the local tree
     *
     * FIXME: I don't think this works
     */
    public void selectLocalTreePath(String path) {
	TreePath tpath = filenameToLocalTreePath(path);
	if (tpath != null) {
	    localTree.expandPath(tpath);
	    localTree.scrollPathToVisible(tpath);
	}
	localPathOpened = path;
    }

    /*
     * Expands the specified file path in the grid tree
     */
    public void selectGridTreePath(String path) {
	TreePath tpath = filenameToGridTreePath(path);
	if (tpath != null) {
	    gridTree.expandPath(tpath);
	    gridTree.scrollPathToVisible(tpath);
	}        
    }

    /*
     * Called just before a directory node in one of the trees is expanded.
     * This is used to lazily load the actual directory contents at this point.
     * It is also used to collapse any path that was previously open.
     */
    public void treeWillExpand(TreeExpansionEvent event) throws ExpandVetoException {
        boolean isGrid = false;

        Object src = event.getSource();
        if (src == localTree) {
            isGrid = false;
        }
        else if (src == gridTree) {
            isGrid = true;
        }
        else {
            System.out.println("Got expansion event from nowhere");
            return;
        }

        TreePath path = event.getPath();
        Object obj = path.getLastPathComponent();
        if (obj instanceof DefaultMutableTreeNode) {
            DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) obj;

	    // collapse all others
	    if (path.getPathCount() >= 3) {
		Object obj3 = path.getPathComponent(2);
		DefaultMutableTreeNode rootNode = (DefaultMutableTreeNode) path.getPathComponent(1);

		for (int i = 0; i < rootNode.getChildCount(); i++) {
		    DefaultMutableTreeNode child = (DefaultMutableTreeNode) rootNode.getChildAt(i);
		    
		    if (child != obj3) {
			Object[] path2 = new Object[3];
			path2[0] = path.getPathComponent(0);
			path2[1] = rootNode;
			path2[2] = child;
			TreePath path3 = new TreePath(path2);
			
                        if (!isGrid) {
                            localTree.collapsePath(path3);
                        }
                        else {
                            gridTree.collapsePath(path3);
                        }
		    }
		}
	    }

            Object obj2 = dmtn.getUserObject();

            if (obj2 instanceof DirectoryTreeNode) {
                DirectoryTreeNode dtn = (DirectoryTreeNode) obj2;
		if (!dtn.isExpanded()) {
		    dtn.expand();

                    if (!isGrid) {
			LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) obj2;
			localPathOpened = ldtn.getFile().getPath();
                        localTreeBuilder.populateDirectoryChildren(dmtn);
                    }
                    else {
                        GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) obj2;
                        gridPathOpened = gdtn.getDirectory().getPath();
                        try {
                            gridTreeBuilder.populateDirectoryChildren(dmtn, filterString);
                        }
                        catch (Exception ex) {
                            // error interacting with grid
                            return;
                        }
                    }
		}
            }

            // scroll to show the new path
            if (!isGrid) {
                Rectangle rect = localTree.getPathBounds(path);
                JScrollBar sb = localScrollPane.getVerticalScrollBar();
                if ((sb != null) && (rect != null)) {
                    sb.setValue(rect.y);
                }
            }
            else {
                Rectangle rect = gridTree.getPathBounds(path);
                JScrollBar sb = gridScrollPane.getVerticalScrollBar();
                if ((sb != null) && (rect != null)) {
                    sb.setValue(rect.y);
                }
            }
        }
    }

    public void treeWillCollapse(TreeExpansionEvent e) throws ExpandVetoException {
    }

    private boolean isValidSourceSelection(JTree tree) {
        TreePath[] sels = tree.getSelectionPaths();
        // source tree must have at least one path selected
        if ((sels == null) || (sels.length < 1)) {
            return false;
        }
        // not only the root node
        if ((sels.length == 1) && (sels[0].getPathCount() == 1)) {
            return false;
        }
        return true;
    }

    private boolean isValidDestinationSelection(JTree tree) {
        TreePath[] sels = tree.getSelectionPaths();
        // destination tree must have exactly one path selected
        if ((sels == null) || (sels.length < 1) || (sels.length > 1)) {
            return false;
        }
        // not the root node
        if (sels[0].getPathCount() == 1) {
            return false;
        }
        return true;
    }

    /*
     * Sets the upload and download button to enabled or disabled depending on whether the currently
     * selected tree nodes would allow upload/download
     */
    private void enableTransferButtons() {
        if ((isValidSourceSelection(localTree)) && (isValidDestinationSelection(gridTree))) {
            uploadButton.setEnabled(true);
        }
        else {
            uploadButton.setEnabled(false);
        }

        if ((isValidSourceSelection(gridTree)) && (isValidDestinationSelection(localTree))) {
            downloadButton.setEnabled(true);
        }
        else {
            downloadButton.setEnabled(false);
        }
    }

    /*
     * If either tree selection changes, may need to change enable state of the upload/download buttons
     */
    public void valueChanged(TreeSelectionEvent e) {
        enableTransferButtons();
    }

    public void startGridGet(String to) {
        String lfn, pfn;
        // See if more selected
        TreePath[] sels = gridTree.getSelectionPaths();

        Vector sourceNames = new Vector();
        Vector sources = new Vector();

        File file = new File(to);
        if (!file.isDirectory()) {
            int lastslash = to.lastIndexOf(File.separatorChar);
            to = to.substring(0, lastslash);
        }

        System.out.println("Getting file from grid to " + to);

        if (sels != null) {
            for (int i = 0; i < sels.length; i++) {
                DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)sels[i].getLastPathComponent();
                Object uo = dmtn.getUserObject();
                if (uo instanceof GridFileTreeNode) {
                    GridFileTreeNode gftn = (GridFileTreeNode) uo;
                    lfn = gftn.getFile().getPath();
                    pfn = to + File.separator + gftn.getFile().getName();
                    /*System.out.println("Getting file " + lfn);

                    try {
                        digsClient.getFile(lfn, pfn);
                        System.out.println("Got file");
                    }
                    catch (Exception e) {
                        System.out.println("Error getting file: " + e);
                        }*/
                    sourceNames.add(lfn);
                    sources.add(gftn.getFile());
                }
                else if (uo instanceof GridDirectoryTreeNode) {
                    GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;
                    lfn = gdtn.getDirectory().getPath();
                    pfn = to + File.separator + gdtn.getDirectory().getName() + File.separator;
                    /*System.out.println("Getting directory " + lfn);

                    try {
                        digsClient.getDirectory(lfn, pfn);
                        System.out.println("Got directory");
                    }
                    catch (Exception e) {
                        System.out.println("Error getting directory: " + e);
                        }*/
                    sourceNames.add(lfn);
                    sources.add(gdtn.getDirectory());
                }
            }
            FileTransferDialogue ftd = new FileTransferDialogue(this, false, sourceNames, to);
            if (ftd.getDestination() != null) {
                String dest = ftd.getDestination();
                String prefix = ftd.getPrefix();
                String postfix = ftd.getPostfix();
                // actually do the download
                //System.out.println("Actually downloading");
                transferTable.addGetTransfer(sources, dest, prefix, postfix);
            }            
        }
    }

    public void startGridPut(String to) {
        String lfn, pfn;

        Vector sourceNames = new Vector();
        Vector sources = new Vector();

        try {
            if (digsClient.fileExists(to)) {
                int lastSlash = to.lastIndexOf('/');
                if (lastSlash < 0) {
                    to = "";
                }
                else {
                    to = to.substring(0, lastSlash);
                }
            }
        }
        catch (Exception ex) {
            // on error just use the filename
        }

        System.out.println("Putting file to grid at " + to);

        TreePath[] sels = localTree.getSelectionPaths();
        if (sels != null) {
            for (int i = 0; i < sels.length; i++) {
                DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode)sels[i].getLastPathComponent();
                Object uo = dmtn.getUserObject();
                if (uo instanceof LocalFileTreeNode) {
                    LocalFileTreeNode lftn = (LocalFileTreeNode) uo;
                    if (to.equals("")) {
                        lfn = lftn.getFile().getName();
                    }
                    else {
                        lfn = to + "/" + lftn.getFile().getName();
                    }
                    pfn = lftn.getFile().getPath();

                    /*try {
                        digsClient.putFile(pfn, lfn);
                        System.out.println("Put file");
                    }
                    catch (Exception e) {
                        System.out.println("Error putting file: " + e);
                        }*/
                    sourceNames.add(pfn);
                    sources.add(lftn.getFile());
                }
                else if (uo instanceof LocalDirectoryTreeNode) {
                    LocalDirectoryTreeNode ldtn = (LocalDirectoryTreeNode) uo;
                    if (to.equals("")) {
                        lfn = ldtn.getFile().getName();
                    }
                    else {
                        lfn = to + "/" + ldtn.getFile().getName();
                    }
                    pfn = ldtn.getFile().getPath() + File.separator;

                    /*try {
                        digsClient.putDirectory(pfn, lfn);
                        System.out.println("Put directory");
                    }
                    catch (Exception e) {
                        System.out.println("Error putting directory: " + e);
                        }*/
                    sourceNames.add(pfn);
                    sources.add(ldtn.getFile());
                }
            }
            FileTransferDialogue ftd = new FileTransferDialogue(this, true, sourceNames, to);
            if (ftd.getDestination() != null) {
                String dest = ftd.getDestination();
                String prefix = ftd.getPrefix();
                String postfix = ftd.getPostfix();
                // actually do the upload
                //System.out.println("Actually uploading");
                transferTable.addPutTransfer(sources, dest, prefix, postfix);
            }
        }
    }

    /** Overridden so we can exit when window is closed */
    protected void processWindowEvent(WindowEvent e) {
        super.processWindowEvent(e);
        if (e.getID() == WindowEvent.WINDOW_CLOSING) {
            exit();
        }
    }

    protected void processComponentEvent(ComponentEvent e) {
        super.processComponentEvent(e);

        /*if (e.getID() == ComponentEvent.COMPONENT_RESIZED) {
            System.out.println("Window is resizing");

            int w = getWidth();
            int h = getHeight();
            w = (w * 2) / 5;
            localScrollPane.setPreferredSize(new Dimension(w, 400));
            gridScrollPane.setPreferredSize(new Dimension(h, 400));
            }*/
    }
}
