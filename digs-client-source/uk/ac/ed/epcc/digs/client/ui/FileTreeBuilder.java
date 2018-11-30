/*
 * Abstract base class for building a tree of files. Specialised for grid and local file trees.
 */
package uk.ac.ed.epcc.digs.client.ui;

import javax.swing.*;
import javax.swing.tree.*;

public abstract class FileTreeBuilder {
    // actually builds the tree and returns the root node
    public abstract TreeNode buildTree() throws Exception;

    /*
     * Inserts the child node into the parent node, keeping the children in alphabetical order.
     * Only works if child node's user object implements toString, but I think it has to anyway.
     */
    public void orderedInsert(DefaultMutableTreeNode parent, DefaultMutableTreeNode child) {
        if (parent.getChildCount() == 0) {
            parent.add(child);
            return;
        }

        String childstr = child.getUserObject().toString();

        for (int i = 0; i < parent.getChildCount(); i++) {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) parent.getChildAt(i);
            String str = node.getUserObject().toString();
            if (childstr.compareTo(str) < 0) {
                parent.insert(child, i);
                return;
            }
        }
        parent.add(child);
    }

}
