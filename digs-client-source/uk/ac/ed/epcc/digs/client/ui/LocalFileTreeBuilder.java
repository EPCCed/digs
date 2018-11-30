package uk.ac.ed.epcc.digs.client.ui;

import javax.swing.*;
import javax.swing.tree.*;

import java.io.*;

public class LocalFileTreeBuilder extends FileTreeBuilder {

    public LocalFileTreeBuilder() {
        
    }

    public void populateDirectoryChildren(DefaultMutableTreeNode tree) {
        LocalDirectoryTreeNode dir = (LocalDirectoryTreeNode) tree.getUserObject();
	dir.refresh();
        File[] contents = dir.getFile().listFiles();

        tree.removeAllChildren();

	if (contents != null) {
	    for (int i = 0; i < contents.length; i++) {
		tree.add(buildTreeForFile(contents[i]));
	    }
	}
    }

    public DefaultMutableTreeNode buildTreeForDirectory(File dir) {
        DefaultMutableTreeNode dirNode = new DefaultMutableTreeNode(new LocalDirectoryTreeNode(dir));
        populateDirectoryChildren(dirNode);
        return dirNode;
    }

    /*
     * Directory nodes initially just contain a placeholder, the real contents are lazily added when
     * the node is first expanded
     */
    public DefaultMutableTreeNode fakeTreeForDirectory(File dir) {
        DefaultMutableTreeNode dirNode = new DefaultMutableTreeNode(new LocalDirectoryTreeNode(dir));
        dirNode.add(new DefaultMutableTreeNode(new PlaceholderTreeNode()));
        return dirNode;
    }

    public DefaultMutableTreeNode buildTreeForFile(File file) {
        if (file.isDirectory()) {
            return fakeTreeForDirectory(file);
        }
        else {
            return new DefaultMutableTreeNode(new LocalFileTreeNode(file));
        }
    }

    public TreeNode buildTree() throws Exception {
	DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode(new RootTreeNode("My Computer"));
	
	File[] roots = File.listRoots();
	for (int i = 0; i < roots.length; i++) {
	    DefaultMutableTreeNode dirNode = buildTreeForDirectory(roots[i]);
	    rootNode.add(dirNode);
	}

        return rootNode;
    }
}
