/*
 * This class implements a simple RLS client.
 *
 * RLS works using a simple protocol involving writing NULL-terminated
 * strings over an authenticated socket connection. See source of C RLS
 * client package for protocol details.
 */
package uk.ac.ed.epcc.digs.client.rls;

import java.io.IOException;
import java.util.Vector;
import java.util.Hashtable;

import org.gridforum.jgss.ExtendedGSSManager;
import org.gridforum.jgss.ExtendedGSSContext;

import org.globus.gsi.gssapi.GSSConstants;
import org.globus.gsi.gssapi.auth.HostAuthorization;

import org.ietf.jgss.GSSContext;
import org.ietf.jgss.GSSManager;
import org.ietf.jgss.GSSException;

public class RlsClient {

    /* Socket for communication with server */
    private RlsSocket socket;

    /* Input stream for reading from server */
    private RlsInputStream in;

    /* Output stream for writing to server */
    private RlsOutputStream out;

    /*
     * Connects to the specified RLS server
     */
    public RlsClient(String host, int port) throws IOException, GSSException {

	// Get security context (default will authorise with user's proxy)
	GSSManager manager = ExtendedGSSManager.getInstance();
	ExtendedGSSContext context = 
	    (ExtendedGSSContext)manager.createContext(null,
						      GSSConstants.MECH_OID,
						      null,
						      GSSContext.DEFAULT_LIFETIME);

	// Create RLS socket
	socket = new RlsSocket(host, port, context);
	
	// Setup authorisation
	socket.setAuthorization(new HostAuthorization(null));

	// Get input and output streams
	in = (RlsInputStream)socket.getInputStream();
	out = (RlsOutputStream)socket.getOutputStream();
	
	// Read error code
	int res = (int)in.rlsReadLong();
	if (res != 0) {
	    throw new IOException("Error opening RLS: " + errorCodeToString(res));
	}
    }

    public synchronized Vector getFileLocations(String lfn) throws IOException {
	Vector results = new Vector();

	out.rlsWrite("lrc_get_pfn");     // RPC call name
	out.rlsWrite(lfn);               // LFN being looked for
	out.rlsWrite("0");               // offset
	out.rlsWrite("0");               // max results

	int res = (int)in.rlsReadLong();
	if (res != 0) {
            String err = in.rlsRead();
	    throw new IOException("Error getting file locations for " + lfn +
				  " from RLS: " + errorCodeToString(res));
	}

	/*
	 * Results come back as the LFN followed by the next PFN each time.
	 * We know we're done when a zero length string comes.
	 */
	String pfn;
	String lfn2 = in.rlsRead();
	while (lfn2.length() > 0) {
	    pfn = in.rlsRead();
	    results.add(pfn);
	    lfn2 = in.rlsRead();
	}
	return results;
    }

    public synchronized String getAttribute(String lfn, String attr) throws IOException {
	out.rlsWrite("lrc_attr_value_get");  // RPC call name
	out.rlsWrite(lfn);                   // LFN
	out.rlsWrite(attr);                  // attribute name
	out.rlsWrite("0");                   // object type (lrc lfn)

	int res = (int)in.rlsReadLong();

	if (res == 23) {
	    // return empty if attribute doesn't exist rather than throwing
	    // an exception
            // skip one string (attribute name) that is returned
            String nm = in.rlsRead();
	    return "";
	}

        if (res == 12) {
            // if lfn doesn't exist
            // skip LFN returned
            String nm = in.rlsRead();

            throw new IOException("Error getting attribute " + attr + " for " + lfn +
                                  " from RLS: LFN doesn't exist");
        }

	if (res != 0) {
            String err = in.rlsRead();
	    throw new IOException("Error getting attribute " + attr + " for " + lfn +
				  " from RLS: " + errorCodeToString(res));
	}

	String str = in.rlsRead();       // skip attribute name
	str = in.rlsRead();              // skip type
	String val = in.rlsRead();       // get value
	str = in.rlsRead();              // skip empty string terminator
	return val;
    }

    public String getFileDisk(String lfn, String node) throws IOException {
	return getAttribute(lfn, node + "-dir");
    }

    public synchronized boolean lfnExists(String lfn) throws IOException {
	out.rlsWrite("lrc_get_pfn");     // RPC call name
	out.rlsWrite(lfn);               // LFN being looked for
	out.rlsWrite("0");               // offset
	out.rlsWrite("0");               // max results

	int res = (int)in.rlsReadLong();
        if (res == 12) {
            String err = in.rlsRead();
            return false;
        }
	if (res != 0) {
            String err = in.rlsRead();
	    throw new IOException("Error getting file locations for " + lfn +
				  " from RLS: " + errorCodeToString(res));
	}

        skipResults();
        return true;
    }

    public synchronized boolean fileAtLocation(String lfn, String node) throws IOException {
        out.rlsWrite("lrc_mapping_exists");
        out.rlsWrite(lfn);
        out.rlsWrite(node);
        
        int res = (int)in.rlsReadLong();
        if (res == 10) {
            // mapping does not exist
            // skip result
            in.rlsRead();
            return false;
        }
	if (res != 0) {
            in.rlsRead();
	    throw new IOException("Error getting RLS mapping existence: " +
                                  errorCodeToString(res));
	}

        // skip string result
        in.rlsRead();
        return true;
    }

    public synchronized Vector listLocationFiles(String node) throws IOException {
        Vector v = new Vector();

        out.rlsWrite("lrc_get_lfn");
        out.rlsWrite(node);
        out.rlsWrite("0");            // offset
        out.rlsWrite("0");            // resource limit
        
        int res = (int)in.rlsReadLong();
        if (res != 0) {
            String err = in.rlsRead();
            throw new IOException("Error listing lfns at " + node + ": " +
                                  errorCodeToString(res));
        }

        String pfn;
        String lfn = in.rlsRead();
        while (lfn.length() > 0) {
            pfn = in.rlsRead();
            v.add(lfn);
            lfn = in.rlsRead();
        }

        return v;
    }

    public int getNumCopies(String lfn) throws IOException {
        Vector v = getFileLocations(lfn);
        return v.size();
    }

    /*
     * Returns a hash table mapping lfns to disk names
     */
    public Hashtable getAllFileDisks(String node) throws IOException {
        return getAllAttributesValues(node + "-dir");
    }

    /*
     * Translate a UNIX-style pattern (with * and ? wildcards) to an SQL-style one
     * (with % and _)
     */
    private String translatePattern(String pattern) {
        StringBuffer sb = new StringBuffer();
        boolean escaped = false;
        for (int i = 0; i < pattern.length(); i++) {
            char c = pattern.charAt(i);
            if (c == '\\') {
                sb.append('\\');
                escaped = true;
                continue;
            }
            if (escaped) {
                sb.append(c);
            }
            else if (c == '*') {
                sb.append('%');
            }
            else if (c == '?') {
                sb.append('_');
            }
            else {
                if ((c == '%') || (c == '_')) {
                    sb.append('\\');
                }
                sb.append(c);
            }

            escaped = false;
        }
        return sb.toString();
    }

    /*
     * Returns a hashtable of the files that match wc (possibly a wildcard). The keys
     * are the lfns, the values are vectors of locations for each lfn.
     */
    public synchronized Hashtable getFileList(String wc) throws IOException {
        Hashtable ht = new Hashtable();
        String pattern = translatePattern(wc);
	out.rlsWrite("lrc_get_pfn_wc");      // RPC call name
	out.rlsWrite(pattern);               // LFN being looked for
	out.rlsWrite("0");                   // offset
	out.rlsWrite("0");                   // max results
        
	int res = (int)in.rlsReadLong();
        if (res == 12) {
            // If we get "LFN does not exist", return empty hashtable rather than
            // throwing an exception
            in.rlsRead();
            return ht;
        }

	if (res != 0) {
            in.rlsRead();
	    throw new IOException("Error getting file locations for " + wc +
				  " from RLS: " + errorCodeToString(res));
	}

        String lfn = in.rlsRead();
        if (lfn.length() == 0) {
            // empty result set
            return ht;
        }
        String pfn = in.rlsRead();

        String oldlfn = lfn;
        
        Vector v = new Vector();

        v.add(pfn);

        // loop over lfn/pfn pairs returned
        while (pfn.length() > 0) {

            if (!oldlfn.equals(lfn)) {
                ht.put(oldlfn, v);
                v = new Vector();
            }
            v.add(pfn);

            oldlfn = lfn;
            lfn = in.rlsRead();
            if (lfn.length() == 0) {
                ht.put(oldlfn, v);
                return ht;
            }
            pfn = in.rlsRead();
        }

        ht.put(oldlfn, v);

        return ht;
    }

    public synchronized Hashtable getAllAttributesValues(String attr) throws IOException {
        Hashtable ht = new Hashtable();

        out.rlsWrite("lrc_attr_search");   // RPC method
        out.rlsWrite(attr);                // attribute name
        out.rlsWrite("0");                 // object type - globus_rls_obj_lrc_lfn
        out.rlsWrite("0");                 // attribute op - globus_rls_attr_op_all
        out.rlsWrite("");                  // operand 1 - not used
        out.rlsWrite("");                  // operand 2 - not used
        out.rlsWrite("0");                 // offset
        out.rlsWrite("0");                 // resource limit

	int res = (int)in.rlsReadLong();
	if (res != 0) {
	    if ((res == 23) || (res == 29)) {
		// skip name returned
		String nm = in.rlsRead();

		// return empty hashtable
		return ht;
	    }
            in.rlsRead();
	    throw new IOException("Error getting all values for " + attr +
				  " from RLS: " + errorCodeToString(res));
	}

        String lfn = in.rlsRead();
        if (lfn.length() == 0) {
            // empty
            return ht;
        }
        String type = in.rlsRead();
        if (type.length() == 0) {
            return ht;
        }
        String val = in.rlsRead();
        while (val.length() > 0) {
            ht.put(lfn, val);

            lfn = in.rlsRead();
            if (lfn.length() == 0) {
                break;
            }
            type = in.rlsRead();
            if (type.length() == 0) {
                break;
            }
            val = in.rlsRead();
        }

        return ht;
    }

    private void skipResults() throws IOException {
        String str = in.rlsRead();
        while (str.length() > 0) {
            str = in.rlsRead();
        }
    }
    
    private String errorCodeToString(int err) {
	switch (err) {
	case 0: return "OK";
	case 1: return "Globus I/O error";
	case 2: return "Invalid RLS handle";
	case 3: return "Bad URL";
	case 4: return "Out of memory";
	case 5: return "Result too large for buffer";
	case 6: return "Bad argument";
	case 7: return "Permission denied";
	case 8: return "Bad RPC method";
	case 9: return "Request made to wrong server (RLI or LRC)";
	case 10: return "Mapping does not exist";
	case 11: return "LFN already exists";
	case 12: return "LFN does not exist";
	case 13: return "PFN already exists";
	case 14: return "PFN does not exist";
	case 15: return "LRC already exists";
	case 16: return "LRC does not exist";
	case 17: return "Database error";
	case 18: return "RLI already exists";
	case 19: return "RLI does not exist";
	case 20: return "Mapping already exists";
	case 21: return "Invalid attribute type";
	case 22: return "Attribute already exists";
	case 23: return "Attribute does not exist";
	case 24: return "Invalid object type";
	case 25: return "Invalid attribute search operator";
	case 26: return "Operation unsupported";
	case 27: return "I/O operation timed out";
	case 28: return "Too many connections";
	case 29: return "Attribute with specified value not found";
	case 30: return "Attribute in use";
	}
	return "unknown";
    }

    /*
     * FIXME: need a close method (?)
     */
};
