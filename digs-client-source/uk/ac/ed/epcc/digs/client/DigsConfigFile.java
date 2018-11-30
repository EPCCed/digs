/*
 * Parser for DiGS config files
 *
 * They are basically simple files of "key = value" pairs, but reading and
 * storing them is complicated by two things:
 *
 * 1. keys can occur more than once in the same file
 * 2. the order of lines in the file is important
 */

package uk.ac.ed.epcc.digs.client;

import java.io.File;
import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.io.FileInputStream;
import java.io.IOException;

import java.util.Vector;

public class DigsConfigFile {

    private class DigsConfigLine {
        String key;
        String value;
    };

    private Vector lines;

    private int position;

    private void load(InputStream in) throws IOException {
        lines = new Vector();

        StringBuffer sb = new StringBuffer();
        
        int c = in.read();
        while (c >= 0) {
            if ((c == '\n') || (c == '\r')) {
                if (sb.length() > 0) {
                    // reached end of line
                    String line = sb.toString();

                    // remove comment if there is one
                    int hash = line.indexOf('#');
                    if (hash >= 0) {
                        line = line.substring(0, hash);
                    }

                    // remove whitespace on beginning and end
                    line = line.trim();

                    // if line is now empty, ignore
                    if (line.length() > 0) {
                        // find equals
                        int equals = line.indexOf('=');
                        if (equals < 0) {
                            throw new IOException("Malformed line in config file");
                        }
                        
                        // split into key and value
                        String key = line.substring(0, equals).trim();
                        String value = line.substring(equals + 1).trim();
                        
                        // add the new line
                        DigsConfigLine cfgline = new DigsConfigLine();
                        cfgline.key = key;
                        cfgline.value = value;
                        lines.add(cfgline);
                    }

                    // start a new line
                    sb.setLength(0);
                }
            }
            else {
                // append char to current line
                sb.append((char) c);
            }
            c = in.read();
        }

        position = 0;
    }

    public DigsConfigFile(File file) throws IOException {
        load(new FileInputStream(file));
    }

    public DigsConfigFile(byte[] buf) throws IOException {
        load(new ByteArrayInputStream(buf));
    }

    public void setPosition(int pos) {
        if ((pos >= 0) && (pos < lines.size())) {
            position = pos;
        }
    }

    public int getPosition() {
        return position;
    }

    public String getNextValue(String key) {

        while (position < lines.size()) {
            DigsConfigLine line = (DigsConfigLine)lines.elementAt(position);

            if (line.key.equals(key)) {
                position++;
                return line.value;
            }

            position++;
        }

        return null;
    }

    public String getFirstValue(String key) {
        position = 0;
        return getNextValue(key);
    }

    public String[] getKeyAndValue() {
        if (position >= lines.size()) {
            return null;
        }

        DigsConfigLine line = (DigsConfigLine)lines.elementAt(position);
        position++;
        String[] result = new String[2];
        result[0] = line.key;
        result[1] = line.value;
        return result;
    }

    public int getIntValue(String key, int def) {
        String str = getFirstValue(key);
        int val;
        if (str == null) {
            val = def;
        }
        else {
            try {
                val = Integer.parseInt(str);
            }
            catch (NumberFormatException e) {
                val = def;
            }
        }
        return val;
    }

    public int getIntValue(String key) {
        return getIntValue(key, 0);
    }
};
