/***********************************************************************
 *
 *   Filename:   PrefixHandler.java
 *
 *   Authors:    Radoslaw Ostrowski             (radek)       EPCC.
 *   Purpose:    Verifies if appropriate prefix was used
 *
 *   Contents:   PrefixHandler class
 *
 *   Used in:    Java client tools
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2007 The University of Edinburgh
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of the
 *   License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *   MA 02111-1307, USA.
 *
 *   As a special exception, you may link this program with code
 *   developed by the OGSA-DAI project without such code being covered
 *   by the GNU General Public License.
 *
 ***********************************************************************/
package uk.ac.ed.epcc.qcdgrid.metadata;

/**
 * This class is used both directly from the command line, and also by the GUI
 * tools.
 * 
 * September 2005 This class was refactored to make use of the new
 * QCDgridObserver pattern when doing long-running directory uploads.
 * 
 * October 2005 Refactored substantially to make use of the metadata
 * verification code; also just generally "tidied up".
 * 
 */
public class PrefixHandler{

    public static String handlePrefix(String lfn){

	String[] result = lfn.split("://");

	/* More then one "://" */
	if(result.length > 2){ 
	    System.out.println("Error: illegal syntax. LFN consisted of more then one '://' substrings");
	    System.exit(1);
	}

	/* If there was no "://" */
	if(result[0].length() == lfn.length()){
	    return result[0];

	}else{
	    /* If not equals "lfn://" */
	    /* Here add more cases for other supported prefixes*/
	    
	    /* We don't care about the prefixes for now */
	    //if(!result[0].equals("lfn")){
	    //	System.out.println("Error: unrecognised prefix " + result[0]);
	    //	System.exit(1);
	    //}

	    //instead, translate :// into //
	    return result[0] + ":/" + result[1];
	}
    }
}
