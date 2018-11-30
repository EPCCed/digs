/*
 * Represents one logical file (or directory) on the grid
 */
package uk.ac.ed.epcc.digs.client;

import java.util.Hashtable;
import java.util.Vector;

public class LogicalFile {
    /*
     * Full logical filename, including all directory components
     */
    protected String path;

    /*
     * Only the last component of the path
     */
    protected String name;

    protected DigsClient digsClient;

    LogicalFile(DigsClient dc, String p) {
	digsClient = dc;
	path = p;
        int slash = p.lastIndexOf("/");
        // get last component of path for name
        if (slash < 0) {
            name = path;
        }
        else {
            name = path.substring(slash + 1);
        }
    }
    
    public String getPath() {
	return path;
    }

    public String getName() {
        return name;
    }

    // returns true if this logical file is locked by someone
    public boolean isLocked() {
	boolean locked = true;
	Hashtable locks = FileListCache.getInstance().getLockList();
        String lockedby = (String) locks.get(path);
        if ((lockedby == null) || (lockedby.equals("(null)")) || (lockedby.equals(""))) {
            locked = false;
        }
        return locked;
    }

    /*
     * Returns DN of user who locked file, or null if not locked
     */
    public String getLockedBy() {
        if (!isLocked()) {
            return null;
        }
        Hashtable locks = FileListCache.getInstance().getLockList();
        String lockedby = (String) locks.get(path);
        return lockedby;
    }

    /*
     * Getters for file attributes in RLS
     */
    public long getSize() {
        Hashtable sizes = FileListCache.getInstance().getSizeList();
        String sizestr = (String) sizes.get(path);
        long size;
        if (sizestr == null) {
            return -1;
        }
        try {
            size = Long.parseLong(sizestr);
        }
        catch (NumberFormatException nfe) {
            return -1;
        }
        return size;
    }

    public String getOwner() {
        String owner;
        try {
            owner = digsClient.getRLSAttribute(path, "submitter");
        }
        catch (Exception ex) {
            owner = null;
        }
        return owner;
    }

    public String getChecksum() {
        String checksum;
        try {
            checksum = digsClient.getRLSAttribute(path, "md5sum");
        }
        catch (Exception ex) {
            checksum = null;
        }
        return checksum;
    }

    public String getGroup() {
        String group;
        try {
            group = digsClient.getRLSAttribute(path, "group");
        }
        catch (Exception ex) {
            group = null;
        }
        return group;
    }

    public String getPermissions() {
        String permissions;
        try {
            permissions = digsClient.getRLSAttribute(path, "permissions");
        }
        catch (Exception ex) {
            permissions = null;
        }
        return permissions;
    }

    public int getReplicaCount() {
        int replcount;
        try {
            String rcs = digsClient.getRLSAttribute(path, "replcount");
            replcount = Integer.parseInt(rcs);
        }
        catch (Exception ex) {
            replcount = 0;
        }
        return replcount;
    }

    public Vector getLocations() {
        Vector v;
        try {
            v = digsClient.getFileLocations(path);
        }
        catch (Exception ex) {
            v = null;
        }
        return v;
    }

    /*
     * Returns true if file matches filter - it matches if it contains the filter
     * string anywhere in its path
     */
    public boolean matches(String filter) {
        if (filter.startsWith("list:")) {
            if ((filter.equals("list:"+path)) ||
                (filter.startsWith("list:"+path+",")) ||
                (filter.contains(","+path+",")) ||
                (filter.endsWith(","+path))) {
                return true;
            }
            return false;
        }

        if (path.contains(filter)) {
            return true;
        }
        return false;
    }

    /*
     * Directories are implemented by a subclass
     */
    public boolean isDirectory() {
        return false;
    }
};
