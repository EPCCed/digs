package uk.ac.ed.epcc.digs.client.ui;

import uk.ac.ed.epcc.digs.client.LogicalDirectory;

public class GridDirectoryTreeNode extends DirectoryTreeNode {

    private boolean expanded;
    private LogicalDirectory directory;

    public GridDirectoryTreeNode(LogicalDirectory dir) {
        directory = dir;
        expanded = false;
    }

    public LogicalDirectory getDirectory() {
        return directory;
    }

    public String toString() {
        return directory.getName();
    }

    public boolean isExpanded() {
        return expanded;
    }

    public void expand() {
        expanded = true;
    }

    public void collapse() {
        expanded = false;
    }
};
