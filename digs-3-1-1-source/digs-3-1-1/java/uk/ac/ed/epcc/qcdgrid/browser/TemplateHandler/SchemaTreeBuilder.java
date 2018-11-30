/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.TemplateHandler;

import javax.swing.tree.*;
import java.io.*;
import java.net.*;
import java.util.Vector;

import javax.xml.parsers.*;
import org.xml.sax.SAXException;
import org.w3c.dom.*;

/**
 * Title: SchemaTreeBuilder Description: Parses the schema document, generating
 * a tree representation of the structure of an instance document Copyright:
 * Copyright (c) 2002 Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class SchemaTreeBuilder {

  /** Vector of all the schema DOMs loaded */
  private Vector schemaDoms;

  /** Vector of namespace prefixes corresponding to each schema */
  private Vector schemaPrefixes;

  /**
   * Parses one schema from a disk file. Stores its DOM in the schemaDoms
   * vector, and automatically takes care of loading any imported schemas
   * 
   * @param is
   *          the input stream giving access to the schema
   */
  private void parseSchema(InputStream is) {
    try {

      /* Do the parsing */
      DocumentBuilderFactory docbuildfact = DocumentBuilderFactory
          .newInstance();
      DocumentBuilder docbuild = docbuildfact.newDocumentBuilder();
      Document dom = docbuild.parse(is);
      schemaDoms.add(dom);

      Element root = dom.getDocumentElement();

     // System.out.println("Done parsing");

      /* Check it's a schema */
      if (!root.getNodeName().endsWith(":schema")) {
        throw new Exception("Root node of schema is not schema, it's "
            + root.getNodeName());
      }

      /* Get its namespace */
      String namespace;
      int colon = root.getNodeName().indexOf(':');
      namespace = root.getNodeName().substring(0, colon + 1);

      schemaPrefixes.add(namespace);
      schemaPrefix = namespace;

      //System.out.println("Determined namespace");

      /* Now look for includes */
      NodeList nl = dom.getElementsByTagName(schemaPrefix + "import");
      for (int i = 0; i < nl.getLength(); i++) {

        //System.out.println("Handling import number " + i);

        Node node = nl.item(i);
        Element el = (Element) node;
        String url = el.getAttribute("schemaLocation");
        if ((url != null) && (!url.equals(""))) {

          /* Load the imported schema from its location */
          URL importedSchema = new URL(url);
          URLConnection urlConn;

          //System.out.println("Determined url as " + url);

          try {
            urlConn = importedSchema.openConnection();
          } catch (Exception e) {
            throw new Exception("Can't open URL " + url + " due to " + e);
          }

          /* try to parse the imported schema */
          parseSchema(urlConn.getInputStream());
        }
      }
    } catch (Exception e) {
      System.err.println("Exception occurred parsing schema: " + e);
    }
  }

  /**
   * Constructor: parses the schema file into a DOM tree
   * 
   * @param is
   *          the input stream providing the schema
   */
  public SchemaTreeBuilder(InputStream is) {

    /* initialise member variables */
    schemaDoms = new Vector();
    schemaPrefixes = new Vector();
    substitutionCache = new Vector();
    substitutionIndex = new Vector();

    tree = null;
    position = null;

    /* Parse the first schema. This will recursively parse any dependencies */
    try {
      parseSchema(is);
    } catch (Exception e) {
      System.err.println("Exception:" + e);
    }
  }

  /**
   * The namespace prefix being used for schema elements (usually xs:, xsd: or
   * similar). Now that multiple schemas are supported, this refers to the
   * prefix of the one currently being processed
   */
  private String schemaPrefix;

  /** Pointer to the root of the tree being generated */
  private DefaultMutableTreeNode tree;

  /** Pointer to the current position in the tree being generated */
  private DefaultMutableTreeNode position;

  /**
   * Clones a tree node AND all its children (the standard clone function does a
   * shallow clone which does not clone the children)
   * 
   * @param in
   *          the node to clone
   * @return a copy of the node and all its children
   */
  private DefaultMutableTreeNode deepClone(DefaultMutableTreeNode in) {

    DefaultMutableTreeNode out = (DefaultMutableTreeNode) in.clone();

    for (int i = 0; i < in.getChildCount(); i++) {
      out.add(deepClone((DefaultMutableTreeNode) in.getChildAt(i)));
    }
    return out;
  }

  /**
   * The substitution cache is used to hold references to all the subtrees so
   * far parsed, indexed in the other vector by element name. On complicated
   * schemas, this can speed things up by orders of magnitude
   */
  private Vector substitutionCache;

  private Vector substitutionIndex;

  /**
   * Gets the index of an element in the substitution cache
   * 
   * @param el
   *          the element to look for
   * @return the index of the element, or -1 if it's not there
   */
  private int subCacheIndex(Element el) {

    for (int i = 0; i < substitutionIndex.size(); i++) {
      if (el == (Element) substitutionIndex.elementAt(i)) {
        return i;
      }
    }

    return -1;
  }

  /**
   * Finds the type definition element for a named type in the schema
   * 
   * @param typeName
   *          the name of the type to look for
   * @return the node of the type definition, or null if none found
   */
  private Node findTypeDefinition(String typeName) {

    Node typeNode = null;

    for (int j = 0; j < schemaDoms.size(); j++) {

      Document dom = (Document) schemaDoms.elementAt(j);
      String prefix = (String) schemaPrefixes.elementAt(j);

      NodeList nl = dom.getElementsByTagName(prefix + "simpleType");

      for (int i = 0; i < nl.getLength(); i++) {

        Node node = nl.item(i);

        Element el = (Element) node;

        if (el.getAttribute("name").equals(typeName)) {
          schemaPrefix = prefix;
          typeNode = node;
        }
      }

      nl = dom.getElementsByTagName(prefix + "complexType");

      for (int i = 0; i < nl.getLength(); i++) {

        Node node = nl.item(i);

        Element el = (Element) node;

        if (el.getAttribute("name").equals(typeName)) {
          schemaPrefix = prefix;
          typeNode = node;
        }
      }
    }
    return typeNode;
  }

  /**
   * Finds the definition of an element, given its name
   * 
   * @param name
   *          the name of the element definition to find
   * @return the node of this element definition
   */
  private Node findElementDefinition(String name) {

    Node elNode = null;

    for (int j = 0; j < schemaDoms.size(); j++) {

      Document dom = (Document) schemaDoms.elementAt(j);
      String prefix = (String) schemaPrefixes.elementAt(j);

      NodeList nl = dom.getElementsByTagName(prefix + "element");

      for (int i = 0; i < nl.getLength(); i++) {

        Node node = nl.item(i);

        Element el = (Element) node;

        if (el.getAttribute("name").equals(name)) {
          elNode = node;
          schemaPrefix = prefix;
        }
      }
    }

    return elNode;
  }

  /**
   * Finds the definition of a model group, given its name
   * 
   * @param name
   *          the name of the group definition to find
   * @return the node of this group definition
   */
  private Node findGroupDefinition(String name) {

    Node elNode = null;

    for (int j = 0; j < schemaDoms.size(); j++) {

      Document dom = (Document) schemaDoms.elementAt(j);
      String prefix = (String) schemaPrefixes.elementAt(j);

      NodeList nl = dom.getElementsByTagName(prefix + "group");

      for (int i = 0; i < nl.getLength(); i++) {

        Node node = nl.item(i);

        Element el = (Element) node;

        if (el.getAttribute("name").equals(name)) {
          elNode = node;
          schemaPrefix = prefix;
        }
      }
    }

    return elNode;
  }

  /**
   * Finds the base type name of a derived simple type. Calls itself recursively
   * until it reaches a built in XPath type at the top of the type hierarchy
   * 
   * @param node
   *          the DOM node of the simple type definition
   * @return the name of the base type this is derived from
   */
  private String findSimpleTypeBase(Node node) {
    NodeList nl = node.getChildNodes();
    String result = null;

    for (int i = 0; i < nl.getLength(); i++) {
      Node child = nl.item(i);

      /* Should contain a restriction node with a 'base' attribute */
      if (child.getNodeName().equals(schemaPrefix + "restriction")) {
        Element restrictionElement = (Element) child;
        String base = restrictionElement.getAttribute("base");

        if ((base != null) && (!base.equals(""))) {

          /* Find the base type definition */
          Node superType = findTypeDefinition(base);
          if (superType == null) {
            /*
             * No type definition - found the built in XPath type
             */
            result = base;
          } else {
            /*
             * Found a type definition - call self recursively to deal with it
             */
            result = findSimpleTypeBase(superType);
          }
        }
      }
    }

    return result;
  }

  /**
   * @return true if the given type name represents a numeric XPath simple type
   */
  private boolean isNumericType(String typeName) {
    if ((typeName.equals(schemaPrefix + "float"))
        || (typeName.equals(schemaPrefix + "double"))
        || (typeName.equals(schemaPrefix + "decimal"))
        || (typeName.equals(schemaPrefix + "byte"))
        || (typeName.equals(schemaPrefix + "unsignedByte"))
        || (typeName.equals(schemaPrefix + "integer"))
        || (typeName.equals(schemaPrefix + "positiveInteger"))
        || (typeName.equals(schemaPrefix + "negativeInteger"))
        || (typeName.equals(schemaPrefix + "nonNegativeInteger"))
        || (typeName.equals(schemaPrefix + "nonPositiveInteger"))
        || (typeName.equals(schemaPrefix + "int"))
        || (typeName.equals(schemaPrefix + "unsignedInt"))
        || (typeName.equals(schemaPrefix + "long"))
        || (typeName.equals(schemaPrefix + "unsignedLong"))
        || (typeName.equals(schemaPrefix + "short"))
        || (typeName.equals(schemaPrefix + "unsignedShort"))) {
      return true;
    }
    return false;
  }

  /**
   * Given the node of a type definition, creates a template node for that type
   * 
   * @param node
   *          the DOM node of the type definition
   * @param nodeName
   *          the name of the element instance of this type
   * @return a template node for the type
   */
  private TemplateNode templateFromTypeNode(Node node, String nodeName) {
    TemplateNode result = null;

    /* Check if it's complex or simple */
    if (node.getNodeName().equals(schemaPrefix + "complexType")) {

      /*
       * For once, complex types are easier to deal with. Just create a complex
       * template node for it
       */
      result = new ComplexTemplateNode(nodeName, "complex");
    } else {

      /*
       * If it's a simple type, we have to work our way up through the type
       * hierarchy to find what it's ultimately derived from
       */
      String oldPrefix = schemaPrefix;
      String base = findSimpleTypeBase(node);
      schemaPrefix = oldPrefix;

      /* Then create a template node for the base type */
      result = templateFromTypeName(base, nodeName);
    }

    return result;
  }

  /**
   * Given a type name, creates a template node for that type
   * 
   * @param typeName
   *          the name of the type
   * @param nodeName
   *          the name of the instance element
   * @return a template for this type
   */
  private TemplateNode templateFromTypeName(String typeName, String nodeName) {

    String oldPrefix = schemaPrefix;
    TemplateNode result = null;

    /* See if it's a user defined type */
    Node typedefNode = findTypeDefinition(typeName);
    if (typedefNode != null) {

      /* If so, process the type definition */
      result = templateFromTypeNode(typedefNode, nodeName);
    } else {

      /* It's a built in simple type - see if it's numeric or textual */
      if (isNumericType(typeName)) {
        result = new NumberTemplateNode(nodeName, typeName);
      } else {
        result = new StringTemplateNode(nodeName, typeName);
      }
    }

    schemaPrefix = oldPrefix;
    return result;
  }

  /**
   * Takes care of generating all the tree nodes for an element definition
   * 
   * @param element
   *          the element definition in the schema
   */
  private void processElement(Node element) throws Exception {

    //System.out.println("Entering processElement");

    String oldPrefix = schemaPrefix;
    Element el = (Element) element;

    /* get name and check that it has one */
    String elname = el.getAttribute("name");
    if ((elname == null) || (elname.equals(""))) {

    //  System.out.println("Element has no name");

      /* no name - see if it's a reference */
      String elref = el.getAttribute("ref");
      if ((elref == null) || (elref.equals(""))) {
        throw new Exception("Unnamed element in schema");
      }

      Node elementRef = findElementDefinition(elref);
      processElement(elementRef);

      schemaPrefix = oldPrefix;
      return;
    }

    // System.out.println("Element name: " + elname);

    /*
     * See if we've already done an element like this. If so, we can clone it
     * from the reference in the cache, which is *MUCH* faster than generating
     * its subtree again
     */
    int sci = subCacheIndex(el);
    if (sci >= 0) {
      // System.out.println("Using cached entry");

      DefaultMutableTreeNode newNode = deepClone((DefaultMutableTreeNode) substitutionCache
          .elementAt(sci));
      position.add(newNode);
    } else {

      // System.out.println("Building element entry");

      /* Work out element type */
      String eltype = el.getAttribute("type");
      if ((eltype == null) || (eltype.equals(""))) {

        /*
         * A bit of a misnomer. The type isn't necessarily complex, it could be
         * a derived simple type
         */
        eltype = "complex";
      }

      // System.out.println("Element type: " + eltype);

      /* Get template node for this element */
      TemplateNode tn;

      if (eltype.equals("complex")) {

        // System.out.println("Getting type definition node for complex...");

        /*
         * No type attribute. The type definition should be nested inside the
         * element tags...
         */
        NodeList nl = el.getChildNodes();
        Node typeNode = null;

        for (int i = 0; i < nl.getLength(); i++) {
          Node node = nl.item(i);

          if ((node.getNodeName().equals(schemaPrefix + "simpleType"))
              || (node.getNodeName().equals(schemaPrefix + "complexType"))) {

            typeNode = node;
          }
        }

        if (typeNode == null) {
          // System.out.println("No type node found - falling back to 'string'");
          tn = templateFromTypeName(schemaPrefix + "string", elname);
        } else {
          tn = templateFromTypeNode(typeNode, elname);
        }
      } else {

        // System.out.println("Getting type definition node for named type...");

        tn = templateFromTypeName(eltype, elname);
      }

      // System.out.println("Got type definition node");

      /* Add the element to the tree and go down to it */
      DefaultMutableTreeNode newNode = new DefaultMutableTreeNode(tn);
      DefaultMutableTreeNode parent = position;

      parent.add(newNode);
      position = newNode;

      // System.out.println("Added node to tree");

      /* There should be a more elegant way to do this */
      if (tn instanceof ComplexTemplateNode) {

        // System.out.println("Setting node reference for complex type");

        /*
         * If the template is complex, we need to give it a reference to the
         * actual node of the element it's representing
         */
        ComplexTemplateNode ctn = (ComplexTemplateNode) tn;
        ctn.setNode(position);
      }

      /* Add any sub-elements, by processing this element's type */
      processElementType(el);

      /* Go back up the tree to where we were */
      position = parent;

      /* Add this element to the substitution cache for possible use later */
      substitutionCache.add(newNode);
      substitutionIndex.add(el);

      /*
       * Look for any possible substitutions for this element - need to add them
       * to the tree here as well
       */
      for (int j = 0; j < schemaDoms.size(); j++) {

        Document dom = (Document) schemaDoms.elementAt(j);
        String prefix = (String) schemaPrefixes.elementAt(j);

        NodeList nl = dom.getElementsByTagName(prefix + "element");
        for (int i = 0; i < nl.getLength(); i++) {
          Node node = nl.item(i);
          Element el2 = (Element) node;
          String subgroup = el2.getAttribute("substitutionGroup");
          if ((subgroup != null) && (subgroup.equals(elname))) {
            schemaPrefix = prefix;
            processElement(el2);
          }
        }
      }
    }
    schemaPrefix = oldPrefix;
  }

  /**
   * Processes a sub-type of a complex type (an extension or restriction).
   * 
   * @param subtype
   *          the node of the schema defining the subtype
   */
  private void processSubtype(Node subtype) throws Exception {

    String oldPrefix = schemaPrefix;
    /*
     * Iterate over all nodes within the subtype (I think there should only be
     * one...)
     */
    NodeList nl = subtype.getChildNodes();
    for (int i = 0; i < nl.getLength(); i++) {
      Node node = nl.item(i);

      /* Handle an extension of an existing type */
      if (node.getNodeName().equals(schemaPrefix + "extension")) {
        Element ext = (Element) node;

        /* Get the base type of this extension */
        String base = ext.getAttribute("base");

        if ((base == null) || (base.equals(""))) {
          throw new Exception("Extension with no base type");
        }

        Node baseType = findTypeDefinition(base);

        if (baseType == null) {
          throw new Exception("Extension of non-existent type " + base);
        }

        /* Add elements of base type to tree */
        processType(baseType);

        /* Add extended elements added by this type to tree */
        processType(node);
      } else if (node.getNodeName().equals(schemaPrefix + "restriction")) {

        /*
         * For restriction types, all necessary info from parent is repeated
         * here, so we don't need to reference the base type
         */
        processType(node);

        /*
         * Old code: doesn't handle restrictions properly, it used to treat them
         * as their parent type
         */
        /*
         * Element ext = (Element)node;
         *  // Get the base type of this extension String base =
         * ext.getAttribute("base");
         * 
         * if ((base==null)||(base.equals(""))) { throw new
         * Exception("Restriction with no base type"); }
         * 
         * Node baseType = findTypeDefinition(base);
         * 
         * if (baseType==null) { throw new Exception("Restriction of
         * non-existent type "+base); }
         *  // Add elements of base type to tree processType(baseType);
         */
      }
    }

    schemaPrefix = oldPrefix;
  }

  /**
   * Takes care of adding all the possible nodes of the given (complex) type to
   * the tree
   * 
   * @param type
   *          the node of the complex type definition
   */
  private void processType(Node type) throws Exception {

    // System.out.println("Entering processType on " + type.getNodeName());

    String oldPrefix = schemaPrefix;
    NodeList nl = type.getChildNodes();

    // System.out.println("Got child node list for type");

    for (int i = 0; i < nl.getLength(); i++) {
      Node node = nl.item(i);

      // System.out.println("Processing node " + i);

      /*
       * By a happy coincidence, when building a template tree, we can treat
       * sequence, choice and all as operating in the same way - just add one
       * element of each type, in order
       */
      if ((node.getNodeName().equals(schemaPrefix + "sequence"))
          || (node.getNodeName().equals(schemaPrefix + "choice"))
          || (node.getNodeName().equals(schemaPrefix + "all"))) {
        /*
         * For sequence, choice or all, call recursively to deal with any child
         * nodes
         */

        // System.out.println("Found sequence/choice/all");

        processType(node);
      } else if (node.getNodeName().equals(schemaPrefix + "element")) {
        /* Process an actual element definition */

        // System.out.println("Found element");

        processElement(node);
      } else if (node.getNodeName().equals(schemaPrefix + "complexContent")) {
        /* Process a derived type */

        // System.out.println("Found complexContent");

        processSubtype(node);
      } else if (node.getNodeName().equals(schemaPrefix + "group")) {
        /* Process a reusable model group */

        // System.out.println("Found group");

        Element el = (Element) node;
        String groupName = el.getAttribute("ref");
        if ((groupName == null) || (groupName.equals(""))) {
          throw new Exception("Group without ref attribute");
        }
        Node groupNode = findGroupDefinition(groupName);
        processType(groupNode);
      }

      // System.out.println("Done node " + i);
    }

    schemaPrefix = oldPrefix;
  }

  /**
   * @param typeName
   *          the name of the type to check
   * @return true if the type name given refers to a built in type
   */
  private boolean isBuiltInType(String typeName) {
    /*
     * Assume a built-in type if it's not defined in the schem anywhere
     */
    if (findTypeDefinition(typeName) == null) {
      return true;
    }
    return false;
  }

  /**
   * Determines the type of the given element and takes care of any further
   * processing required by that type.
   * 
   * @param el
   *          the element definition in the schema
   */
  private void processElementType(Element el) throws Exception {

    String oldPrefix = schemaPrefix;
    Node typeNode = null;

    // System.out.println("Entering processElementType on " + el.getTagName());

    /* See if it has a type attribute */
    String typeName = el.getAttribute("type");

    // System.out.println("Got element type attribute");

    if ((typeName == null) || (typeName.equals(""))) {

      /*
       * No type attribute. The type definition should be nested inside the
       * element tags...
       */
      // System.out.println("No type attribute, looking for nested type");

      NodeList nl = el.getChildNodes();

      // System.out.println("Got element child nodes list");
      if (nl == null) {
        // System.out.println("Element child nodes list is null");
      }

      for (int i = 0; i < nl.getLength(); i++) {
        Node node = nl.item(i);

        // System.out.println("Got item " + i);

        if ((node.getNodeName().equals(schemaPrefix + "simpleType"))
            || (node.getNodeName().equals(schemaPrefix + "complexType"))) {

          typeNode = node;
        }
      }
    } else {

      // System.out.println("Element type is " + typeName);

      /*
       * Built in types don't have any child nodes or anything, so bail out now
       */
      if (isBuiltInType(typeName))
        return;

      /* Find the type definition for this type */
      typeNode = findTypeDefinition(typeName);
    }

    // System.out.println("Exited type finding code");

    if (typeNode == null) {
      /* Allow no type node for now... */
      return;
      // throw new Exception("No type found for element "+el.getNodeName());
    }

    /*
     * For simple types this element will become a leaf node, so don't do
     * anything more here
     */
    if (typeNode.getNodeName().equals(schemaPrefix + "complexType")) {
      processType(typeNode);
    }

    schemaPrefix = oldPrefix;
  }

  /**
   * This method actually builds and returns the tree.
   * 
   * @return the root node of the SWING tree built
   */
  public DefaultMutableTreeNode buildTree(String rootElement) {

    try {
      /* Look for the root element definition */
      Node node = findElementDefinition(rootElement);

      // System.out.println("Found document root node");

      if (node == null) {
        throw new Exception("Element type " + rootElement
            + " definition not found");
      }

      Element el = (Element) node;

      /* Create the root of the tree */
      ComplexTemplateNode ntn = new ComplexTemplateNode(rootElement, "complex");

      tree = new DefaultMutableTreeNode(ntn);
      position = tree;
      ntn.setNode(position);

      // System.out.println("Created tree root node");

      /*
       * Process the type of the root element. This is where the complex
       * hierarchy of recursive methods which process the schema and build the
       * tree is entered
       */
      processElementType(el);
    } catch (Exception e) {
      System.err.println("Exception occurred building tree: " + e);
      return null;
    }

    return tree;
  }
}
