package uk.ac.ed.epcc.digs.client;

import java.util.prefs.Preferences;

public class NodeSetter {
    public static void main(String[] args) {
	if (args.length != 2) {
	    System.err.println("Usage: java NodeSetter <main node name> <main node path>");
	}
	else {
	    Preferences prefs = Preferences.userNodeForPackage(DigsClient.class);
	    prefs.put("mainnode", args[0]);
	    prefs.put("mainnodepath", args[1]);
	}
    }
}
