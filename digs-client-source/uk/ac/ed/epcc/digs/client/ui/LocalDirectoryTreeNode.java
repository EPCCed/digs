package uk.ac.ed.epcc.digs.client.ui;

import java.io.File;

public class LocalDirectoryTreeNode extends DirectoryTreeNode {
    private boolean expanded;
    private File file;

    public LocalDirectoryTreeNode(File f) {
        file = f;
        expanded = false;
    }

    public String toString() {
	String str = file.getName();

	/* Windows drive files have empty name, use path instead in this case */
	if (str.equals("")) {
	    str = file.getPath();
	}
        return str;
    }

    public File getFile() {
        return file;
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

    public void refresh() {
	String path = file.getPath();
	file = new File(path);
    }
}
