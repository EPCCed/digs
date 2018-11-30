/*
 * Used for the inert node that exists at the root of both file trees
 */
package uk.ac.ed.epcc.digs.client.ui;

public class RootTreeNode extends FileTreeNode {

    private String name;

    public RootTreeNode(String nm) {
	name = nm;
    }

    public String toString() {
        return name;
    }
}
