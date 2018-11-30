package uk.ac.ed.epcc.digs.client.ui;

public abstract class DirectoryTreeNode {
    public abstract String toString();
    public abstract boolean isExpanded();
    public abstract void expand();
    public abstract void collapse();
}
