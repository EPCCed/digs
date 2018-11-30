package uk.ac.ed.epcc.qcdgrid.browser.QueryHandler;

import java.util.Vector;
import java.io.ByteArrayInputStream;

import javax.xml.parsers.*;
import org.xml.sax.SAXException;
import org.w3c.dom.*;
import org.xmldb.api.base.*;
import org.xmldb.api.modules.*;
import org.xmldb.api.*;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.QCDgridExistDatabase;
import uk.ac.ed.epcc.qcdgrid.XmlUtils;

public class QueryTypeManager {

    private Vector queryTypes;

    /* singleton instance */
    private static QueryTypeManager instance_ = null;

    public static QueryTypeManager getInstance() {
	return instance_;
    }

    public static void setInstance(QueryTypeManager qtm) {
	instance_ = qtm;
    }

    public QueryTypeManager() throws QCDgridException {
	/* Initialise */
	queryTypes = new Vector();
	
	/* Parse query types XML file */
	QCDgridExistDatabase db = QCDgridExistDatabase.getDriver();
	
	String queryTypesXml = null;
	try{
	    queryTypesXml = db.getSchema("QueryTypes.xml");  
	}
	catch(XMLDBException xdbe){
	    throw new QCDgridException("Could not load document QueryTypes.xml from the database: "
				       + xdbe.getMessage());
	}
	
	if ((queryTypesXml == null) || (queryTypesXml.equals(""))) {
	    throw new QCDgridException("No QueryTypes.xml file in collection!");
	}
	
	Document dom = XmlUtils.domFromString(queryTypesXml);
	Element root = dom.getDocumentElement();

	if (!root.getNodeName().equals("queryTypes")) {
	    throw new QCDgridException(
		"Root node of query types file is not 'queryTypes'");
	}
	
	NodeList nl = root.getChildNodes();
	
	for (int i = 0; i < nl.getLength(); i++) {
	    Node node = nl.item(i);
	    String name = node.getNodeName();
	    
	    if (node instanceof Element) {
		if (name.equals("queryType")) {
		    QueryType qt = new QueryType((Element) node);
		    queryTypes.add(qt);
		} else {
		    throw new QCDgridException("Malformed QueryTypes.xml file");
		}
	    }
	}

	QueryTypeManager.setInstance(this);
    }
    
    public Vector getQueryTypeList() {
	return queryTypes;
    }
    
    public QueryType queryTypeFromName(String name) {
	
	for (int i = 0; i < queryTypes.size(); i++) {
	    QueryType qt = (QueryType) queryTypes.elementAt(i);
	    
	    if (qt.getName().equals(name)) {
		return qt;
	    }
	}
	return null;
    }
}
