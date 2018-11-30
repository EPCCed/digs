package uk.ac.ed.epcc.digs.client.ui;

import java.io.File;

public class LocalFileTreeNode extends FileTreeNode {
    private File file;

    public LocalFileTreeNode(File f) {
        file = f;
    }

    public String toString() {
        return file.getName();
    }

    public File getFile() {
        return file;
    }
}
