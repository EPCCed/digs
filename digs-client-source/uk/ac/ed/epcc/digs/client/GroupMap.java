package uk.ac.ed.epcc.digs.client;

import java.util.Vector;
import java.util.Hashtable;
import java.io.IOException;
import java.io.InputStream;
import java.io.ByteArrayInputStream;

import uk.ac.ed.epcc.digs.DigsException;

public class GroupMap {

    private Hashtable groupmap;

    public GroupMap() {
        groupmap = new Hashtable();
    }

    public GroupMap(byte[] groupmapfile) throws DigsException {
        groupmap = new Hashtable();

        try {
            InputStream in = new ByteArrayInputStream(groupmapfile);
            StringBuffer sb = new StringBuffer();

            int c = in.read();
            while (c >= 0) {
                if ((c == '\n') || (c == '\r')) {
                    if (sb.length() > 0) {
                        String line = sb.toString().trim();
                        if (!line.equals("")) {
                            String dn, groups;
                            Vector v = new Vector();

                            /* parse line */
                            if (line.charAt(0) == '"') {
                                /* DN is in quotes */
                                int endQuote = line.indexOf('"', 1);
                                if (endQuote < 0) {
                                    throw new DigsException("Invalid line " + line + " in group map file");
                                }
                                dn = line.substring(1, endQuote);
                                groups = line.substring(endQuote + 1).trim();
                            }
                            else {
                                /* DN is not in quotes */
                                int space = line.indexOf(' ');
                                if (space < 0) {
                                    throw new DigsException("Invalid line " + line + " in group map file");
                                }
                                dn = line.substring(0, space);
                                groups = line.substring(space).trim();
                            }

                            /* parse groups */
                            String[] grps = groups.split(",");
                            for (int i = 0; i < grps.length; i++) {
                                v.add(grps[i]);
                            }
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
            throw new DigsException("Error parsing group map file: " + e.toString());
        }
    }

    public Vector getUserGroups(String user) {
        Vector v = (Vector)groupmap.get(user);
        return v;
    }

    public boolean isUserInGroup(String user, String group) {
        Vector v = getUserGroups(user);
        if (v == null) return false;

        for (int i = 0; i < v.size(); i++) {
            String grp = (String)v.elementAt(i);
            if (grp.equals(group)) return true;
        }
        return false;
    }
};
