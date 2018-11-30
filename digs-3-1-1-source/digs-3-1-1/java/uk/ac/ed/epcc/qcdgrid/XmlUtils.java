package uk.ac.ed.epcc.qcdgrid;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.File;
import java.io.IOException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXParseException;

import org.apache.log4j.Logger;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

public class XmlUtils {

    private static Logger logger_ = Logger.getLogger(XmlUtils.class);
    
    public static Document domFromFile(File doc) throws QCDgridException {
	try {
	    Document dom = null;
	    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
	    DocumentBuilder builder = factory.newDocumentBuilder();
	    return builder.parse(doc);
	} catch (ParserConfigurationException pce) {
	    throw new QCDgridException("Parser configuration exception: "
				       + pce.getMessage());
	} catch (SAXException se) {
	    throw new QCDgridException("Parser configuration exception: "
				       + se.getMessage());
	} catch (IOException ioe) {
	    throw new QCDgridException("Parser configuration exception: "
				       + ioe.getMessage());
	}
    }
    
    
    public static Document domFromString(String strDoc){
	//logger_.debug("Entering domFromString: " + strDoc);
	Document retval = null;
	DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
	if(dbf==null){
	    logger_.debug("Null docBuilderFactory");
	}
	try{
	    //logger_.debug("Trying to parse document");
	    DocumentBuilder db = dbf.newDocumentBuilder();
	    if(db == null){
		logger_.debug("Null document builder");
		return null;
	    }
	    retval = db.parse(new InputSource(new ByteArrayInputStream(strDoc.getBytes())));
	    logger_.debug("Successfully parsed document string");
	} catch(ParserConfigurationException pce) {
	    //pce.printStackTrace();
	    logger_.debug("Error (ParserConfigurationException) creating DOM: " 
			  + pce.getMessage());
	}catch(SAXException se) {
	    //se.printStackTrace();
	    logger_.debug("Error (SAXException) creating DOM: " + se.getMessage());
	}catch(IOException ioe) {
	    //ioe.printStackTrace();
	    logger_.debug("Error (IOException) creating DOM: " + ioe.getMessage());
	}       
	return retval;
    }
    
    /**
     * 
     * @param document
     * @param element
     * @return null if element not found
     */
    public static String getElementValueFromStringDocument(String document, String element){
	
	logger_.debug("Parsing string " + document + " to get " + element);
	Document doc = XmlUtils.domFromString(document);
	if(document == null){
	    logger_.debug("Null document created in getElementValueFromStringDocument");
	}else{
	    logger_.debug("Successfully created DOM document");
	    NodeList list = doc.getElementsByTagName(element);
	    if(list==null) return null;
	    return getNodeValue(list);
	}
	logger_.debug("Returning null from getElementValueFromStringDocument");
	return null;
    }
    
    private static String getNodeValue(NodeList list){
	logger_.debug("getNodeValue entered");
	if(list==null) {
	    logger_.debug("Exit0 getNodeValue");
	    return null;
	}
	else{
	    if( list.item(0) == null){
		logger_.debug("Exit1 getNodeValue");
		return null;
	    }
	    else{
		list = list.item(0).getChildNodes();  
	    }
	}
	if(list == null) {
	    logger_.debug("Exit2 getNodeValue");
	    return null;
	}
	Node n = list.item(0);
	String val = null;
	if(n!=null) val = (String) n.getNodeValue();
	logger_.debug("Exit3 getNodeValue");
	return val;
    }
    
    public static String getValueFromPath(String document, String path) {
	logger_.debug("getValueFromPath entered");
	Document doc = XmlUtils.domFromString(document);
	if (document == null) {
	    logger_.debug("Failed to parse document in getValueFromPath");
	}
	else {
	    logger_.debug("Parsed document in getValueFromPath");

	    String result = null;

	    Element root = doc.getDocumentElement();
	    String[] pathelems = path.split("/");
	    NodeList nl = root.getChildNodes();

	    Node node = null;
	    String nodeName = null;
	    int i, j;

	    if (root.getNodeName().equals(pathelems[0])) {
		
		for (i = 1; i < pathelems.length; i++) {
		    
		    for (j = 0; j < nl.getLength(); j++) {
			node = nl.item(j);
			nodeName = node.getNodeName();
			
			if (nodeName.equals(pathelems[i])) {
			    break;
			}
		    }

		    if (j == nl.getLength()) break;
		    nl = node.getChildNodes();
		}

		if ((i == pathelems.length) && (node != null)) {
		    node = node.getFirstChild();
		    result = node.getNodeValue();
		}
	    }
	    else {
		System.out.println("Root node doesn't match");
	    }
	    return result;
	}
	logger_.debug("Returning null from getValueFromPath");
	return null;
    }
}
