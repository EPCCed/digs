/*
 * Used in both trees to allow different tree nodes to have different icons
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;

import java.io.File;

public class TreeRenderer extends DefaultTreeCellRenderer {

    private ImageIcon fileIcon;
    private ImageIcon folderIcon;
    private ImageIcon lockedFileIcon;
    private ImageIcon lockedFolderIcon;

    public TreeRenderer(String imgpath) {
        // load all the icons
        fileIcon = new ImageIcon(imgpath + File.separator + "icon-file.png");
        folderIcon = new ImageIcon(imgpath + File.separator + "icon-folder.png");
        lockedFileIcon = new ImageIcon(imgpath + File.separator + "icon-locked-file.png");
        lockedFolderIcon = new ImageIcon(imgpath + File.separator + "icon-locked-folder.png");
    }

    public Component getTreeCellRendererComponent(
        JTree tree,
        Object value,
        boolean sel,
        boolean expanded,
        boolean leaf,
        int row,
        boolean hasFocus) {

        Icon icon = folderIcon;

        super.getTreeCellRendererComponent(
            tree, value, sel,
            expanded, leaf, row,
            hasFocus);

        DefaultMutableTreeNode dmtn = (DefaultMutableTreeNode) value;
        Object uo = dmtn.getUserObject();
        if (uo instanceof RootTreeNode) {
            // always display root node as folder
            icon = folderIcon;
        }
        else if (uo instanceof FileTreeNode) {
            icon = fileIcon;
            if (uo instanceof GridFileTreeNode) {
                GridFileTreeNode gftn = (GridFileTreeNode) uo;
                if (gftn.getFile().isLocked()) {
                    // special icon for locked files
                    icon = lockedFileIcon;
                }
            }
        }
        else if (uo instanceof DirectoryTreeNode) {
            if (uo instanceof GridDirectoryTreeNode) {
                GridDirectoryTreeNode gdtn = (GridDirectoryTreeNode) uo;
                if (gdtn.getDirectory().isLocked()) {
                    // special icon for locked folders
                    icon = lockedFolderIcon;
                }
            }
        }

        setIcon(icon);
        return this;
    }    
};
