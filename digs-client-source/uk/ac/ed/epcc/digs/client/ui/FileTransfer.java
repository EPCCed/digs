package uk.ac.ed.epcc.digs.client.ui;

import uk.ac.ed.epcc.digs.client.DigsClient;
import uk.ac.ed.epcc.digs.client.TransferMonitor;

import java.io.File;

public class FileTransfer implements Runnable, TransferMonitor {

    public static final int WAITING = 0;
    public static final int UPLOADING = 1;
    public static final int DOWNLOADING = 2;
    public static final int DONE = 3;
    public static final int ERROR = 4;
    public static final int CANCELLED = 5;
    public static final int REFRESHING = 6;

    private String source;
    private String destination;
    private DigsClient digsClient;
    private boolean isPut;
    private boolean isRef;
    private int status;
    private Thread thread;
    private String errorString;
    private TransferTable transferTable;
    private long bytesTransferred;
    private long fileSize;

    public FileTransfer(String src, String dest, DigsClient dc, boolean put, TransferTable tt, long size) {
        source = src;
        destination = dest;
        digsClient = dc;
        isPut = put;
	isRef = false;
        status = WAITING;
        thread = null;
        errorString = "";
        transferTable = tt;
        bytesTransferred = 0;
        fileSize = size;
    }

    public FileTransfer(TransferTable tt) {
	source = "-";
	destination = "-";
	digsClient = null;
	isPut = false;
	isRef = true;
	status = REFRESHING;
	thread = null;
	errorString = "";
	transferTable = tt;
	bytesTransferred = 0;
	fileSize = 1;
    }

    public String getSource() {
        return source;
    }

    public String getDestination() {
        return destination;
    }

    public boolean isPutTransfer() {
        return isPut;
    }

    public boolean isRefresh() {
	return isRef;
    }

    public int getStatus() {
        return status;
    }

    public String getStatusString() {
        String result = "";

        long percentage;
	if (fileSize == 0) {
	    percentage = 100;
	}
	else {
	    percentage = (bytesTransferred * 100) / fileSize;
	}

        switch (status)
        {
        case WAITING:
            result = "Waiting";
            break;
        case UPLOADING:
            result = "Uploading (" + bytesTransferred + "/" + fileSize + " bytes, " + percentage + "%)";
            break;
        case DOWNLOADING:
            result = "Downloading (" + bytesTransferred + "/" + fileSize + " bytes, " + percentage + "%)";
            break;
        case DONE:
            result = "Done";
            break;
        case ERROR:
            result = "Error: " + errorString;
            break;
        case CANCELLED:
            result = "Cancelled";
            break;
	case REFRESHING:
	    result = "Refreshing";
	    break;
        }
        return result;
    }

    public void statusChanged() {
        transferTable.statusChanged();
    }

    public void run() {
        if (status == CANCELLED) {
            return;
        }

        if (isPut) {
            status = UPLOADING;
        }
        else {
            // make sure destination path exists
            File f = new File(destination);
            File dir = f.getParentFile();
            dir.mkdirs();
            status = DOWNLOADING;
        }
        statusChanged();

        try {
            if (isPut) {
                if (digsClient.fileExists(destination)) {
                    digsClient.modifyFile(source, destination, this);
                }
                else {
                    digsClient.putFile(source, destination, this);
                }
                if (status != CANCELLED) {
                    status = DONE;
                    statusChanged();
                }
            }
            else {
                digsClient.getFile(source, destination, this);
                if (status == CANCELLED) {
                    // delete destination file if cancelled while downloading
                    File f = new File(destination);
                    f.delete();
                }
                else {
                    status = DONE;
                    statusChanged();
                }
            }
        }
        catch (Exception ex) {
            if (status != CANCELLED) {
                status = ERROR;
                errorString = ex.toString();
                statusChanged();
            }
        }
    }

    public void start() {
	if (isRef) {
	    // FIXME: make this properly asynchronous
	    status = DONE;
	    statusChanged();
	}
	else {
	    thread = new Thread(this);
	    thread.start();
	}
    }

    public void cancel() {
        status = CANCELLED;
        statusChanged();
    }

    public void updateBytesTransferred(long bytes) {
	if (bytes > bytesTransferred) {
	    bytesTransferred = bytes;
	}
        statusChanged();
    }
}
