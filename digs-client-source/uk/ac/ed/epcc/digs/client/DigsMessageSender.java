/*
 * Sends messages to DiGS control node
 */

package uk.ac.ed.epcc.digs.client;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.gridforum.jgss.ExtendedGSSManager;
import org.gridforum.jgss.ExtendedGSSContext;

import org.globus.gsi.gssapi.GSSConstants;
import org.globus.gsi.gssapi.net.GssSocket;
import org.globus.gsi.gssapi.net.GssSocketFactory;
import org.globus.gsi.gssapi.auth.HostAuthorization;

import org.ietf.jgss.GSSContext;
import org.ietf.jgss.GSSManager;
import org.ietf.jgss.GSSException;

public class DigsMessageSender {

    /* name of main node */
    private String hostname;

    /* port of control thread's socket on main node */
    private int port;

    public DigsMessageSender(String host, int port) {
        this.hostname = host;
        this.port = port;
    }

    public String sendMessage(String message) throws IOException, GSSException {
        /* default authentication, will use user's proxy */
        GSSManager manager = ExtendedGSSManager.getInstance();
        ExtendedGSSContext context = 
            (ExtendedGSSContext)manager.createContext(null,
                                                      GSSConstants.MECH_OID,
                                                      null,
                                                      GSSContext.DEFAULT_LIFETIME);
        
        GssSocketFactory factory = GssSocketFactory.getDefault();
        GssSocket sock = (GssSocket)factory.createSocket(hostname, port, context);

        byte[] response = new byte[32];

        /* check host certificate */
        sock.setAuthorization(new HostAuthorization(null));

        OutputStream out = sock.getOutputStream();
        InputStream in = sock.getInputStream();

        /* send the message */
        out.write(message.getBytes());
        out.flush();

        /* read the response */
        in.read(response);

        sock.close();

        String resstr = new String(response);
        return resstr;
    }

};
