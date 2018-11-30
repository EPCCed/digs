/*
 * GridFTP functionality for DiGS. This is a fairly thin wrapper around the CoG GridFTPClient
 * class.
 */
package uk.ac.ed.epcc.digs.client;

import java.util.Hashtable;

import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import java.net.InetAddress;

import org.globus.ftp.GridFTPClient;
import org.globus.ftp.Session;
import org.globus.ftp.DataSink;
import org.globus.ftp.DataSource;
import org.globus.ftp.Buffer;
import org.globus.ftp.Marker;
import org.globus.ftp.MarkerListener;
import org.globus.ftp.exception.ClientException;
import org.globus.ftp.exception.ServerException;

import org.globus.ftp.DataSourceStream;
import org.globus.ftp.DataSinkStream;

import uk.ac.ed.epcc.digs.DigsException;

public class DigsFTPClient implements DataSink, MarkerListener, Runnable,
				      StorageElementInterface
{

    class DigsSourceStream extends DataSourceStream {
        private TransferMonitor monitor;
        public DigsSourceStream(InputStream in, TransferMonitor tm) {
            super(in);
            monitor = tm;
        }

        public Buffer read() throws IOException {
            Buffer buffer = super.read();
            if (monitor != null) {
                monitor.updateBytesTransferred(totalRead);
            }
            return buffer;
        }
    }

    class DigsSinkStream extends DataSinkStream {
        private TransferMonitor monitor;
        public DigsSinkStream(OutputStream out, TransferMonitor tm) {
            super(out);
            monitor = tm;
        }

        public void write(Buffer buffer) throws IOException {
            super.write(buffer);
            if (monitor != null) {
                monitor.updateBytesTransferred(offset);
            }
        }
    }


    public static final int OP_GET = 0;
    public static final int OP_PUT = 1;
    public static final int OP_RENAME = 2;
    public static final int OP_GETBYTES = 3;

    private static Hashtable timeouts = new Hashtable();
    private static Hashtable longTimeouts = new Hashtable();

    private int operation;
    private String param1, param2;
    private String remoteServer;
    private Exception exception;
    private byte[] bytes;

    private TransferMonitor monitor;

    private short portNumber;

    private String lockedFilename(String filename) {
	int lastslash = filename.lastIndexOf('/');
	if (lastslash < 0) {
	    return "LOCKED-" + filename;
	}
	return filename.substring(0, lastslash+1) + "LOCKED-"
	    + filename.substring(lastslash+1);
    }

    private GridFTPClient gridftp;
    private boolean doLock;

    private void init(String server) throws IOException, ServerException, ClientException {

	// Without this, the first gridftp connection sometimes fails from the GUI (???)
	InetAddress addr = InetAddress.getByName(server);

        gridftp = new GridFTPClient(server, portNumber);
        gridftp.authenticate(null);
        gridftp.setType(Session.TYPE_IMAGE);
        gridftp.setPassiveMode(true);
    }

    public DigsFTPClient() {
	portNumber = 2811;
	doLock = true;
    }

    public DigsFTPClient(String server) throws IOException, ServerException, ClientException {
	portNumber = 2811;
	doLock = true;
        init(server);
    }

    public void setPort(short port)
    {
	portNumber = port;
    }

    public void setLock(boolean lock)
    {
	doLock = lock;
    }

    public void put(String localFilename, String remoteFilename) throws Exception {
	//gridftp.put(new File(localFilename), remoteFilename, false);
        DataSource source = new DigsSourceStream(new FileInputStream(new File(localFilename)), monitor);
        gridftp.put(remoteFilename, source, null, false);
    }

    public void get(String remoteFilename, String localFilename) throws IOException, ServerException, ClientException {
        //gridftp.get(remoteFilename, new File(localFilename));
        DataSink sink = new DigsSinkStream(new FileOutputStream(new File(localFilename)), monitor);
        gridftp.get(remoteFilename, sink, null);
    }

    private byte[] databuf;
    private long bufferOffset;

    public byte[] get(String remoteFilename) throws IOException, ServerException, ClientException {
        long filesize = gridftp.size(remoteFilename);

        databuf = new byte[(int)filesize];
        bufferOffset = 0;

        gridftp.get(remoteFilename, this, this);

        return databuf;
    }

    public void end() throws IOException, ServerException {
        gridftp.close();
    }

    public void rename(String source, String dest) throws Exception {
	gridftp.rename(source, dest);
    }

    /* FIXME: add other needed methods */

    /*
     * Data sink interface
     */
    public void close() {
    }

    public void write(Buffer buffer) {
        byte[] buf = buffer.getBuffer();
        int len = buffer.getLength();
        long offset = buffer.getOffset();
        if (offset < 0) {
            if ((((int)bufferOffset) + len) > databuf.length) {
                // sometimes data buffer isn't big enough, possibly binary/ASCII translation differences
                // so expand it if necessary
                byte[] newdatabuf = new byte[((int)bufferOffset) + len];
                for (int j = 0; j < databuf.length; j++) {
                    newdatabuf[j] = databuf[j];
                }
                databuf = newdatabuf;
            }

            for (int i = 0; i < len; i++) {
                databuf[((int) bufferOffset) + i] = buf[i];
            }
            bufferOffset += len;
        }
        else {
            for (int i = 0; i < len; i++) {
                databuf[((int) offset) + i] = buf[i];
            }
        }
    }

    /*
     * Threading/time outs/etc.
     */
    public void run() {
        exception = null;
        try {
            init(remoteServer);
            switch (operation) {
            case OP_PUT:
		if (doLock) {
		    String lockedname = lockedFilename(param2);
		    put(param1, lockedname);
		    rename(lockedname, param2);
		}
		else {
		    put(param1, param2);
		}
                break;
            case OP_GET:
                get(param1, param2);
                break;
            case OP_RENAME:
                rename(param1, param2);
                break;
            case OP_GETBYTES:
                bytes = get(param1);
                break;
            }
            end();
        }
        catch (Exception e) {
            /* put exception where other thread can get at it */
            exception = e;
        }
    }

    public void runFtpWithTimeout() throws Exception {
        long timeout = 300000;

        Float f = (Float)timeouts.get(remoteServer);
        if (f != null) {
            timeout = (long)(f.floatValue() * 1000.0);
        }

        long currtime;
        Thread t = new Thread(this);
        t.start();
        long start = System.currentTimeMillis();
        while (t.isAlive()) {
            currtime = System.currentTimeMillis();
            if ((currtime - start) >= timeout) {
                throw new DigsException("FTP operation on " + remoteServer + " timed out");
            }
            Thread.sleep(10);
        }
        if (exception != null) {
            throw exception;
        }
    }

    public void runFtpWithLongTimeout() throws Exception {
        long timeout = 300000;

        Float f = (Float)longTimeouts.get(remoteServer);
        if (f != null) {
            timeout = (long)(f.floatValue() * 1000.0);
        }

        long currtime;
        Thread t = new Thread(this);
        t.start();
        long start = System.currentTimeMillis();
        while (t.isAlive()) {
            currtime = System.currentTimeMillis();
            if ((currtime - start) >= timeout) {
                throw new DigsException("FTP operation on " + remoteServer + " timed out");
            }
            Thread.sleep(10);
        }
        if (exception != null) {
            throw exception;
        }
    }

    /*
     * Marker listener interface
     */
    public void markerArrived(Marker m) {
    }

    /*
     * Static helper functions. Useful when just one operation is to be performed on a server.
     */
    public static void putFile(String server, String localFilename, String remoteFilename) throws Exception {
	DigsFTPClient ftp = new DigsFTPClient();
        ftp.putFile(server, localFilename, remoteFilename, null);
    }

    public void putFile(String server, String localFilename, String remoteFilename, TransferMonitor tm) throws Exception {
	remoteServer = server;
	param1 = localFilename;
	param2 = remoteFilename;
	operation = OP_PUT;
	monitor = tm;
	runFtpWithLongTimeout();
    }

    public static void getFile(String server, String remoteFilename, String localFilename) throws Exception {
	DigsFTPClient ftp = new DigsFTPClient();
        ftp.getFile(server, remoteFilename, localFilename, null);
    }

    public void getFile(String server, String remoteFilename, String localFilename, TransferMonitor tm) throws Exception {
	remoteServer = server;
	param1 = remoteFilename;
	param2 = localFilename;
	operation = OP_GET;
	monitor = tm;
	runFtpWithLongTimeout();
    }

    public static byte[] getFile(String server, String remoteFilename) throws Exception {
        DigsFTPClient ftp = new DigsFTPClient();
        ftp.remoteServer = server;
        ftp.param1 = remoteFilename;
        ftp.operation = OP_GETBYTES;
        ftp.runFtpWithTimeout();
        return ftp.bytes;
    }

    public static void renameFile(String server, String source, String dest) throws Exception {
        DigsFTPClient ftp = new DigsFTPClient();
        ftp.remoteServer = server;
        ftp.param1 = source;
        ftp.param2 = dest;
        ftp.operation = OP_RENAME;
        ftp.runFtpWithTimeout();
    }

    public static void setServerTimeout(String server, float to) {
        Float f = new Float(to);
        timeouts.put(server, f);
    }

    public static void setServerLongTimeout(String server, float to) {
        Float f = new Float(to);
        longTimeouts.put(server, f);
    }
};
