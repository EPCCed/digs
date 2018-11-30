/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import java.io.ByteArrayInputStream;

import javax.swing.*;
import javax.swing.tree.*;

/**
 * Title:       SAXTree
 * Description: The SAXTree reads an XML schema from a string and constructs
 * a tree representation. This class has been heavily butchered as most of
 * the work is now done in SchemaTreeBuilder. And it has nothing to do with
 * SAX any more, either
 * Copyright:    Copyright (c) 2002
 * Company:      The University of Edinburgh
 * @author Amy Krause
 * @version 1.0
 */
public class SAXTree {

    /** The base tree to render */
    private JTree jTree;

    /** Tree model to use */
    DefaultTreeModel defaultTreeModel;

    /**
     * constructor: creates a new SAXTree of a schema
     * @param schemaDocument the XML document to build a tree from
     */
    public SAXTree(String schemaDocument) {

	byte[] schemaAsBytes = schemaDocument.getBytes();
	SchemaTreeBuilder stb = new SchemaTreeBuilder(new ByteArrayInputStream(schemaAsBytes));

	/* FIXME: how can we get it to know that gauge_configuration should be at the root of
	 * the tree, without being told explicitly? */
//	DefaultMutableTreeNode rootNode = stb.buildTree("gauge_configuration");
	DefaultMutableTreeNode rootNode = stb.buildTree("markovChain");
	
	DefaultMutableTreeNode base =
	    new DefaultMutableTreeNode("Schema Document");
	
	base.add(rootNode);

	// Build the tree model
	defaultTreeModel = new DefaultTreeModel(base);
	jTree = new JTree(defaultTreeModel);
	DefaultTreeSelectionModel s = new DefaultTreeSelectionModel();
	s.setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
	jTree.setSelectionModel(s);
    }

    /**
     * @return the JTree constructed from the document
     */
    public JTree getJTree() {
	return jTree;
    }
    
    /**
     * @return the name of the root element of the tree
     */
    public String getRootName() {
	TreeModel jTreeModel = jTree.getModel();
	return jTreeModel.getChild(jTreeModel.getRoot(), 0).toString();
    }
}
