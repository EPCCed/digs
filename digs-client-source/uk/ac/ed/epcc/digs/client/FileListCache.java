package uk.ac.ed.epcc.digs.client;

import java.util.Hashtable;
import java.util.Date;

public class FileListCache implements Runnable {

    private DigsClient digsClient;

    private Hashtable fileList;
    private Hashtable lockList;
    private Hashtable sizeList;

    private long fileListTimestamp;
    private boolean fileListReady;
    private boolean immediateRefresh;
    private boolean stopping;

    private Exception exception;
    
    private Thread thread;

    private static FileListCache instance;

    public static void create(DigsClient dc) {
	instance = new FileListCache(dc);
    }

    public static FileListCache getInstance() {
	return instance;
    }

    private FileListCache(DigsClient dc) {
	digsClient = dc;
	fileListTimestamp = 0;
	fileList = null;
        lockList = null;
        sizeList = null;
	fileListReady = false;
	immediateRefresh = false;
	stopping = false;
	exception = null;
	thread = new Thread(this);
	thread.start();
    }

    public void run() {
	// loop until told to stop
	while (!stopping) {
	    if ((immediateRefresh) ||
		((System.currentTimeMillis() - fileListTimestamp) > 1200000)) {

		// time to refresh the list
		Hashtable fl, ll, sl;
		try {
		    //System.out.println("Getting lockedby attributes " + (new Date()).toString());
		    ll = digsClient.getAllAttributesValues("lockedby");
		    //System.out.println("Getting size attributes " + (new Date()).toString());
                    sl = digsClient.getAllAttributesValues("size");
		    //System.out.println("Getting file list " + (new Date()).toString());
		    //fl = digsClient.getFileList("*");
		    fl = null;
		    //System.out.println("Done get file list " + (new Date()).toString());
		    fileList = fl;
		    lockList = ll;
                    sizeList = sl;
		    fileListTimestamp = System.currentTimeMillis();
		    fileListReady = true;
		}
		catch (Exception ex) {
		    exception = ex;
		}
		immediateRefresh = false;
	    }
	    try {
		Thread.sleep(100);
	    }
	    catch (Exception ex) {
	    }
	}
    }

    public void stop() {
	stopping = true;
    }

    public boolean isReady() {
	return fileListReady;
    }

    /*public Hashtable getFileList() {
	if (isReady()) {
	    return fileList;
	}

	immediateRefresh = true;
	while (!isReady()) {
	    try {
		Thread.sleep(10);
	    }
	    catch (Exception ex) {
	    }
	}
	return fileList;
	}*/

    public Hashtable getLockList() {
	if (isReady()) {
	    return lockList;
	}

	immediateRefresh = true;
	while (!isReady()) {
	    try {
		Thread.sleep(10);
	    }
	    catch (Exception ex) {
	    }
	}
	return lockList;
    }

    public Hashtable getSizeList() {
        if (isReady()) {
            return sizeList;
        }

	immediateRefresh = true;
	while (!isReady()) {
	    try {
		Thread.sleep(10);
	    }
	    catch (Exception ex) {
	    }
	}
        return sizeList;
    }

    public void refresh() {
	// request immediate refresh and invalidate current list
	immediateRefresh = true;
	fileListReady = false;
    }
};
