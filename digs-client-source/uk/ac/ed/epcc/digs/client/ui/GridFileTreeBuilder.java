package uk.ac.ed.epcc.digs.client.ui;

import javax.swing.*;
import javax.swing.tree.*;

import java.util.Vector;

import uk.ac.ed.epcc.digs.client.DigsClient;
import uk.ac.ed.epcc.digs.client.LogicalFile;
import uk.ac.ed.epcc.digs.client.LogicalDirectory;

import uk.ac.ed.epcc.digs.DigsException;

public class GridFileTreeBuilder extends FileTreeBuilder {

    private DigsClient digsClient;

    public GridFileTreeBuilder(DigsClient dc) {
        digsClient = dc;
    }

    /*
     * If a filter string is given, only files and directories that match the filter will be populated.
     * (Note: directories match the filter if they contain any other file or directory that matches the
     * filter!)
     */
    public void populateDirectoryChildren(DefaultMutableTreeNode tree, String filter) throws Exception {
        int i;

        GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) tree.getUserObject();
        LogicalDirectory dir = gdtn.getDirectory();
        
        tree.removeAllChildren();

        Vector files = dir.getFiles();
        for (i = 0; i < files.size(); i++) {
            Object obj = files.elementAt(i);
            LogicalFile lf = (LogicalFile) obj;

            if (lf.isDirectory()) {
                LogicalDirectory ld = (LogicalDirectory) lf;
                //System.out.println("Found directory " + ld.getPath());

                if ((filter.equals("")) || (ld.matches(filter))) {
                    DefaultMutableTreeNode node2 = fakeTreeForDirectory(ld);
                    //tree.add(node2);
                    orderedInsert(tree, node2);
                }
            }
            else {
                //System.out.println("Found file " + lf.getPath());

                if ((filter.equals("")) || (lf.matches(filter))) {
                    DefaultMutableTreeNode node2 = buildTreeForFile(lf);
                    //tree.add(node2);
                    orderedInsert(tree, node2);
                }
            }
        }
    }

    // default version with no filter
    public void populateDirectoryChildren(DefaultMutableTreeNode tree) throws Exception {
        populateDirectoryChildren(tree, "");
    }

    public DefaultMutableTreeNode buildTreeForFile(LogicalFile file) {
        return new DefaultMutableTreeNode(new GridFileTreeNode(file));
    }

    /*
     * Used at first when directory node is added. Real contents are lazily added when node is
     * first expanded
     */
    public DefaultMutableTreeNode fakeTreeForDirectory(LogicalDirectory dir) {
        DefaultMutableTreeNode node = new DefaultMutableTreeNode(new GridDirectoryTreeNode(dir));
        node.add(new DefaultMutableTreeNode(new PlaceholderTreeNode()));
        return node;        
    }

    private DefaultMutableTreeNode buildTreeForDirectory(LogicalDirectory dir) throws Exception {
        DefaultMutableTreeNode node = new DefaultMutableTreeNode(new GridDirectoryTreeNode(dir));
        populateDirectoryChildren(node);
        return node;
    }

    // build the entire grid file tree
    public TreeNode buildTree() throws Exception {
        DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode(new RootTreeNode("DiGS"));
        
        LogicalDirectory rootDir = digsClient.getRootDirectory();
        DefaultMutableTreeNode node = buildTreeForDirectory(rootDir);
        rootNode.add(node);

        return rootNode;
    }

};
