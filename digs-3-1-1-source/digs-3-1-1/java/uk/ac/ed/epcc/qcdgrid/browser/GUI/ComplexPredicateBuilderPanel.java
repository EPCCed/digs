/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.tree.*;

import uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler.*;

/**
 * Title:       ComplexPredicateBuilderPanel
 * Description: GUI panel for generating predicates to operate on
 * complex type nodes
 * Copyright:    Copyright (c) 2003
 * Company:      The University of Edinburgh
 * @author James Perry
 * @version 1.0
 */
public class ComplexPredicateBuilderPanel extends PredicateBuilderPanel implements ActionListener {

    /** the button for setting a "sub-predicate" */
    private JButton setSubPredicate;

    /** the tree of sub nodes of this node */
    private JTree actualTree;

    /** the scroll pane that holds the tree */
    private JScrollPane scrollPane;

    /** the predicate of the selected sub node */
    private String subPredicate;

    /** adapter class for handling mouse events */
    private MouseAdapter mouseAdapter;

    /** path to node of tree currently selected */
    private TreePath selectedNode;

    /** namespace currently set */
    private String namespace;

    /**
     *  Constructor
     *  @param treeRoot the root node for the tree of nodes below this node
     */
    public ComplexPredicateBuilderPanel(DefaultMutableTreeNode treeRoot) {
	super();

	setSubPredicate = new JButton("Set Sub-Predicate");
	setSubPredicate.addActionListener(this);
	add(setSubPredicate, BorderLayout.SOUTH);

	actualTree = new JTree(treeRoot);
	scrollPane = new JScrollPane(actualTree, ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
					  ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
	scrollPane.setPreferredSize(new Dimension(200,100));
	add(scrollPane, BorderLayout.CENTER);

	subPredicate = null;

 	mouseAdapter = new MouseAdapter() {
		public void mouseClicked(MouseEvent me) {
		    mouseWasClicked(me);
		}
	    };

	actualTree.addMouseListener(mouseAdapter);

	selectedNode = null;
	subPredicate = null;
    }

    /**
     *  Event handler for mouse clicking on tree
     *  @param me mouse event to handle
     */
    public void mouseWasClicked(MouseEvent me) {
	TreePath tn;
	Object source = me.getSource();

	if (source==(Object)actualTree) {
	    tn = actualTree.getPathForLocation(me.getX(), me.getY());
	    if (tn!=null) {
		selectedNode = tn;
	    }
	}
    }    

    /**
     *  sets the namespace used by this predicate builder
     *  @param namespace the namespace prefix to use
     */
    public void setNamespace(String ns) {
	namespace = ns;
    }

    /**
     *  @return the XPath of this predicate input
     */
    public String getXPath() {
	String xpath = "";
       	Object[] path = selectedNode.getPath();

	/*
	 * Start at 1 to skip the root node
	 */
	for (int i=1; i<path.length; i++) {
	    if ((namespace!=null)&&(!namespace.equals(""))) {
		xpath+=namespace+":"+((DefaultMutableTreeNode)path[i]).getUserObject().toString();
	    }
	    else {
		xpath+=((DefaultMutableTreeNode)path[i]).getUserObject().toString();
	    }
	    if (i!=(path.length-1)) {
		xpath+="/";
	    }
	}

	if (subPredicate!=null) {
	    xpath+="["+subPredicate+"]";
	}

	return xpath;
    }

    /**
     *  disables all the GUI elements on this panel
     */
    public void disable() {
	super.disable();
	setSubPredicate.setEnabled(false);
	actualTree.setEnabled(false);
    }

    /**
     *  enables all the GUI elements on this panel
     */
    public void enable() {
	super.enable();
	setSubPredicate.setEnabled(true);
	actualTree.setEnabled(true);
    }

    /**
     *  Action event handler for the set sub predicate button
     *  @param e the action event to handle
     */
    public void actionPerformed(ActionEvent e) {
	Object source = e.getSource();

	if (source==(Object)setSubPredicate) {

	    if (selectedNode!=null) {

		/* Spawn a child instance of this dialogue to get the sub predicate */
		Object[] path = selectedNode.getPath();
		TemplateNode tn = (TemplateNode)((DefaultMutableTreeNode)path[path.length-1]).getUserObject();

		PredicateInputDialog pdlg = new PredicateInputDialog(tn, "Input Predicate for "+tn, namespace,
								     false, subPredicate, null);
		pdlg.setModal(true);
		pdlg.show();
		subPredicate = pdlg.getPredicate();
	    }
	}
    }

    /**
     *  Sets the predicate in this dialogue. Used for editing an existing predicate
     *  @param pred the XPath predicate to edit
     *  @return true if the predicate was set successfully, false if not
     */
    public boolean setPredicate(String pred) {
	/* Try to parse the given query into its node selecting part and its predicate.
	   This won't always be 100% successful, but it should work most of the time */
	int pstart = pred.indexOf("[");
	int pend = pred.indexOf("]");
	String path;

	if (pstart<0) {
	    subPredicate = null;
	    path = pred;
	}
	else {
	    path = pred.substring(0, pstart);
	    if (pend<0) {
		subPredicate = pred.substring(pstart+1);
	    }
	    else {
		subPredicate = pred.substring(pstart+1, pend);
	    }
	}

//	System.out.println("Path: "+path+", predicate: "+subPredicate);

	/* Separated path and predicate. Now navigate tree according to path */
	DefaultMutableTreeNode node = (DefaultMutableTreeNode)actualTree.getModel().getRoot();
	DefaultMutableTreeNode newnode;
	int pos=0;
	if (path.charAt(pos)=='/') {
	    pos++;
	}
	int nextSlash=path.indexOf('/', pos);
	int numPathComponents = 0;
	do {
	    String nodeName = path.substring(pos, nextSlash);

	    numPathComponents++;

//	    System.out.println(nodeName);

	    newnode = null;
	    for (int i=0; i<node.getChildCount(); i++) {
		DefaultMutableTreeNode cnode = (DefaultMutableTreeNode)node.getChildAt(i);
		TemplateNode tn = (TemplateNode)cnode.getUserObject();
		if (nodeName.equals(tn.getNodeName())) {
		    newnode = cnode;
		}
	    }

	    if (newnode==null) {
		return false;
	    }

	    node = newnode;
	    pos=nextSlash+1;
	    nextSlash=path.indexOf('/', pos+1);
	    if (nextSlash<0) {
		nextSlash=path.length();
	    }
	} while (pos<path.length());

	TreeNode[] treeNode = node.getPath();

	/*
	 * Why don't we convert the entire path as for the top level node selection tree?
	 * Well, this tree uses the same node objects as that one, so the path returned
	 * will be relative to the tree's top level root, not to the root that's
	 * displayed. So need to use just the last few components of the path.
	 */
	Object[] tnao = new Object[numPathComponents+1];
	for (int j=0; j<tnao.length; j++) {
	    tnao[j]=(Object)treeNode[j+(treeNode.length-(numPathComponents+1))];
	}
	actualTree.setExpandsSelectedPaths(true);
	selectedNode = new TreePath(tnao);
	actualTree.setSelectionPath(selectedNode);

	return true;
    }
}
