/*
 * Output stream for RlsSocket. Very similar to CoG's GSIGssOutputStream,
 * but adds the rlsWrite method for writing a NULL-terminated string to
 * the stream, as required by the RLS protocol.
 */

package uk.ac.ed.epcc.digs.client.rls;

import java.io.OutputStream;
import java.io.IOException;

import org.globus.gsi.gssapi.net.GssSocket;
import org.globus.gsi.gssapi.net.GssOutputStream;
import org.globus.gsi.gssapi.SSLUtil;

import org.ietf.jgss.GSSContext;

class RlsOutputStream extends GssOutputStream {

    protected byte [] header;
    protected int mode;
    
    public RlsOutputStream(OutputStream out, GSSContext context) {
	this(out, context, GssSocket.SSL_MODE);
    }

    public RlsOutputStream(OutputStream out, GSSContext context, int mode) {
	super(out, context);
	this.header = new byte[4];
	setWrapMode(mode);
    }

    public void flush() 
	throws IOException {
	if (this.index == 0) return;
	writeToken(wrap());
	this.index = 0;
    }

    public void rlsWrite(String str) throws IOException {
	byte[] buf = str.getBytes();
        this.out.write(buf);
        this.out.write(0);
        this.out.flush();
    }

    public void setWrapMode(int mode) {
	this.mode = mode;
    }

    public int getWrapMode() {
	return this.mode;
    }

    public void writeToken(byte[] token)
	throws IOException {
	if (this.mode == GssSocket.GSI_MODE) {
	    SSLUtil.writeInt(token.length, this.header, 0);
	    this.out.write(this.header);
	}
	this.out.write(token);
	this.out.flush();
    }
    
}
