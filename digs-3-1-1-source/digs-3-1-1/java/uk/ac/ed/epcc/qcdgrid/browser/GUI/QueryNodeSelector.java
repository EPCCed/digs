/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
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
import java.util.Vector;

import uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler.*;

/**
 * Title:        QueryNodeSelector
 * Description: The QueryNodeSelector is a dialog which displays a tree view
 * of the template document and allows the user to select a single node.
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */

public class QueryNodeSelector extends JDialog {
    /** GUI widgets - mostly generated by JBuilder */
    JPanel jPanel2 = new JPanel();
    JScrollPane jScrollPane1 = new JScrollPane();
    JTree jTree1 = new JTree();
    JFrame jParent;
    BorderLayout borderLayout1 = new BorderLayout();
    JPanel jPanel3 = new JPanel();
    JButton jPredicate = new JButton();
    JButton jCancel = new JButton();
    DefaultMutableTreeNode selectedNode;
    DefaultMutableTreeNode originalNode = null;

    String namespace;

    /** constructor - creates a new query node selector with an empty tree */
    public QueryNodeSelector() {
	try {
	    jTree1 = new JTree();
	    selectedNode = null;
	    jbInit();
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
    }

    /** constructor - creates a new query node selector
     *  @param t the tree to display in this node selector
     *  @param par the parent window
     *  @param namespace the namespace prefix to use
     */
    public QueryNodeSelector(JTree t, JFrame par, String namespace) {
	super(par);
	this.namespace = namespace;
	try {
	    jTree1 = t;
	    jParent = par;
	    jbInit();
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	pack();
    }

    /** constructor - creates a new query node selector for editing an existing
     *  query
     *  @param t the tree to display in this node selector
     *  @param par the parent window
     *  @param namespace the namespace prefix to use
     *  @param query the XPath of the existing query
     *  @param name the name of the existing query
     */
    public QueryNodeSelector(JTree t, JFrame par, String namespace, String query, String name) {
	this(t, par, namespace);

	queryName = name;

	/* Try to parse the given query into its node selecting part and its predicate.
	   This won't always be 100% successful, but it should work most of the time */
	int pstart = query.indexOf("[");
	int pend = query.indexOf("]");
	String path;

	if (pstart<0) {
	    predicate = null;
	    path = query;
	}
	else {
	    path = query.substring(0, pstart);
	    if (pend<0) {
		predicate = query.substring(pstart+1);
	    }
	    else {
		predicate = query.substring(pstart+1, pend);
	    }
	}

	/* Separated path and predicate. Now navigate tree according to path */
	DefaultMutableTreeNode node = (DefaultMutableTreeNode)t.getModel().getRoot();
	DefaultMutableTreeNode newnode;
	int pos=0;
	if (path.charAt(pos)=='/') {
	    pos++;
	}
	int nextSlash=path.indexOf('/', pos);
	if (nextSlash < 0) {
	    nextSlash = path.length();
	}
	do {
	    String nodeName = path.substring(pos, nextSlash);

	    newnode = null;
	    for (int i=0; i<node.getChildCount(); i++) {
		DefaultMutableTreeNode cnode = (DefaultMutableTreeNode)node.getChildAt(i);
		TemplateNode tn = (TemplateNode)cnode.getUserObject();
		if (nodeName.equals(tn.getNodeName())) {
		    newnode = cnode;
		}
	    }

	    if (newnode==null) {
		return;
	    }

	    node = newnode;
	    pos=nextSlash+1;
	    nextSlash=path.indexOf('/', pos+1);
	    if (nextSlash<0) {
		nextSlash=path.length();
	    }
	} while (pos<path.length());

	TreeNode[] treeNode = node.getPath();
	Object[] tnao = new Object[treeNode.length];
	for (int j=0; j<tnao.length; j++) {
	    tnao[j]=(Object)treeNode[j];
	}
	t.setExpandsSelectedPaths(true);
	t.setSelectionPath(new TreePath(tnao));

	originalNode = node;
    }

    /** creates the GUI widgets */
    private void jbInit() throws Exception {
	this.setTitle("Node Selector");
	jPanel2.setLayout(borderLayout1);
	jPredicate.setText("Set Predicate/Name");
	jPredicate.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jPredicate_actionPerformed(e);
		}
	    });
	jCancel.setText("Cancel");
	jCancel.addActionListener(new java.awt.event.ActionListener() {
		public void actionPerformed(ActionEvent e) {
		    jCancel_actionPerformed(e);
		}
	    });
	jTree1.addMouseListener(new java.awt.event.MouseAdapter() {
		public void mouseClicked(MouseEvent e) {
		    jTree1_mouseClicked(e);
		}
		public void mousePressed(MouseEvent e) {
		    jTree1_mousePressed(e);
		}
	    });
	jScrollPane1.setPreferredSize(new Dimension(300, 300));
	this.getContentPane().add(jPanel2, BorderLayout.SOUTH);
	jPanel2.add(jPanel3, BorderLayout.SOUTH);
	jPanel3.add(jPredicate, null);
	jPanel3.add(jCancel, null);
	this.getContentPane().add(jScrollPane1, BorderLayout.CENTER);
	jScrollPane1.getViewport().add(jTree1, null);
    }

    /** @return the selected node of the tree */
    public DefaultMutableTreeNode getSelectedNode() {
	return selectedNode;
    }

    /** cancels the node selection operation, destroys the dialogue */
    void cancel () {
	selectedNode = null;
	dispose();
    }
    
    /** the XPath predicate of this query */
    private String predicate = null;

    /** the name of this query */
    private String queryName = null;
    
    /** @return the XPath predicate of this query */
    public String getPredicate() {
	return predicate;
    }
    
    /** @return the name of this query */
    public String getQueryName() {
	return queryName;
    }
    
    /**
     *  @param node the selected node from the template tree
     *  @param showPredicateDialog indicates whether the user selected the
     *  "Set Predicate/Name" button or used the shortcut to leave the predicate
     *  empty
     */
    void ok (DefaultMutableTreeNode node, boolean showPredicateDialog) {
	selectedNode = node;
	if (showPredicateDialog) { // user wants to select a name or a predicate

	    if (selectedNode != originalNode) {
		/*
		 * If user changed node (when editing a query), wipe the predicate.
		 * It could be inappropriate and almost certainly won't be any use.
		 */
		predicate = null;
	    }

	    TemplateNode tn = (TemplateNode)node.getUserObject();

	    PredicateInputDialog pdlg = new PredicateInputDialog(tn,
								 "Input Predicate", namespace, true, predicate,
								 queryName);
	    pdlg.setModal(true);
	    pdlg.show();
	    predicate = pdlg.getPredicate();
	    queryName = pdlg.getQueryName();
	    // if the user cancelled the dialog
	    if (predicate == null ) selectedNode = null;
	}
	else { // use empty predicate and default name
	    predicate = "";
	    queryName = null;
	}
	dispose();
    }
    
    /** action event handlers */
    void jPredicate_actionPerformed(ActionEvent e) {
	DefaultMutableTreeNode node =
            (DefaultMutableTreeNode)jTree1.getLastSelectedPathComponent();
	if (node == null) cancel();
	else ok(node, true);
    }
    
    void jCancel_actionPerformed(ActionEvent e) {
	cancel();
    }
    
    /** handles the shortcut for selecting a node with the default name
     *  and an empty predicate
     */
    void jTree1_mouseClicked(MouseEvent e) {
	int selRow = jTree1.getRowForLocation(e.getX(),e.getY());
	if (selRow > 0) {
	    if (e.getModifiers() == MouseEvent.BUTTON3_MASK) {
		jTree1.setSelectionRow(selRow);
		ok((DefaultMutableTreeNode)jTree1.getLastSelectedPathComponent(), false);
	    }
	}
    }
    
    /** react to mouse button 3 pressed by selecting the node */
    void jTree1_mousePressed(MouseEvent e) {
	int selRow = jTree1.getRowForLocation(e.getX(),e.getY());
	if (selRow > 0) {
	    if (e.getModifiers() == MouseEvent.BUTTON3_MASK) {
		jTree1.setSelectionRow(selRow);
	    }
	}
    }
}
