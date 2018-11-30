/*
 * Used in directory nodes before the real children have been added.
 */
package uk.ac.ed.epcc.digs.client.ui;

public class PlaceholderTreeNode extends FileTreeNode {

    public PlaceholderTreeNode() {
    }

    public String toString() {
        return "Working...";
    }
}
