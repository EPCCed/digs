/*
 * The CoG kit doesn't provide a socket that's compatible with the
 * one RLS uses, so we have to provide this one that has custom
 * input and output streams. This class is the same as the CoG's
 * GSIGssSocket except it uses RlsInputStream and RlsOutputStream
 * as the input and output. 
 *
 * The reason we inherit from GssSocket and do everything else the
 * same as GSIGssSocket is so that Globus will do the authentication
 * for us.
 */

package uk.ac.ed.epcc.digs.client.rls;

import java.net.Socket;
import java.io.IOException;

import org.globus.gsi.gssapi.net.GssSocket;

import org.ietf.jgss.GSSContext;

class RlsSocket extends GssSocket {

    public RlsSocket(String host, int port, GSSContext context)
	throws IOException {
	super(host, port, context);
    }

    public RlsSocket(Socket socket, GSSContext context) {
	super(socket, context);
    }

    public void setWrapMode(int mode) {
	this.mode = mode;
    }

    public int getWrapMode() {
	return this.mode;
    }

    protected void writeToken(byte [] token)
	throws IOException {
	if (this.out == null) {
	    if (this.mode == -1) {
		if (this.in != null) {
		    this.mode = ((RlsInputStream)in).getWrapMode();
		}
	    }
	    this.out = new RlsOutputStream(this.socket.getOutputStream(), 
					      this.context,
					      this.mode);
	}
	((RlsOutputStream)this.out).writeToken(token);
    }
    
    protected byte[] readToken()
	throws IOException {
	if (this.in == null) {
	    this.in = new RlsInputStream(this.socket.getInputStream(),
					    this.context);
	}
	return ((RlsInputStream)this.in).readHandshakeToken();
    }
    
}

