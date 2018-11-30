package uk.ac.ed.epcc.digs.client;

import java.util.Hashtable;

public class StorageNode {

    /* SE type enumeration */
    public static final int TYPE_INVALID = -1;
    public static final int TYPE_GLOBUS = 0;
    public static final int TYPE_SRM = 1;
    public static final int TYPE_OMERO = 2;

    /* properties from mainnodelist.conf */
    private String name;
    private String path;
    private String site;
    private long freeSpace;
    private String extraRsl;
    private String extraJssContact;
    private float jobTimeout;
    private float ftpTimeout;
    private float copyTimeout;
    private boolean gpfs;

    private String inbox;
    private int numDisks;
    private int type;
    private long[] quotas;

    private Hashtable properties;

    /* state properties */
    private boolean dead;
    private boolean disabled;
    private boolean retiring;

    public String getInbox() {
	if (type == TYPE_INVALID) {
	    // assume DiGS 2.0
	    return path + "/data/NEW";
	}
        return inbox;
    }

    public void setInbox(String ibx) {
        inbox = ibx;
    }

    public int getNumDisks() {
        return numDisks;
    }

    public void setNumDisks(int n) {
        long[] newQuotas;
        int i;

        newQuotas = new long[n];
        for (i = 0; i < n; i++) {
            if (i < numDisks) {
                newQuotas[i] = quotas[i];
            }
            else {
                newQuotas[i] = 0;
            }
        }

        quotas = newQuotas;
        numDisks = n;
    }

    public long getQuota(int idx) {
        if (idx >= numDisks) {
            return -1;
        }
        return quotas[idx];
    }

    public void setQuota(int idx, long quota) {
        if (idx < numDisks) {
            quotas[idx] = quota;
        }
    }

    public int getType() {
        return type;
    }

    public void setType(int t) {
        type = t;
    }

    public String getName() {
        return name;
    }

    public void setName(String nm) {
        name = nm;
    }

    public String getPath() {
        return path;
    }

    public void setPath(String p) {
        path = p;
    }

    public String getSite() {
        return site;
    }

    public void setSite(String s) {
        site = s;
    }

    public long getFreeSpace() {
        return freeSpace;
    }

    public void setFreeSpace(long spc) {
        freeSpace = spc;
    }

    public String getExtraRsl() {
        return extraRsl;
    }

    public void setExtraRsl(String rsl) {
        extraRsl = rsl;
    }

    public String getExtraJssContact() {
        return extraJssContact;
    }

    public void setExtraJssContact(String jss) {
        extraJssContact = jss;
    }

    public float getJobTimeout() {
        return jobTimeout;
    }

    public void setJobTimeout(float jto) {
        jobTimeout = jto;
    }

    public float getFtpTimeout() {
        return ftpTimeout;
    }

    public void setFtpTimeout(float fto) {
        ftpTimeout = fto;
	if (type == TYPE_GLOBUS) {
	    DigsFTPClient.setServerTimeout(name, ftpTimeout);
	}
    }

    public float getCopyTimeout() {
        return copyTimeout;
    }

    public void setCopyTimeout(float cto) {
        copyTimeout = cto;
	if (type == TYPE_GLOBUS) {
	    DigsFTPClient.setServerLongTimeout(name, copyTimeout);
	}
    }

    public boolean isGpfs() {
        return gpfs;
    }
    
    public void setGpfs(boolean g) {
        gpfs = g;
    }

    public boolean isDead() {
        return dead;
    }

    public void setDead(boolean d) {
        dead = d;
    }

    public boolean isDisabled() {
        return disabled;
    }

    public void setDisabled(boolean d) {
        disabled = d;
    }

    public boolean isRetiring() {
        return retiring;
    }

    public void setRetiring(boolean r) {
        retiring = r;
    }

    public StorageNode() {
        name = "";
        path = "";
        site = "";
        freeSpace = 0;
        extraRsl = "";
        extraJssContact = "";
        jobTimeout = 45.0f;
        ftpTimeout = 45.0f;
        copyTimeout = 600.0f;
        gpfs = false;

        numDisks = 0;
        inbox = "";
        type = TYPE_INVALID;

        dead = false;
        disabled = false;
        retiring = false;

        quotas = null;

	properties = new Hashtable();
    }

    public String getProperty(String propname) {
	return (String)properties.get(propname);
    }

    public void setProperty(String propname, String propval) {
	properties.put(propname, propval);
    }

    public StorageElementInterface getSE() {
	StorageElementInterface se;
	switch (type) {
	case TYPE_GLOBUS:
	    se = new DigsFTPClient();
	    break;
	case TYPE_SRM:
	    se = new SRMAdaptor(getProperty("endpoint"));
	    break;
	default:
	    se = new DigsFTPClient();
	    break;
	}
	return se;
    }

    public boolean isValid() {
        /*
         * Returns true if this object has all the information needed to refer
         * properly to a storage node
         */
        if ((name.equals("")) || (path.equals("")) || (site.equals(""))) {// || (type == TYPE_INVALID)) {
            return false;
        }
        return true;
    }
};
