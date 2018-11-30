/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.QueryHandler;

import java.util.*;
import javax.swing.tree.*;

/**
 * Title:        AtomicXPathQueryExpression
 * Description: An AtomicXPathQueryExpression contains a node of the template
 * tree and a corresponding predicate or a string with an arbitrary query.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class AtomicXPathQueryExpression {

    /** a node of the template tree */
    private DefaultMutableTreeNode path;

    /** the corresponding predicate string */
    private String predicate;

    /** an arbitrary query expression
     * If this string is not null then the parameters path and predicate are ignored.
     */
    private String anyQuery;
    
    public AtomicXPathQueryExpression() {
    }

    /** Constructs an AtomicXPathQueryExpression from a tree node and a predicate
     *  string.
     *  @param node the selected node of the template tree
     *  @param pred a predicate string for this node
     */
    public AtomicXPathQueryExpression(DefaultMutableTreeNode node, String pred) {
	path = node;
	predicate = pred;
	anyQuery=null;
    }
    
    /** Constructs an AtomicXPathQueryExpression from an arbitrary query string
     * @param q an arbitrary query string
     */
    public AtomicXPathQueryExpression(String q) {
	anyQuery = q;
    }
    
    /** Sets the predicate of the expression
     * @param pred the new XPath predicate
     */
    public void setPredicate(String pred) {
	predicate = pred;
    }

    /**
     * Optimises the XPath as much as it can
     * @param q the XPath query string to optimise
     * @return the optimised query
     */
    private String optimise(String q) {
	String result = null;

	/*
	 * Replace "/a/b/c[string(.) = 'value']" with "/a/b[c = 'value']" which
	 * is faster, particularly on the LDG metadata catalogue
	 */
	int sidx = q.indexOf("[string(.) = ");
	if (sidx > 0)
	{
	    // found "[string(.) = "
	    // look for the '/' before it
	    int slashidx = q.lastIndexOf('/', sidx);
	    if (slashidx > 1)
	    {
		// found the slash, it's not at start of query
		int endquoteidx = -1;
		String quotetype;

		if (q.charAt(sidx + 13) == '\'')
		{
		    endquoteidx = q.indexOf('\'', sidx + 14);
		    quotetype = "'";
		}
		else if (q.charAt(sidx+13) == '"')
		{
		    endquoteidx = q.indexOf('"', sidx + 14);
		    quotetype = "\"";
		}

		if (endquoteidx >= 0)
		{
		    if (q.charAt(endquoteidx + 1) == ']')
		    {
			// Now we've verified that it's of the right form, and
			// we have all the information we need
			result = q.substring(0, slashidx) + "[" +
			    q.substring(slashidx+1, sidx) + q.substring(sidx + 10);
			System.out.println("Optimised query is " + result);
		    }
		}
	    }
	}

	if (result == null) return q;
	return result;
    }
    
    /**
     * @return a string representation of this object (its XPath in the default
     * namespace)
     */
    public String toString() {
	return toXPathString();
    }

    /**
     * @return this object's XPath in the default namespace
     */
    public String toXPathString() {
	return toXPathString("");
    }

    /** construct a string from the path and the predicate string, with a
     * namespace prefix
     * @param namespace the namespace prefix to use on elements
     * @return the XPath query
     */
    public String toXPathString(String namespace) {

	if (anyQuery != null) return anyQuery;

	/* first get the path from the root to the node */
	Object[] pathToRoot = path.getUserObjectPath();
	String result = "";
	for (int i=1; i<pathToRoot.length; i++) {
	    if ((namespace==null)||(namespace.equals(""))) {
		result += "/" + pathToRoot[i].toString();
	    }
	    else {
		result += "/"+namespace+":" + pathToRoot[i].toString();
	    }
	}
	/* attach the predicate string to the path, enclosed in [] */
	if (predicate != null && predicate.length() > 0) {
	    result = result + "[" + predicate + "]";
	}
	return optimise(result);
    }

    /**
     *  Replaces symbols in a query string which may interfere with XML parsing
     *  with their entity reference equivalents. Used when the query is serialised
     *  to disk as XML in the config file
     *  @param str the query string to quote
     *  @return a quoted version of the string
     */
    private String quoteSymbols(String str) {
	int i;

	i=str.indexOf('&');
	while (i>=0) {
	    str = str.substring(0, i) + "&amp;" + str.substring(i+1);
	    i=str.indexOf('&', i+1);
	}

	i=str.indexOf('>');
	while (i>=0) {
	    str = str.substring(0, i) + "&gt;" + str.substring(i+1);
	    i=str.indexOf('>');
	}

	i=str.indexOf('<');
	while (i>=0) {
	    str = str.substring(0, i) + "&lt;" + str.substring(i+1);
	    i=str.indexOf('<');
	}

	i=str.indexOf('"');
	while (i>=0) {
	    str = str.substring(0, i) + "&quot;" + str.substring(i+1);
	    i=str.indexOf('"');
	}

	i=str.indexOf('\'');
	while (i>=0) {
	    str = str.substring(0, i) + "&apos;" + str.substring(i+1);
	    i=str.indexOf('\'');
	}

	return str;
    }

    /**
     * @return an XML representation of the atomic query suitable for saving
     */
    public String toXML() {
	String xml;

	if (anyQuery != null) {
	    xml = "<atomic_query><xpath>"+quoteSymbols(anyQuery)+"</xpath></atomic_query>";
	}
	else {
	    Object[] pathToRoot = path.getUserObjectPath();
	    String result = "";
	    for (int i=1; i<pathToRoot.length; i++) {
		result += "/"+pathToRoot[i].toString();
	    }

	    xml = "<atomic_query><node>"+result+"</node><predicate>"+quoteSymbols(predicate)+"</predicate></atomic_query>";
	}

	return xml;
    }

    public boolean isPlainText() {
	if (anyQuery != null) {
	    return true;
	}
	return false;
    }
}
