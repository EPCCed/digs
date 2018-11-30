package uk.ac.ed.epcc.digs.client.ui;

import uk.ac.ed.epcc.digs.client.LogicalFile;

public class GridFileTreeNode extends FileTreeNode {

    private LogicalFile file;

    public GridFileTreeNode(LogicalFile lf) {
        file = lf;
    }
    
    public LogicalFile getFile() {
        return file;
    }
    
    public String toString() {
        return file.getName();
    }
};
