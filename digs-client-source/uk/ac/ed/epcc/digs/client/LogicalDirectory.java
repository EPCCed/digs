/*
 * Represents a logical directory stored in DiGS
 */
package uk.ac.ed.epcc.digs.client;

import java.util.Vector;
import java.util.Hashtable;
import java.util.Enumeration;

public class LogicalDirectory extends LogicalFile {

    private static Vector newFolders;

    LogicalDirectory(DigsClient dc, String p) {
        super(dc, p);
    }

    public static void addNewFolder(String name) {
        newFolders.add(name);
    }

    public static void removeNewFolder(String name) {
        Vector toremove = new Vector();
        int i;
        for (i = 0; i < newFolders.size(); i++) {
            String folder = (String) newFolders.elementAt(i);
            if ((folder.equals(name)) ||
                (folder.startsWith(name + "/"))) {
                toremove.add(folder);
            }
        }

        for (i = 0; i < toremove.size(); i++) {
            newFolders.remove(toremove.elementAt(i));
        }
    }

    /*
     * Gets a vector of LogicalFiles representing all files and subdirectories
     */
    public Vector getFiles() throws Exception {
	Vector v = new Vector();
	int pathlen;

	//Hashtable files = FileListCache.getInstance().getFileList();
	Hashtable files = FileListCache.getInstance().getSizeList();
	String ourpath;
	if (path.equals("")) {
	    ourpath = "";
	    pathlen = 0;
	}
	else {
	    ourpath = path + "/";
	    pathlen = path.length() + 1;
	}

	Hashtable dirs = new Hashtable();

	Enumeration en = files.keys();
	while (en.hasMoreElements()) {
	    String lfn = (String)en.nextElement();
	    if (lfn.startsWith(ourpath)) {

		String lfnend = lfn.substring(pathlen);
	    
		int slash = lfnend.indexOf('/');

		if (slash < 0) {
		    // simple file
		    LogicalFile lf = new LogicalFile(digsClient, lfn);
		    v.add(lf);
		}
		else {
		    // directory
		    String dirname = lfnend.substring(0, slash);
		    if (!dirs.containsKey(dirname)) {
			String dirpath;
			if (path.equals("")) {
			    dirpath = dirname;
			}
			else {
			    dirpath = path + "/" + dirname;
			}
			LogicalDirectory ld = new LogicalDirectory(digsClient, dirpath);
			v.add(ld);
			dirs.put(dirname, "");
		    }
		}
	    }
	}

        // add new folders in here
        for (int i = 0; i < newFolders.size(); i++) {
            String nf = (String) newFolders.elementAt(i);

            // see if it is within this directory
            if (nf.startsWith(ourpath)) {

                // get its path within this directory
                String lfnend = nf.substring(pathlen);

                // and find the next path component
                int slash = lfnend.indexOf('/');
                if (slash < 0) {
                    // it's just the one at this level
                    // handle already present dir
                    if (!dirs.containsKey(lfnend)) {
                        LogicalDirectory ld = new LogicalDirectory(digsClient, nf);
                        v.add(ld);
                        dirs.put(lfnend, "");
                    }
                }
                else {
                    String dirname = lfnend.substring(0, slash);
                    if (!dirs.containsKey(dirname)) {
                        String dirpath;
                        if (path.equals("")) {
                            dirpath = dirname;
                        }
                        else {
                            dirpath = path + "/" + dirname;
                        }
                        LogicalDirectory ld = new LogicalDirectory(digsClient, dirpath);
                        v.add(ld);
                        dirs.put(dirname, "");
                    }
                }
            }
        }

	return v;
    }

    public boolean isLocked() {
        // FIXME: implement - it's locked if all the files within are locked
        return false;
    }

    /*
     * Get size of whole directory by calling getSize for all contents and summing
     */
    public long getSize() {
        long total = 0;

        Vector files;
        try {
            files = getFiles();
        }
        catch (Exception ex) {
            return total;
        }

        for (int i = 0; i < files.size(); i++) {
            LogicalFile lf = (LogicalFile) files.elementAt(i);
            total += lf.getSize();
        }

        return total;
    }

    /*
     * Override attribute getters: directories don't have these
     */
    public String getLockedBy() {
        return null;
    }

    public String getOwner() {
        return null;
    }

    public String getChecksum() {
        return null;
    }

    public String getGroup() {
        return null;
    }

    public String getPermissions() {
        return null;
    }

    public int getReplicaCount() {
        return 0;
    }

    /*
     * Returns true if this directory matches the filter string. It matches
     * if it contains any file that matches, at any level in the hierarchy.
     */
    public boolean matches(String filter) {
        //Vector files;

        if ((!filter.startsWith("list:")) && (path.contains(filter))) {
            return true;
        }

	Hashtable files = FileListCache.getInstance().getSizeList();
	String ourpath;
	if (path.equals("")) {
	    ourpath = "";
	}
	else {
	    ourpath = path + "/";
	}
	Enumeration en = files.keys();
	while (en.hasMoreElements()) {
	    String lfn = (String)en.nextElement();
	    if ((lfn.startsWith(ourpath)) && (lfn.contains(filter))) {
		return true;
	    }
	}

        /*try {
            files = getFiles();
        }
        catch (Exception ex) {
            return false;
        }

        for (int i = 0; i < files.size(); i++) {
            LogicalFile lf = (LogicalFile) files.elementAt(i);
            if (lf.matches(filter)) {
                return true;
            }
	    }*/
        return false;
    }

    public boolean isDirectory() {
        return true;
    }

    static {
        newFolders = new Vector();
    }
};
