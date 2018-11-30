/*
 * This class handles dragging and dropping between the local and grid file trees
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;

import java.io.File;

public class TreeTransferHandler extends TransferHandler {

    /* set to true if this is the transfer handler for the grid tree, false for local tree */
    private boolean isGrid;

    private Main main;

    public TreeTransferHandler(boolean grid, Main mn) {
	super();
	isGrid = grid;
        main = mn;
    }

    public int getSourceActions(JComponent c) {
	return COPY;
    }

    /*
     * Convert a tree path to a string (filename)
     */
    private String pathToString(TreePath path) {
	int i;
	String result = "";

	for (i = 1; i < path.getPathCount(); i++) {
	    
	    DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) path.getPathComponent(i);
	    Object ftn = dmtn.getUserObject();

	    result = result + ftn.toString();

	    if (i != (path.getPathCount() - 1)) {
		if (isGrid) {
                    if (!result.equals("")) {
                        result = result + "/";
                    }
		}
		else {
		    result = result + File.separator;
		}
	    }
	}

	return result;
    }

    /*
     * Create a string transferable. It contains the first filename selected.
     * If multiple files are selected, the destination tree will query this from
     * the source tree.
     */
    public Transferable createTransferable(JComponent c) {
	JTree tree = (JTree) c;
	TreePath path = tree.getSelectionPath();

	return new StringSelection(pathToString(path));
    }

    public void exportDone(JComponent c, Transferable t, int action) {
    }

    public boolean canImport(TransferSupport supp) {
	try {
	    Transferable tr = supp.getTransferable();
	    String str = (String) tr.getTransferData(DataFlavor.stringFlavor);
	    boolean gfn = true;

	    /*
	     * Determine whether a grid file or a local file. Local files will
	     * start with either "/" or "X:\"; grid files won't
	     */
	    if ((str.charAt(0) == '/') || ((str.charAt(1) == ':') && (str.charAt(2) == '\\'))) {
		gfn = false;
	    }

	    /*
	     * Only allow transfers from grid to local or vice versa. Don't allow moving files
	     * by dragging them within one tree
	     */
	    if ((isGrid) && (gfn)) {
		return false;
	    }
	    if ((!isGrid) && (!gfn)) {
		return false;
	    }
	    return true;
	}
	catch (Exception ex) {
	    return false;
	}
    }

    public boolean importData(TransferSupport supp) {
	if (!canImport(supp)) return false;
	try {
	    Transferable tr = supp.getTransferable();
	    String str = (String) tr.getTransferData(DataFlavor.stringFlavor);

	    /* get location where file was dropped, as a path string */
	    Point pt = supp.getDropLocation().getDropPoint();

	    JTree tree = (JTree) supp.getComponent();
	    TreePath path = tree.getPathForLocation(pt.x, pt.y);
	    if (path == null) {
		return false;
	    }
	    String pathstr = pathToString(path);

	    /*
	     * actually initiate transfer
	     */
	    if (isGrid) {
                main.startGridPut(pathstr);
		//System.out.println("Putting file " + str + " onto grid at " + pathstr);
	    }
	    else {
                main.startGridGet(pathstr);
		//System.out.println("Getting file " + str + " from grid to " + pathstr);
	    }
	    return true;
	}
	catch (Exception ex) {
	    return false;
	}
    }

};
