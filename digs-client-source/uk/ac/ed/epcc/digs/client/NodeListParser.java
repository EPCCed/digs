package uk.ac.ed.epcc.digs.client;

import java.util.Hashtable;
import java.util.Vector;
import java.io.IOException;
import java.io.InputStream;
import java.io.ByteArrayInputStream;

import uk.ac.ed.epcc.digs.DigsException;

public class NodeListParser {
    public static Vector parseSimpleNodeList(byte[] data) throws DigsException {
        Vector list = new Vector();
        try {
            InputStream in = new ByteArrayInputStream(data);
            StringBuffer sb = new StringBuffer();

            int c = in.read();
            while (c >= 0) {
                if ((c == '\n') || (c == '\r')) {
                    if (sb.length() > 0) {
                        String line = sb.toString().trim();
                        if (!line.equals("")) {
                            list.add(line);
                        }
                        sb.setLength(0);
                    }
                }
                else {
                    sb.append((char) c);
                }
                c = in.read();
            }
        }
        catch (IOException e) {
            throw new DigsException("Error parsing node list file: " + e.toString());
        }
        return list;
    }

    public static Hashtable parseMainNodeList(byte[] data) throws DigsException {
        String[] nextVal;
        DigsConfigFile mnl;

        try {
            mnl = new DigsConfigFile(data);
        }
        catch (IOException e) {
            throw new DigsException("Error parsing mainnodelist.conf file: " + e.toString());
        }

        StorageNode node = new StorageNode();
        Hashtable storageNodes = new Hashtable();

        nextVal = mnl.getKeyAndValue();
        if (nextVal == null) {
            throw new DigsException("mainnodelist.conf is empty");
        }
        if (!nextVal[0].equals("node")) {
            throw new DigsException("Expected 'node' first in mainnodelist.conf");
        }
        node.setName(nextVal[1]);

        nextVal = mnl.getKeyAndValue();
        while (nextVal != null) {

            if (nextVal[0].equals("node")) {
                if (!node.isValid()) {
                    throw new DigsException("Incomplete definition for node " + node.getName() + 
                                            " in mainnodelist.conf");
                }
                if (storageNodes.put(node.getName(), node) != null) {
                    throw new DigsException("Duplicate entries for " + node.getName() +
                                            " in mainnodelist.conf");
                }
                node = new StorageNode();
                node.setName(nextVal[1]);
            }
            else if (nextVal[0].equals("path")) {
                node.setPath(nextVal[1]);
            }
            else if (nextVal[0].equals("site")) {
                node.setSite(nextVal[1]);
            }
            else if (nextVal[0].equals("disk")) {
                try {
                    node.setFreeSpace(Long.parseLong(nextVal[1]));
                }
                catch (NumberFormatException e) {
                    throw new DigsException("Invalid free space value '" + nextVal[1] +
                                            "' in mainnodelist.conf");
                }
            }
            else if (nextVal[0].equals("extrarsl")) {
                node.setExtraRsl(nextVal[1]);
            }
            else if (nextVal[0].equals("extrajsscontact")) {
                node.setExtraJssContact(nextVal[1]);
            }
            else if (nextVal[0].equals("jobtimeout")) {
                try {
                    node.setJobTimeout(Float.parseFloat(nextVal[1]));
                }
                catch (NumberFormatException e) {
                    throw new DigsException("Invalid job timeout value '" + nextVal[1] +
                                            "' in mainnodelist.conf");
                }
            }
            else if (nextVal[0].equals("ftptimeout")) {
                try {
                    node.setFtpTimeout(Float.parseFloat(nextVal[1]));
                }
                catch (NumberFormatException e) {
                    throw new DigsException("Invalid FTP timeout value '" + nextVal[1] +
                                            "' in mainnodelist.conf");
                }
            }
            else if (nextVal[0].equals("copytimeout")) {
                try {
                    node.setCopyTimeout(Float.parseFloat(nextVal[1]));
                }
                catch (NumberFormatException e) {
                    throw new DigsException("Invalid copy timeout value '" + nextVal[1] +
                                            "' in mainnodelist.conf");
                }
            }
            else if (nextVal[0].equals("gpfs")) {
                if (nextVal[1].equals("0")) {
                    node.setGpfs(false);
                }
                else {
                    node.setGpfs(true);
                }
            }
            else if (nextVal[0].equals("type")) {
                if (nextVal[1].equals("globus")) {
                    node.setType(StorageNode.TYPE_GLOBUS);
                }
                else if (nextVal[1].equals("srm")) {
                    node.setType(StorageNode.TYPE_SRM);
                }
                else if (nextVal[1].equals("omero")) {
                    node.setType(StorageNode.TYPE_OMERO);
                }
                else {
                    throw new DigsException("Unrecognised type " + nextVal[1] + " in mainnodelist.conf");
                }
            }
            else if (nextVal[0].equals("inbox")) {
                node.setInbox(nextVal[1]);
            }
            else if (nextVal[0].startsWith("data")) {
                int disknum;
                if (nextVal[0].equals("data")) {
                    disknum = 0;
                }
                else {
                    String disknumstr = nextVal[0].substring(4);
                    try {
                        disknum = Integer.parseInt(disknumstr);
                    }
                    catch (NumberFormatException e) {
                        throw new DigsException("Invalid disk quota " + nextVal[0] + " in mainnodelist.conf");
                    }
                }

                if ((disknum + 1) > node.getNumDisks()) {
                    node.setNumDisks(disknum + 1);
                }

                try {
                    node.setQuota(disknum, Long.parseLong(nextVal[1]));
                }
                catch (NumberFormatException e) {
                    throw new DigsException("Invalid disk quota " + nextVal[1] + " in mainnodelist.conf");
                }
            }
	    else {
		node.setProperty(nextVal[0], nextVal[1]);
	    }

            nextVal = mnl.getKeyAndValue();
        }

        /* put the last node in the list */
        if (!node.isValid()) {
            throw new DigsException("Incomplete definition for node " + node.getName() + 
                                    " in mainnodelist.conf");
        }
        if (storageNodes.put(node.getName(), node) != null) {
            throw new DigsException("Duplicate entries for " + node.getName() +
                                    " in mainnodelist.conf");
        }

        return storageNodes;
    }
};
