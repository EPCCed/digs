/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.io.*;

import javax.swing.tree.*;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.Attributes;

/**
 * Title:        InstanceTreeBuilder
 * Description:  Parses an XML document using SAX and generates a
 * tree representation of its content suitable for displaying in
 * a GUI
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class InstanceTreeBuilder extends DefaultHandler {

    /** Root node of the tree */
    private DefaultMutableTreeNode rootNode;

    /** Current position in tree */
    private DefaultMutableTreeNode current;

    /** Name of document */
    private String docName;

    /**
     *  Constructor: creates a new parser and parses the document given
     *  @param is input stream to read document from
     *  @param name name of document
     */
    public InstanceTreeBuilder(InputStream is, String name) {
	rootNode = null;
	current = null;
	docName = name;
	try {
	    SAXParserFactory spf = SAXParserFactory.newInstance();
	    SAXParser sp = spf.newSAXParser();
	    sp.parse(is, this);
	}
	catch (Exception e) {
	    System.err.println("Exception occurred: "+e);
	}
    }

    /** 
     *  Handles character data by appending it to an element node
     *  (preceded by an '=' sign)
     */
    public void characters(char[] ch, int start, int length) {
	String str = new String(ch, start, length);
	str = str.trim();
	if (!str.equals("")) {
	    String oldStr = (String)current.getUserObject();
	    String newStr = oldStr + " = "+str;
	    current.setUserObject(newStr);
	}
    }

    /**
     *  Handles end of document (no need to do anything)
     */
    public void endDocument() {

    }

    /**
     *  Handles start of document - creates the root node and makes it the
     *  current position
     */
    public void startDocument() {
	rootNode = new DefaultMutableTreeNode(docName);
	current = rootNode;
    }

    /**
     *  Handles end of an element - walks back up the tree to its parent
     */
    public void endElement(String nameSpaceURI, String name, String qname) {
	current = (DefaultMutableTreeNode)current.getParent();
    }

    /**
     *  Handles start of an element - creates a new node for it under the
     *  current tree position, containing its name and attributes
     */
    public void startElement(String nameSpaceURI, String name, String qname, Attributes atts) {
	String nodeText = qname;
	
	if (atts!=null) {
	    for (int i=0; i<atts.getLength(); i++) {
		nodeText += " "+atts.getQName(i)+"=\""+atts.getValue(i)+"\"";
	    }
	}

	DefaultMutableTreeNode newNode = new DefaultMutableTreeNode(nodeText);
	current.add(newNode);
	current = newNode;
    }

    /**
     *  @return the tree created
     */
    public DefaultMutableTreeNode getTree() {
	return rootNode;
    }
}
