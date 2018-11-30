/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser;

import javax.swing.UIManager;
import uk.ac.ed.epcc.qcdgrid.browser.GUI.MainGUIFrame;

/**
 * Title:        Browser
 * Description: The application constructor
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class Browser {
    /** whether or not to pack the main browser frame */
    boolean packFrame = false;
    MainGUIFrame frame = null;

    /**Construct the application*/
    public Browser() {
	frame = new MainGUIFrame();
	//Validate frames that have preset sizes
	//Pack frames that have useful preferred size info, e.g. from their layout
	if (packFrame) {
	    frame.pack();
	}
	else {
	    frame.validate();
	}
	frame.setVisible(true);
    }

    public void exit() {
	frame.exit();
    }

    public boolean isFinished() {
	return frame.isFinished();
    }

    /**Main method*/
    public static void main(String[] args) {
	try {
	    UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	new Browser();
    }
}
