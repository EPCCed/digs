/*
 * This class provides a client interface to a DiGS-powered grid.
 *
 * As well as methods for accessing grid functionality, there is a
 * simple command line client that provides most of the functionality
 * of the original C client.
 */
package uk.ac.ed.epcc.digs.client;

import java.util.Vector;
import java.util.Hashtable;
import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.util.Enumeration;
import java.security.MessageDigest;
import java.util.prefs.Preferences;
import java.util.Observable;
import java.util.Observer;

import uk.ac.ed.epcc.digs.client.rls.RlsClient;
import uk.ac.ed.epcc.digs.DigsException;

import org.globus.gsi.GlobusCredential;
import org.globus.gsi.GlobusCredentialException;

public class DigsClient extends Observable {

    /*
     * status report, sent to observers during startup
     */
    public class DigsClientStatus {
        public DigsClientStatus(String s, int p) {
            status = s;
            percentage = p;
        }
        public String status;
        public int percentage;
    };

    /* 
     * central node address and DiGS install path on there (these refer
     * to the backup node instead if that's what we're using)
     */
    private String mainnode;
    private String mainnodepath;

    /* whether we're using the backup node rather than the main node */
    private boolean usingBackupNode;

    /* the main config file */
    private DigsConfigFile config;

    /* RLS connection */
    private RlsClient rls;

    /* for sending messages to control thread */
    private DigsMessageSender messageSender;

    /* all storage nodes on grid, indexed by FQDN */
    private Hashtable storageNodes;

    /* node preference list */
    private Vector nodePreferenceList;

    /* user's local preferences */
    private Preferences prefs;

    /* group map file */
    private GroupMap groupmap;

    /* administrator DNs */
    private Vector administrators;

    /* version of the DiGS grid we're connected to */
    private int digsVersion;

    /*======================================================================
     *
     * Node information methods
     *
     *====================================================================*/
    public String getNodePath(String node) throws DigsException {
        StorageNode sn = (StorageNode)storageNodes.get(node);
        if (sn == null) {
            throw new DigsException("Node " + node + " not known");
        }
        return sn.getPath();
    }

    public String getNodeInbox(String node) throws DigsException {
        StorageNode sn = (StorageNode)storageNodes.get(node);
        if (sn == null) {
            throw new DigsException("Node " + node + " not known");
        }
        if (digsVersion < 3) {
            /*
             * old style fixed location inbox
             */
            return sn.getPath() + "/data/NEW";
        }
        return sn.getInbox();
    }

    public boolean isNodeDead(String node) throws DigsException {
        StorageNode sn = (StorageNode)storageNodes.get(node);
        if (sn == null) {
            throw new DigsException("Node " + node + " not known");
        }
        return sn.isDead();
    }

    public boolean isNodeDisabled(String node) throws DigsException {
        StorageNode sn = (StorageNode)storageNodes.get(node);
        if (sn == null) {
            throw new DigsException("Node " + node + " not known");
        }
        return sn.isDisabled();
    }

    public boolean isNodeRetiring(String node) throws DigsException {
        StorageNode sn = (StorageNode)storageNodes.get(node);
        if (sn == null) {
            throw new DigsException("Node " + node + " not known");
        }
        return sn.isRetiring();
    }

    public Vector getNodesByType(int type) {
        Vector result = new Vector();
        Enumeration enm = storageNodes.elements();
        while (enm.hasMoreElements()) {
            StorageNode sn = (StorageNode)enm.nextElement();
            if (sn.getType() == type) {
                result.add(sn.getName());
            }
        }
        return result;
    }

    /*======================================================================
     *
     * Utilities
     *
     *====================================================================*/
    public String getUserDN() throws GlobusCredentialException {
        GlobusCredential user = GlobusCredential.getDefaultCredential();
        return user.getIdentity();
    }

    /*
     * FIXME: this should really be stored in the user preferences instead
     * of an environment variable
     */
    public String getUserGroup() {
        String group = System.getenv("DIGS_GROUP");
        if (group == null)
        {
            group = "ukq";
            try {
                Vector v = groupmap.getUserGroups(getUserDN());
                group = (String)v.elementAt(0);
            }
            catch (Exception e) {
                /* default to ukq if error */
            }
        }
        return group;
    }

    public boolean isUserAdministrator() throws Exception {
        int i;
        String dn = getUserDN();
        for (i = 0; i < administrators.size(); i++) {
            String admin = (String)administrators.elementAt(i);
            if (admin.equals(dn)) {
                return true;
            }
        }
        return false;
    }

    /*
     * Returns MD5 sum of contents of a local file, as an upper case
     * hexadecimal string
     */
    public String checksumFile(String filename) throws Exception {
        MessageDigest md5 = MessageDigest.getInstance("MD5");
        File file = new File(filename);
        FileInputStream is = new FileInputStream(file);
        byte[] buffer = new byte[64 * 1024];

        int count;
        do {
            count = is.read(buffer);
            if (count > 0) {
                md5.update(buffer, 0, count);
            }
        } while (count > 0);

        byte[] digest = md5.digest();

        StringBuffer result = new StringBuffer("");
        for (int i = 0; i < digest.length; i++) {
            // there must be a simpler way to do this conversion but I can't
            // find it just now
            result.append(Integer.toHexString(((int)(digest[i] >> 4)) & 0xf));
            result.append(Integer.toHexString(((int)(digest[i] & 0xf)) & 0xf));
        }

        return result.toString().toUpperCase();
    }

    /*
     * Given a vector of node names, returns a sorted vector in which the
     * names are in the order in which they appear in the preference list.
     * Any names that don't appear in the preference list go at the end.
     *
     * WARNING: this deletes things from the original list! Don't pass a
     * list that you need to keep in here
     */
    public Vector sortByPreference(Vector list) {
        Vector result = new Vector();

        /* loop over whole node preference list */
        for (int i = 0; i < nodePreferenceList.size(); i++) {
            String pn = (String)nodePreferenceList.elementAt(i);

            /* see if this preferred node is present in our list */
            for (int j = 0; j < list.size(); j++) {
                String ln = (String)list.elementAt(j);
                if (pn.equals(ln)) {
                    result.add(ln);
                    list.set(j, null);
                    break;
                }
            }
        }

        /* handle any that weren't on the preference list at all */
        for (int k = 0; k < list.size(); k++) {
            String node = (String)list.elementAt(k);
            if (node != null) {
                result.add(node);
            }
        }

        return result;
    }

    /*
     * Returns a vector of names of nodes that have the given amount of
     * space free (in kilobytes)
     */
    public Vector getNodesWithSpace(long space) {
	Vector nodes = new Vector();
	Enumeration en = storageNodes.keys();
	while (en.hasMoreElements()) {
	    String node = (String)en.nextElement();
	    StorageNode sn = (StorageNode)storageNodes.get(node);
	    if (sn.getFreeSpace() > space) {
		nodes.add(node);
	    }
	}

        // return them in preference order
	return sortByPreference(nodes);
    }

    public String getRLSAttribute(String lfn, String attr) throws Exception {
        return rls.getAttribute(lfn, attr);
    }

    public Hashtable getAllAttributesValues(String attr) throws Exception {
	return rls.getAllAttributesValues(attr);
    }

    public Hashtable getFileList(String wc) throws Exception {
	Hashtable files = rls.getFileList(wc);
	return files;
    }

    public Vector getFileLocations(String lfn) throws Exception {
        return rls.getFileLocations(lfn);
    }

    public LogicalDirectory getRootDirectory() {
	return new LogicalDirectory(this, "");
    }

    /*
     * Returns the size (in bytes) of a logical directory on the grid
     */
    public long getLogicalDirectorySize(String ldn) throws Exception {
        long total = 0;
        Hashtable files = rls.getFileList("*");
        
        String prefix;
        if (ldn.endsWith("/")) {
            prefix = ldn;
        }
        else {
            prefix = ldn + "/";
        }

        // loop over all logical files
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String lfn = (String)en.nextElement();
            if (lfn.startsWith(prefix)) {
                String szstr = rls.getAttribute(lfn, "size");
                long size = Long.parseLong(szstr);
                total += size;
            }
        }

        return total;
    }

    /*======================================================================
     *
     * Startup
     *
     *====================================================================*/
    private void updateStatus(String str, int percentage) {
        setChanged();
        notifyObservers(new DigsClientStatus(str, percentage));
    }

    /*
     * Loads the grid configuration from the main node
     */
    private void loadConfig() throws Exception {
        int i;

        /* assume version 3 grid */
        digsVersion = 3;

        /* load main configuration */
        updateStatus("Downloading main configuration file", 0);
        byte[] cfgbuf = DigsFTPClient.getFile(mainnode, mainnodepath+"/qcdgrid.conf");
        config = new DigsConfigFile(cfgbuf);

        /* load master list of nodes */
        updateStatus("Downloading master list of nodes", 15);
        cfgbuf = DigsFTPClient.getFile(mainnode, mainnodepath+"/mainnodelist.conf");
        storageNodes = NodeListParser.parseMainNodeList(cfgbuf);

        /* process dead nodes */
        updateStatus("Downloading node lists", 30);
        cfgbuf = DigsFTPClient.getFile(mainnode, mainnodepath+"/deadnodes.conf");
        Vector list = NodeListParser.parseSimpleNodeList(cfgbuf);
        for (i = 0; i < list.size(); i++) {
            String nodename = (String)list.elementAt(i);
            StorageNode node = (StorageNode)storageNodes.get(nodename);
            if (node == null) {
                throw new DigsException("Unknown node " + nodename + " in deadnodes.conf");
            }
            node.setDead(true);
        }

        /* process disabled nodes */
        updateStatus("Downloading node lists", 45);
        cfgbuf = DigsFTPClient.getFile(mainnode, mainnodepath+"/disablednodes.conf");
        list = NodeListParser.parseSimpleNodeList(cfgbuf);
        for (i = 0; i < list.size(); i++) {
            String nodename = (String)list.elementAt(i);
            StorageNode node = (StorageNode)storageNodes.get(nodename);
            if (node == null) {
                throw new DigsException("Unknown node " + nodename + " in disablednodes.conf");
            }
            node.setDisabled(true);
        }

        /* process retiring nodes */
        updateStatus("Downloading node lists", 60);
        cfgbuf = DigsFTPClient.getFile(mainnode, mainnodepath+"/retiringnodes.conf");
        list = NodeListParser.parseSimpleNodeList(cfgbuf);
        for (i = 0; i < list.size(); i++) {
            String nodename = (String)list.elementAt(i);
            StorageNode node = (StorageNode)storageNodes.get(nodename);
            if (node == null) {
                throw new DigsException("Unknown node " + nodename + " in retiringnodes.conf");
            }
            node.setRetiring(true);
        }

        /* load group map file */
        updateStatus("Downloading group map file", 75);
        try {
            cfgbuf = DigsFTPClient.getFile(mainnode, mainnodepath+"/group-mapfile");
            groupmap = new GroupMap(cfgbuf);
        }
        catch (Exception e) {
            /*
             * If group map load fails, assume v2 and create an empty group map
             */
            digsVersion = 2;
            groupmap = new GroupMap();
        }

        /* load administrators list from config file */
        String admin = config.getFirstValue("administrator");
        while (admin != null) {
            administrators.add(admin);
            admin = config.getNextValue("administrator");
        }
    }

    /*
     * Attempts to start the grid (read config files and connect to RLS) from the
     * given node
     */
    private void startFrom(String node, String path) throws Exception {
        this.mainnode = node;
        this.mainnodepath = path;

        loadConfig();

        updateStatus("Connecting to replica location service", 90);
        rls = new RlsClient(mainnode, config.getIntValue("rc_port", 39281));
    }

    /*
     * Decodes a comma separated preference list into a vector of node names
     */
    private Vector decodePreferenceList(String preflist) {
        Vector result = new Vector();
        String[] nodes = preflist.split(",");
        for (int i = 0; i < nodes.length; i++) {
            result.add(nodes[i]);
        }
        return result;
    }

    /*
     * Encodes a vector of node names into a comma separated preference list
     */
    private String encodePreferenceList(Vector preflist) {
        String result = "";
        for (int i = 0; i < preflist.size(); i++) {
            result += (String) preflist.elementAt(i);
            if (i < (preflist.size() - 1)) {
                result += ",";
            }
        }
        return result;
    }

    /*
     * Initialises the grid. Reads main and backup node names from user preferences.
     * Tries to connect to main node, then to backup node if that fails
     */
    public void init() throws Exception {
        administrators = new Vector();

        prefs = Preferences.userNodeForPackage(this.getClass());
        
        // try to get main node info - exception if it's not there
        String node = prefs.get("mainnode", "");
        String path = prefs.get("mainnodepath", "");
        if ((node.equals("")) || (path.equals(""))) {
            throw new DigsException("Main node info not found in preferences");
        }

        usingBackupNode = false;

        // try to connect to main node
        try {
            startFrom(node, path);
            messageSender = new DigsMessageSender(node, config.getIntValue("qcdgrid_port"));
        }
        catch (Exception e) {
            // if that failed and we have a backup node, try to connect to backup node
            // (don't create message sender however)
            node = prefs.get("backupnode", "");
            path = prefs.get("backupnodepath", "");
            if ((node.equals("")) || (path.equals(""))) {
                throw new DigsException("Main node is down: " + e);
            }

            // if both fail, this will throw exception
            try {
                startFrom(node, path);
                usingBackupNode = true;
                messageSender = null;
            }
            catch (Exception e2) {
                throw new DigsException("Main node failed with: " + e + ", backup node failed with: " + e2);
            }
        }

        // try to get node prefs info
        String nodeprefs = prefs.get("nodeprefs", "");
        if (nodeprefs.equals("")) {
            // create default node prefs list and save it
            Vector v = new Vector();
            Enumeration en = storageNodes.keys();
            while (en.hasMoreElements()) {
                String str = (String) en.nextElement();
                v.add(str);
            }
            nodeprefs = encodePreferenceList(v);
            prefs.put("nodeprefs", nodeprefs);
        }
        
        // parse node prefs list
        nodePreferenceList = decodePreferenceList(nodeprefs);

        updateStatus("Connected!", 100);
    }

    /*
     * Updates the user preferences (main and backup node names and paths)
     * then connects to the grid. backupnode and backuppath may be null,
     * in which case they are ignored
     */
    public DigsClient(String mainnode, String mainpath, String backupnode, String backuppath) throws Exception {
        // update the preferences
        prefs = Preferences.userNodeForPackage(this.getClass());
        prefs.put("mainnode", mainnode);
        prefs.put("mainnodepath", mainpath);
        if (backupnode != null) {
            prefs.put("backupnode", backupnode);
            prefs.put("backuppath", backuppath);
        }

        // continue as normal
        init();
    }

    /*
     * Creates a DigsClient object connected to the grid specified in the
     * user preferences
     */
    public DigsClient() throws Exception {
        init();
        /*String mn = prefs.get("mainnode", "");
        System.out.println("Stored main node is: '" + mn + "'");
        String mp = prefs.get("mainnodepath", "");
        System.out.println("Stored main node path is: '" + mp + "'");

        prefs.put("mainnode", "ukqcdcontrol.epcc.ed.ac.uk");
        prefs.put("mainnodepath", "/home/qcdgrid/qcdgrid");*/

        /*String mn = prefs.get("backupnode", "");
        System.out.println("Stored backup node is: '" + mn + "'");
        String mp = prefs.get("backupnodepath", "");
        System.out.println("Stored backup node path is: '" + mp + "'");

        prefs.put("backupnode", "pytier2.swan.ac.uk");
        prefs.put("backupnodepath", "/home/qcdgrid/qcdgrid");*/
    }

    public DigsClient(Observer obs) throws Exception {
        addObserver(obs);
        init();
    }

    public boolean fileExists(String lfn) {
        try {
            if (rls.lfnExists(lfn)) {
                return true;
            }
            return false;
        }
        catch (Exception ex) {
            return false;
        }
    }


    /*======================================================================
     *
     * Simple message sending operations
     *
     *====================================================================*/
    public boolean ping() {
        boolean result = true;

        if (usingBackupNode) return false;

        try {
            String response = messageSender.sendMessage("ping");
            if (!response.startsWith("OK")) {
                result = false;
            }
        }
        catch (Exception e) {
            result = false;
        }

        return result;
    }

    public void retireNode(String node) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        if (isNodeRetiring(node)) {
            throw new DigsException("Node " + node + " is already retiring");
        }
        String response = messageSender.sendMessage("retire " + node);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void unretireNode(String node) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        if (!isNodeRetiring(node)) {
            throw new DigsException("Node " + node + " is not retiring");
        }
        String response = messageSender.sendMessage("unretire " + node);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void disableNode(String node) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        if (isNodeDisabled(node)) {
            throw new DigsException("Node " + node + " is already disabled");
        }
        String response = messageSender.sendMessage("disable " + node);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void addNode(String node, String site, String path) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        if (storageNodes.containsKey(node)) {
            throw new DigsException("Node " + node + " is already on grid");
        }
        String response = messageSender.sendMessage("add " + node + " " + site + " " + path);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void enableNode(String node) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        if (!isNodeDisabled(node)) {
            throw new DigsException("Node " + node + " is already enabled");
        }
        String response = messageSender.sendMessage("enable " + node);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void deleteFile(String lfn) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        lfn = lfn.replaceAll("//", "/");
        if (!rls.lfnExists(lfn)) {
            throw new DigsException("File " + lfn + " does not exist on grid");
        }
        String response = messageSender.sendMessage("delete " + lfn);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void deleteDirectory(String lfn) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        lfn = lfn.replaceAll("//", "/");
        if (rls.lfnExists(lfn)) {
            throw new DigsException(lfn + " is a regular file");
        }
        String response = messageSender.sendMessage("rmdir " + lfn);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    /*
     * FIXME: accesses the console directly. Should be changed to allow e.g. a GUI
     * to use this method
     */
    public void removeNode(String node) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        /* first look for files stored only on this node */
        int atRisk = 0;
        Hashtable files = rls.getFileList("*");
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String lfn = (String)en.nextElement();
            Vector pfns = (Vector)files.get(lfn);
            if (pfns.size() == 1) {
                // only one location
                String pfn = (String)pfns.elementAt(0);
                if (pfn.equals(node)) {
                    atRisk++;
                    if (atRisk <= 10) {
                        System.out.println("File " + lfn + " is at risk");
                    }
                    else if (atRisk == 11) {
                        System.out.println("...more files at risk...");
                    }
                }
            }
        }

        if (atRisk == 0) {
            System.out.println("All files are duplicated, removing node...");
        }
        else {
            // get user to confirm
            System.out.println("\n" + atRisk + " files are only present on " + node);
            System.out.println("Are you sure you want to remove this node? (Y/N)");
            char c = (char) System.in.read();
            if ((c != 'y') && (c != 'Y')) {
                System.out.println("Remove operation cancelled by user");
                return;
            }
        }

        String response = messageSender.sendMessage("remove " + node);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    /*======================================================================
     *
     * Permission changing
     *
     *====================================================================*/
    public void setPermissions(String lfn, boolean isPublic, boolean recursive)
        throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        String group = getUserGroup();

        lfn = lfn.replaceAll("//", "/");
        if (!rls.lfnExists(lfn)) {
            throw new DigsException("File " + lfn + " does not exist on grid");
        }

        String msg;
        if (isPublic) {
            msg = "chmod " + recursive + " " + group + " " + lfn + " public";
        }
        else {
            msg = "chmod " + recursive + " " + group + " " + lfn + " private";
        }
        String response = messageSender.sendMessage(msg);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    /*======================================================================
     *
     * Putting
     *
     *====================================================================*/
    public void putFile(String pfn, String lfn) throws Exception {
        putFile(pfn, lfn, null);
    }

    public void putFile(String pfn, String lfn, TransferMonitor trm) throws Exception {

        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        lfn = lfn.replaceAll("//", "/");
	// check if file already exists
	if (rls.lfnExists(lfn)) {
	    throw new DigsException("File " + lfn + " already exists on grid");
	}

	File file = new File(pfn);
	if (!file.exists()) {
	    throw new DigsException("Local file " + pfn + " not found");
	}

	// get size of local file
	long length = file.length();

	// get MD5 sum of file
	String md5sum = checksumFile(pfn);

	// get user DN and info
	String dn = getUserDN();
	String group = getUserGroup();
	String permissions = "private";

	// work out flattened filename
	String flatLfn = lfn.replaceAll("/", "-DIR-");

	// get possible nodes to put file on
	Vector nodes = getNodesWithSpace((length / 1024) + 1);

	// try copying to each node in turn
	Enumeration en = nodes.elements();
	boolean succeeded = false;
	String node = "";
	while ((en.hasMoreElements()) && (!succeeded)) {
	    node = (String)en.nextElement();

	    if ((!isNodeDead(node)) && (!isNodeDisabled(node)) &&
		(!isNodeRetiring(node))) {

		try {
		    // try to copy file here
		    String finalName = getNodeInbox(node) + "/"+ flatLfn;

		    StorageNode sn = (StorageNode)storageNodes.get(node);
		    if (sn == null) {
			throw new DigsException("Node " + node + " not known");
		    }

		    StorageElementInterface se = sn.getSE();
		    se.putFile(node, pfn, finalName, trm);

		    // break out if successful
		    succeeded = true;
		}
		catch (Exception e) {
		    // ignore FTP exceptions, we'll try the next node if it failed
                    System.out.println("Exception: " + e);
		}
	    }
	}
	if (!succeeded) {
	    // error if reached end and no success
	    throw new DigsException("No storage node available for " + lfn);
	}

	String safeDN = dn.replace(' ', '+');

        // get time in seconds
        long tm = System.currentTimeMillis() / 1000;

	// send putFile message to main node
	String msg = "putFile " + lfn + " " + group + " " + permissions +
	    " " + length + " " + md5sum + " " + tm + " " + safeDN;

        String response = messageSender.sendMessage(msg);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
	}

	// send check message to main node
	response = messageSender.sendMessage("check " + node);
	if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
	}
    }

    public void modifyFile(String pfn, String lfn, TransferMonitor trm) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        lfn = lfn.replaceAll("//", "/");
        
	File file = new File(pfn);
	if (!file.exists()) {
	    throw new DigsException("Local file " + pfn + " not found");
	}

	// get size of local file
	long length = file.length();

	// get MD5 sum of file
	String md5sum = checksumFile(pfn);

	// work out flattened filename
	String flatLfn = lfn.replaceAll("/", "-DIR-");

	// get possible nodes to put file on
	Vector nodes = getNodesWithSpace((length / 1024) + 1);

	// try copying to each node in turn
	Enumeration en = nodes.elements();
	boolean succeeded = false;
	String node = "";
	while ((en.hasMoreElements()) && (!succeeded)) {
	    node = (String)en.nextElement();

	    if ((!isNodeDead(node)) && (!isNodeDisabled(node)) &&
		(!isNodeRetiring(node))) {

		try {
		    // try to copy file here
		    String finalName = getNodeInbox(node) + "/"+ flatLfn;

		    StorageNode sn = (StorageNode)storageNodes.get(node);
		    if (sn == null) {
			throw new DigsException("Node " + node + " not known");
		    }

		    StorageElementInterface se = sn.getSE();
		    se.putFile(node, pfn, finalName, trm);

		    // break out if successful
		    succeeded = true;
		}
		catch (Exception e) {
		    // ignore FTP exceptions, we'll try the next node if it failed
                    System.out.println("Exception: " + e);
		}
	    }
	}
	if (!succeeded) {
	    // error if reached end and no success
	    throw new DigsException("No storage node available for " + lfn);
	}

        // get time in seconds
        long tm = System.currentTimeMillis() / 1000;

	// send modify message to main node
	String msg = "modify " + lfn + " " + node + " " + md5sum + " " + length +
            " " + tm;
        String response = messageSender.sendMessage(msg);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
	}

	// send check message to main node
	response = messageSender.sendMessage("check " + node);
	if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
	}
    }

    public void putDirectory(String pdn, String ldn) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

	File dir = new File(pdn);
	if (!dir.isDirectory()) {
	    throw new DigsException(pdn + " is not a directory");
	}
	String[] contents = dir.list();
	for (int i = 0; i < contents.length; i++) {
	    String pfn = pdn + File.separatorChar + contents[i];
	    String lfn = ldn + "/" + contents[i];
	    File file = new File(pfn);
	    if (file.isDirectory()) {
		if ((!contents[i].equals(".")) && (!contents[i].equals(".."))) {
		    putDirectory(pfn, lfn);
		}
	    }
	    else {
		System.out.println(pfn + " => " + lfn);
		putFile(pfn, lfn);

		// FIXME: should really update free disk space like C code does
	    }
	}
    }

    /*======================================================================
     *
     * Getting
     *
     *====================================================================*/
    public void getFile(String lfn, String pfn) throws IOException, DigsException {
        getFile(lfn, pfn, null);
    }

    public void getFile(String lfn, String pfn, TransferMonitor tm) throws IOException, DigsException {
        // substitute single slash for double
        lfn = lfn.replaceAll("//", "/");
        Vector locs = rls.getFileLocations(lfn);

        // get in preference order
        locs = sortByPreference(locs);

        for (int i = 0; i < locs.size(); i++) {
            String loc = (String)locs.elementAt(i);
	    System.out.println("Trying location " + loc);
            if ((!isNodeDead(loc)) && (!isNodeDisabled(loc))) {
                try {
                    String dir = rls.getFileDisk(lfn, loc);
                    String path = getNodePath(loc);

		    StorageNode sn = (StorageNode)storageNodes.get(loc);
		    if (sn == null) {
			throw new DigsException("Node " + loc + " not known");
		    }

		    StorageElementInterface se = sn.getSE();
                    se.getFile(loc, path + "/" + dir + "/" + lfn, pfn, tm);
                    // success!
                    return;
                }
                catch (Exception e) {
                    System.err.println("Error retrieving file from " + loc + ": " + e);
                }
            }
        }
        throw new DigsException("All copies of file inaccessible");
    }

    public void getDirectory(String ldn, String pdn) throws IOException, DigsException {
        Hashtable files = rls.getFileList("*");
        // substitute single slash for double
        ldn = ldn.replaceAll("//", "/");

        String prefix;
        if (ldn.endsWith("/")) {
            prefix = ldn;
        }
        else {
            prefix = ldn + "/";
        }

        // loop over all logical files
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String lfn = (String)en.nextElement();
            if (lfn.startsWith(prefix)) {
                // found one in this directory
                // work out local filename
                String pfn = pdn + lfn.substring(prefix.length());
                pfn = pfn.replace('/', File.separatorChar);

                System.out.println(lfn + " -> " + pfn);

                // make sure local directory exists
                File file = new File(pfn);
                File dir = file.getParentFile();
                if (!dir.mkdirs()) {
                    throw new DigsException("Unable to create directory structure for local copy");
                }

                getFile(lfn, pfn);
            }
        }
    }

    /*======================================================================
     *
     * Touching
     *
     *====================================================================*/
    public void touchFile(String lfn, String host) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        lfn = lfn.replaceAll("//", "/");
        if (!rls.lfnExists(lfn)) {
            throw new DigsException("File " + lfn + " does not exist on grid");
        }

        // automatic host determination using preference list if none specified
        if (host == null) {
            String szstr = rls.getAttribute(lfn, "size");
            long size = Long.parseLong(szstr);
            Vector nodes = getNodesWithSpace((size / 1024) + 1);
            if (nodes.size() == 0) {
                throw new DigsException("No node has space for another copy of " + lfn);
            }
            host = (String)nodes.elementAt(0);
        }
        else {
            if (!storageNodes.containsKey(host)) {
                throw new DigsException("Storage node " + host + " does not exist on grid");
            }
        }

        String response = messageSender.sendMessage("touch " + lfn + " " + host);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void touchDirectory(String ldn, String host) throws Exception {
        if (usingBackupNode) {
            throw new DigsException("Operation not available with backup node");
        }

        ldn = ldn.replaceAll("//", "/");
        if (rls.lfnExists(ldn)) {
            throw new DigsException(ldn + " is a regular file");
        }

        // automatic host determination using preference list if none specified
        if (host == null) {
            long size = getLogicalDirectorySize(ldn);
            Vector nodes = getNodesWithSpace((size / 1024) + 1);
            if (nodes.size() == 0) {
                throw new DigsException("No node has space for another copy of " + ldn);
            }
            host = (String)nodes.elementAt(0);
        }
        else {
            if (!storageNodes.containsKey(host)) {
                throw new DigsException("Storage node " + host + " does not exist on grid");
            }
        }

        String response = messageSender.sendMessage("touchdir " + ldn + "/ " + host);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }        
    }

    /*======================================================================
     *
     * Locking/unlocking/checking locks
     *
     *====================================================================*/
    private boolean attrIsNull(String attr) {
        if ((attr == null) || (attr.equals("(null)")) || (attr.equals(""))) {
            return true;
        }
        return false;
    }

    public void lockFile(String lfn) throws Exception {
        String lockedby = rls.getAttribute(lfn, "lockedby");
        if ((attrIsNull(lockedby)) || (isUserAdministrator())) {
            String response = messageSender.sendMessage("lock " + lfn);
            if (!response.startsWith("OK")) {
                throw new DigsException("Control thread responded: " + response);
            }
        }
        else {
            throw new DigsException("File " + lfn + " is already locked by " + lockedby);
        }
    }

    public void unlockFile(String lfn) throws Exception {
        String lockedby = rls.getAttribute(lfn, "lockedby");
        if (attrIsNull(lockedby)) {
            throw new DigsException("File " + lfn + " is not locked");
        }
        if ((!lockedby.equals(getUserDN())) && (!isUserAdministrator())) {
            throw new DigsException("File " + lfn + " is locked by " + lockedby + ", cannot unlock");
        }
        String response = messageSender.sendMessage("unlock " + lfn);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void lockDirectory(String lfn) throws Exception {
        String ldn;
        if (lfn.endsWith("/")) {
            ldn = lfn;
        }
        else {
            ldn = lfn + "/";
        }

        String dn = getUserDN();

        Hashtable files = rls.getFileList("*");

        Hashtable lockedby = rls.getAllAttributesValues("lockedby");

        /* loop over all files in directory, seeing if we can lock all of them */
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String file = (String)en.nextElement();
            if (file.startsWith(ldn)) {
                /* found a file in our directory */
                String lb = (String)lockedby.get(file);
                if ((!attrIsNull(lb)) && (!lb.equals(dn)) &&
                    (!isUserAdministrator())) {
                    throw new DigsException("Cannot lock directory, " + file + " already locked by " + lb);
                }
            }
        }

        /* send message to do lock */
        String response = messageSender.sendMessage("lockdir " + lfn);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void unlockDirectory(String lfn) throws Exception {
        String response = messageSender.sendMessage("unlockdir " + lfn);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void checkLock(String lfn) throws Exception {
        String lockedby = rls.getAttribute(lfn, "lockedby");
        if (attrIsNull(lockedby)) {
            System.out.println("File " + lfn + " is not locked");
        }
        else {
            System.out.println("File " + lfn + " is locked by " + lockedby);
        }
    }

    public void checkDirectoryLock(String lfn) throws Exception {
        String ldn;
        if (lfn.endsWith("/")) {
            ldn = lfn;
        }
        else {
            ldn = lfn + "/";
        }

        Hashtable files = rls.getFileList("*");

        String alllockedby = null;
        boolean allunlocked = true;
        boolean firsttime = true;

        Hashtable lockedby = rls.getAllAttributesValues("lockedby");

        /* loop over all files in directory, seeing if we can report in simple form */
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String file = (String)en.nextElement();
            if (file.startsWith(ldn)) {
                /* found a file in our directory */
                String lb = (String)lockedby.get(file);
                if (attrIsNull(lb)) {
                    if (firsttime) {
                        alllockedby = lb;
                        allunlocked = false;
                    }
                    else {
                        if ((alllockedby == null) || (!alllockedby.equals(lb))) {
                            alllockedby = null;
                            allunlocked = false;
                        }
                    }
                }
                else {
                    if (!allunlocked) {
                        alllockedby = null;
                    }
                }
                firsttime = false;
            }
        }

        if (alllockedby != null) {
            // all locked by same person
            System.out.println("Directory " + lfn + " is locked by " + alllockedby);
        }
        else if (allunlocked) {
            // all unlocked
            System.out.println("Directory " + lfn + " is unlocked");
        }
        else {
            // need to print them all individually
            en = files.keys();
            while (en.hasMoreElements()) {
                String file = (String)en.nextElement();
                if (file.startsWith(ldn)) {
                    /* found a file in our directory */
                    String lb = (String)lockedby.get(file);
                    if (attrIsNull(lb)) {
                        System.out.println("File " + file + " is locked by " + lb);
                    }
                }
            }
        }
    }

    /*======================================================================
     *
     * Replica counts
     *
     *====================================================================*/
    public void setReplicaCount(String lfn, int count) throws Exception {
        String response = messageSender.sendMessage("replcount " + lfn + " " + count);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void setReplicaCountDirectory(String lfn, int count) throws Exception {
        String response = messageSender.sendMessage("replcountdir " + lfn + " " + count);
        if (!response.startsWith("OK")) {
            throw new DigsException("Control thread responded: " + response);
        }
    }

    public void checkReplicaCount(String lfn) throws Exception {
        String rc = rls.getAttribute(lfn, "replcount");
        if (attrIsNull(rc)) {
            int deflt = config.getIntValue("min_copies", 2);
            System.out.println("Replication count for " + lfn + " defaults to " + deflt);
        }
        else {
            System.out.println("Replication count for " + lfn + " is " + rc);
        }
    }

    public void checkReplicaCountDirectory(String lfn) throws Exception {
        String ldn;
        if (lfn.endsWith("/")) {
            ldn = lfn;
        }
        else {
            ldn = lfn + "/";
        }

        Hashtable files = rls.getFileList("*");
        Hashtable rcs = rls.getAllAttributesValues("replcount");
        int rc = -1;
        boolean simple = true;

        /* loop over all files in directory, seeing if we can report in simple form */
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String file = (String)en.nextElement();
            if (file.startsWith(ldn)) {
                /* found a file in our directory */
                String rcstr = (String)rcs.get(file);
                if (attrIsNull(rcstr)) {
                    // default RC for this file
                    if (rc <= 0) rc = 0;
                    else {
                        simple = false;
                        break;
                    }
                }
                else {
                    // specific RC for this file
                    if (rc < 0) {
                        rc = Integer.parseInt(rcstr);
                    }
                    else if (rc != Integer.parseInt(rcstr)) {
                        simple = false;
                        break;
                    }
                }
            }
        }        

        if (simple) {
            /* all files in directory are the same, print as one */
            if (rc == 0) {
                System.out.println("Directory " + lfn + " has default replication count (" +
                                   config.getIntValue("min_copies", 2) + ")");
            }
            else {
                System.out.println("Directory " + lfn + " replication count is " + rc);
            }
        }
        else {
            /* need to do complex reporting. Loop over directory again */
            en = files.keys();
            while (en.hasMoreElements()) {
                String file = (String)en.nextElement();
                if (file.startsWith(ldn)) {
                    /* found a file in our directory */
                    String rcstr = (String)rcs.get(file);
                    if (attrIsNull(rcstr)) {
                        System.out.println("Replication count for " + file + " is " + rcstr);
                    }
                }
            }
        }
    }

    /*======================================================================
     *
     * Listing
     *
     *====================================================================*/
    /*
     * FIXME: the list methods print directly to the console. Should
     * generalise them to be useful for e.g. a GUI
     */

    /* flags to tell list method what to display  */
    public static final int LIST_SHOW_NUM_COPIES  = 0x00000001;
    public static final int LIST_SHOW_LOCATIONS   = 0x00000002;
    public static final int LIST_SHOW_GROUP       = 0x00000004;
    public static final int LIST_SHOW_PERMISSIONS = 0x00000008;
    public static final int LIST_SHOW_CHECKSUM    = 0x00000010;
    public static final int LIST_SHOW_SIZE        = 0x00000020;
    public static final int LIST_SHOW_SUBMITTER   = 0x00000040;

    public static final int LIST_SHOW_LONG        = 0x0000007f;


    public void list(String wc, int flags) throws IOException {

        // list the relevant files
        Hashtable files = rls.getFileList(wc);

        Hashtable groups = null;
        Hashtable permissions = null;
        Hashtable checksums = null;
        Hashtable sizes = null;
        Hashtable submitters = null;

        // get all the attributes we need with bulk queries
        if ((flags & LIST_SHOW_GROUP) != 0) {
            groups = rls.getAllAttributesValues("group");
        }

        if ((flags & LIST_SHOW_PERMISSIONS) != 0) {
            permissions = rls.getAllAttributesValues("permissions");
        }

        if ((flags & LIST_SHOW_CHECKSUM) != 0) {
            checksums = rls.getAllAttributesValues("md5sum");
        }

        if ((flags & LIST_SHOW_SIZE) != 0) {
            sizes = rls.getAllAttributesValues("size");
        }

        if ((flags & LIST_SHOW_SUBMITTER) != 0) {
            submitters = rls.getAllAttributesValues("submitter");
        }

        // iterate over all the lfns
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            StringBuilder line = new StringBuilder();

            String lfn = (String)en.nextElement();
            
            line.append(lfn);

            if (groups != null) {
                line.append(" ");
                String grp = (String)groups.get(lfn);
                if (grp == null) {
                    line.append("<null group>");
                }
                else {
                    line.append(grp);
                }
            }

            if (permissions != null) {
                line.append(" ");
                String perm = (String)permissions.get(lfn);
                if (perm == null) {
                    line.append("<null permissions>");
                }
                else {
                    line.append(perm);
                }
            }

            if (submitters != null) {
                line.append(" ");
                String sub = (String)submitters.get(lfn);
                if (sub == null) {
                    line.append("<null submitter>");
                }
                else {
                    line.append(sub);
                }
            }

            if (sizes != null) {
                line.append(" ");
                String sz = (String)sizes.get(lfn);
                if (sz == null) {
                    line.append("<null size>");
                }
                else {
                    line.append(sz);
                }
            }

            if (checksums != null) {
                line.append(" ");
                String cksum = (String)checksums.get(lfn);
                if (cksum == null) {
                    line.append("<null checksum>");
                }
                else {
                    line.append(cksum);
                }
            }

            if ((flags & LIST_SHOW_NUM_COPIES) != 0) {
                Vector pfns = (Vector)files.get(lfn);
                line.append(" (");
                line.append(pfns.size());
                line.append(")");
            }

            if ((flags & LIST_SHOW_LOCATIONS) != 0) {
                Vector pfns = (Vector)files.get(lfn);
                line.append(" [");
                for (int i = 0; i < pfns.size(); i++) {
                    String loc = (String)pfns.elementAt(i);
                    line.append(loc);
                    if (i < (pfns.size() - 1)) {
                        line.append(" ");
                    }
                }
                line.append("]");
            }

            System.out.println(line.toString());
        }
    }

    public void listByNode(String node) throws IOException {
        Vector files = rls.listLocationFiles(node);
        for (int i = 0; i < files.size(); i++) {
            System.out.println((String)files.elementAt(i));
        }
    }

    public void listByGroup(String group) throws IOException {
        Hashtable files = rls.getFileList("*");
        Hashtable groups = rls.getAllAttributesValues("group");

        // iterate over all the lfns
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String lfn = (String)en.nextElement();
            String filegrp = (String)groups.get(lfn);
            if (filegrp != null) {
                if (filegrp.equals(group)) {
                    System.out.println(lfn);
                }
            }
        }
    }

    public void listBySubmitter(String submitter) throws IOException {
        Hashtable files = rls.getFileList("*");
        Hashtable submitters = rls.getAllAttributesValues("submitter");

        // iterate over all the lfns
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String lfn = (String)en.nextElement();
            String filesub = (String)submitters.get(lfn);
            if (filesub != null) {
                if (filesub.equals(submitter)) {
                    System.out.println(lfn);
                }
            }
        }
    }

    public void listByPermissions(String group, String perms) throws IOException {
        Hashtable files = rls.getFileList("*");
        Hashtable groups = rls.getAllAttributesValues("group");
        Hashtable permissions = rls.getAllAttributesValues("permissions");

        // iterate over all the lfns
        Enumeration en = files.keys();
        while (en.hasMoreElements()) {
            String lfn = (String)en.nextElement();
            String filegrp = (String)groups.get(lfn);
            String fileperm = (String)permissions.get(lfn);
            if ((filegrp != null) && (fileperm != null)) {
                if ((filegrp.equals(group)) && (fileperm.equals(perms))) {
                    System.out.println(lfn);
                }
            }
        }
    }

    /*======================================================================
     *
     * Main command line client
     *
     *====================================================================*/
    private static void usage(String[] args) {
        if (args.length == 0) {
            System.err.println("Usage: java DigsClient <command> [arguments]");
        }
        else if (args[0].equals("setnodes")) {
            System.err.println("setnodes command arguments: <main node name> <main node path> [<backup node name> <backup node path>]");
        }
        else if (args[0].equals("ping")) {
            System.err.println("ping command takes no arguments");
        }
        else if (args[0].equals("lock")) {
            System.err.println("lock command arguments: [-R | --recursive] [-c] <grid filename>");
        }
        else if (args[0].equals("unlock")) {
            System.err.println("unlock command arguments: [-R | --recursive] <grid filename>");
        }
        else if (args[0].equals("replcount")) {
            System.err.println("replcount command arguments: [-R | --recursive] [-c] [-d] <grid filename> [<count>]");
        }
        else if (args[0].equals("add")) {
            System.err.println("add command arguments: <node name> <site name> <path to DiGS on node>");
        }
        else if (args[0].equals("remove")) {
            System.err.println("remove command arguments: <node name>");
        }
        else if (args[0].equals("disable")) {
            System.err.println("disable command arguments: <node name>");
        }
        else if (args[0].equals("enable")) {
            System.err.println("enable command arguments: <node name>");
        }
        else if (args[0].equals("retire")) {
            System.err.println("retire command arguments: <node name>");
        }
        else if (args[0].equals("unretire")) {
            System.err.println("unretire command arguments: <node name>");
        }
        else if (args[0].equals("get")) {
            System.err.println("get command arguments: [-R | --recursive] <grid filename> <local filename>");
        }
        else if (args[0].equals("put")) {
            System.err.println("put command arguments: [-R | --recursive] <local filename> <grid filename>");
        }
        else if (args[0].equals("delete")) {
            System.err.println("delete command arguments: [-R | --recursive] <grid filename>");
        }
        else if (args[0].equals("touch")) {
            System.err.println("touch command arguments: [-R | --recursive] <grid filename> [<desired storage node>]");
        }
        else if (args[0].equals("makepublic")) {
            System.err.println("makepublic command arguments: [-R | --recursive] <grid filename>");
        }
        else if (args[0].equals("makeprivate")) {
            System.err.println("makeprivate command arguments: [-R | --recursive] <grid filename>");
        }
        else if (args[0].equals("list")) {
            System.err.println("list command arguments: [ [<switches>] <pattern> | -W <location> | -O <submitter> | -G <group> | -P <group> <permissions>]");
            System.err.println(" Switches:");
            System.err.println("  -l: show all attributes");
            System.err.println("  -w: show file locations");
            System.err.println("  -g: show file group");
            System.err.println("  -c: show file checksum");
            System.err.println("  -s: show file size");
            System.err.println("  -p: show file permissions");
            System.err.println("  -o: show file submitter");
            System.err.println(" Special options:");
            System.err.println("  -W: list all files at location");
            System.err.println("  -O: list all files submitted by");
            System.err.println("  -G: list all files in group");
            System.err.println("  -P: list all files in group with permissions");
        }
        else {
            System.err.println("Unrecognised command '" + args[0] + "'");
            System.err.println("Valid commands are:");
            System.err.println("  setnodes ping add remove disable enable retire unretire replcount");
            System.err.println("  get put delete touch makepublic makeprivate list lock unlock");
        }
    }

    public static void main(String[] args) {
        int i;

        if (args.length == 0) {
            usage(args);
            return;
        }

        try {
            DigsClient client;

            /* update the node settings */
            if (args[0].equals("setnodes")) {
                if (args.length == 3) {
                    client = new DigsClient(args[1], args[2], null, null);
                }
                else if (args.length == 5) {
                    client = new DigsClient(args[1], args[2], args[3], args[4]);
                }
                else {
                    usage(args);
                }
                return;
            }

            client = new DigsClient();

            if (args[0].equals("ping")) {
                if (args.length != 1) {
                    usage(args);
                    return;
                }
                if (client.ping()) {
                    System.out.println("Control thread is alive");
                }
                else {
                    System.out.println("Unable to contact control thread");
                }
            }
            else if (args[0].equals("replcount")) {
                boolean check = false, recursive = false, deflt = false;
                String name = null;
                int count = -1;
                /* check params, this is a complicated one */
                if (args.length < 3) {
                    usage(args);
                    return;
                }
                for (i = 1; i < args.length; i++) {
                    if ((args[i].equals("-R")) || (args[i].equals("--recursive"))) {
                        recursive = true;
                    }
                    else if (args[i].equals("-c")) {
                        check = true;
                    }
                    else if (args[i].equals("-d")) {
                        deflt = true;
                    }
                    else if (name == null) {
                        name = args[i];
                    }
                    else if (count < 0) {
                        try {
                            count = Integer.parseInt(args[i]);
                        }
                        catch (NumberFormatException nfe) {
                            usage(args);
                            return;
                        }
                    }
                    else {
                        usage(args);
                        return;
                    }
                }
                /* sanity check params */
                if ((name == null) ||
                    ((count < 0) && (!deflt) && (!check)) ||
                    ((count >= 0) && (check)) ||
                    ((count >= 0) && (deflt)) ||
                    ((check) && (deflt))) {
                    usage(args);
                    return;
                }
                
                if (check) {
                    if (recursive) {
                        client.checkReplicaCountDirectory(name);
                    }
                    else {
                        client.checkReplicaCount(name);
                    }
                }
                else {
                    /* default represented by count of zero */
                    if (deflt) count = 0;
                    if (recursive) {
                        client.setReplicaCountDirectory(name, count);
                    }
                    else {
                        client.setReplicaCount(name, count);
                    }
                }
            }
            else if (args[0].equals("lock")) {
                boolean check = false, recursive = false;
                String name = null;
                /* check lock params */
                if (args.length < 2) {
                    usage(args);
                    return;
                }
                for (i = 1; i < args.length; i++) {
                    if ((args[i].equals("-R")) || (args[i].equals("--recursive"))) {
                        recursive = true;
                    }
                    else if (args[i].equals("-c")) {
                        check = true;
                    }
                    else {
                        if (name != null) {
                            usage(args);
                            return;
                        }
                        name = args[i];
                    }
                }
                if (name == null) {
                    usage(args);
                    return;
                }

                /* do lock */
                if (recursive) {
                    if (check) {
                        client.checkDirectoryLock(name);
                    }
                    else {
                        client.lockDirectory(name);
                    }
                }
                else {
                    if (check) {
                        client.checkLock(name);
                    }
                    else {
                        client.lockFile(name);
                    }
                }
            }
            else if (args[0].equals("unlock")) {
                if (args.length == 2) {
                    client.unlockFile(args[1]);
                }
                else if (args.length == 3) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    client.unlockDirectory(args[2]);
                }
                else {
                    usage(args);
                    return;
                }
            }
            else if (args[0].equals("add")) {
                if (args.length != 4) {
                    usage(args);
                    return;
                }
                client.addNode(args[1], args[2], args[3]);
            }
            else if (args[0].equals("remove")) {
                if (args.length != 2) {
                    usage(args);
                    return;
                }
                client.removeNode(args[1]);
            }
            else if (args[0].equals("disable")) {
                if (args.length != 2) {
                    usage(args);
                    return;
                }
                client.disableNode(args[1]);
            }
            else if (args[0].equals("enable")) {
                if (args.length != 2) {
                    usage(args);
                    return;
                }
                client.enableNode(args[1]);
            }
            else if (args[0].equals("retire")) {
                if (args.length != 2) {
                    usage(args);
                    return;
                }
                client.retireNode(args[1]);
            }
            else if (args[0].equals("unretire")) {
                if (args.length != 2) {
                    usage(args);
                    return;
                }
                client.unretireNode(args[1]);
            }
            else if (args[0].equals("get")) {
                if (args.length == 3) {
                    client.getFile(args[1], args[2]);
                }
                else if (args.length == 4) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    client.getDirectory(args[2], args[3]);
                }
                else {
                    usage(args);
                }
            }
            else if (args[0].equals("put")) {
                if (args.length == 3) {
                    client.putFile(args[1], args[2]);
                }
                else if (args.length == 4) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    client.putDirectory(args[2], args[3]);
                }
                else {
                    usage(args);
                }
            }
            else if (args[0].equals("delete")) {
                if (args.length == 2) {
                    client.deleteFile(args[1]);
                }
                else if (args.length == 3) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    System.out.println("This will delete all logical files under " + args[2]);
                    System.out.println("Are you sure you want to continue? (Y/N)");
                    char c = (char) System.in.read();
                    if ((c == 'y') || (c == 'Y')) {
                        client.deleteDirectory(args[2]);
                    }
                }
                else {
                    usage(args);
                }
            }
            else if (args[0].equals("touch")) {
                if (args.length == 2) {
                    client.touchFile(args[1], null);
                }
                else if (args.length == 3) {
                    if ((args[1].equals("-R")) || (args[1].equals("--recursive"))) {
                        client.touchDirectory(args[2], null);
                    }
                    else {
                        client.touchFile(args[1], args[2]);
                    }
                }
                else if (args.length == 4) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    client.touchDirectory(args[2], args[3]);
                }
                else {
                    usage(args);
                }
            }
            else if (args[0].equals("makepublic")) {
                if (args.length == 2) {
                    client.setPermissions(args[1], true, false);
                }
                else if (args.length == 3) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    client.setPermissions(args[2], true, true);
                }
                else {
                    usage(args);
                }
            }
            else if (args[0].equals("makeprivate")) {
                if (args.length == 2) {
                    client.setPermissions(args[1], false, false);
                }
                else if (args.length == 3) {
                    if ((!args[1].equals("-R")) && (!args[1].equals("--recursive"))) {
                        usage(args);
                        return;
                    }
                    client.setPermissions(args[2], false, true);
                }
                else {
                    usage(args);
                }
            }
            else if (args[0].equals("list")) {
                int flags = 0;
                String wc = "*";

                // handle remaining arguments
                for (i = 1; i < args.length; i++) {
                    if (args[i].equals("-l")) {
                        flags = LIST_SHOW_LONG;
                    }
                    else if (args[i].equals("-n")) {
                        flags |= LIST_SHOW_NUM_COPIES;
                    }
                    else if (args[i].equals("-w")) {
                        flags |= LIST_SHOW_LOCATIONS;
                    }
                    else if (args[i].equals("-g")) {
                        flags |= LIST_SHOW_GROUP;
                    }
                    else if (args[i].equals("-c")) {
                        flags |= LIST_SHOW_CHECKSUM;
                    }
                    else if (args[i].equals("-s")) {
                        flags |= LIST_SHOW_SIZE;
                    }
                    else if (args[i].equals("-p")) {
                        flags |= LIST_SHOW_PERMISSIONS;
                    }
                    else if (args[i].equals("-o")) {
                        flags |= LIST_SHOW_SUBMITTER;
                    }
                    else if (args[i].equals("-W")) {
                        if (i != (args.length - 2)) {
                            usage(args);
                            return;
                        }
                        // list files by location
                        client.listByNode(args[i+1]);
                        return;
                    }
                    else if (args[i].equals("-O")) {
                        // list files by submitter
                        if (i != (args.length - 2)) {
                            usage(args);
                            return;
                        }
                        client.listBySubmitter(args[i+1]);
                        return;
                    }
                    else if (args[i].equals("-G")) {
                        // list files by group
                        if (i != (args.length - 2)) {
                            usage(args);
                            return;
                        }
                        client.listByGroup(args[i+1]);
                        return;
                    }
                    else if (args[i].equals("-P")) {
                        // list files by permissions
                        if (i != (args.length - 3)) {
                            usage(args);
                            return;
                        }
                        client.listByPermissions(args[i+1], args[i+2]);
                        return;
                    }
                    else {
                        if (!wc.equals("*")) {
                            usage(args);
                            return;
                        }
                        wc = args[i];
                    }
                }

                client.list(wc, flags);
            }
            else {
                usage(args);
            }
        }
        catch (Exception e) {
            System.err.println("Exception occurred: " + e);
        }
    }
};
