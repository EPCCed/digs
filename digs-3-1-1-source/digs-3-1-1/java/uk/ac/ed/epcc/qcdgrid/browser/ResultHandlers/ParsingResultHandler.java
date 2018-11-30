/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import java.util.*;
import java.io.*;

import javax.xml.parsers.*;
import org.xml.sax.SAXException;
import org.w3c.dom.*;

import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;

/**
 * Title: ParsingResultHandler Description: A result handler subclass which
 * splits the result into documents and parses each one. Intended to be extended
 * by result handlers which need to do more complex things with the results
 * Copyright: Copyright (c) 2002 Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */

abstract public class ParsingResultHandler extends ResultHandler {

  /**
   * Vector which holds DOM Documents representing each document from the result
   * set
   */
  protected Vector documents;

  /**
   * Vector which holds each document from the result set as a string
   */
  protected Vector docsAsStrings;

  /** Number of documents returned */
  protected int numDocuments;

  /**
   * Parses the results and stores them in the above vector. Should be called by
   * any subclasses wishing to make use of this
   * 
   * @param results
   *          a string containing the entire result set
   * @param queryTime
   *          the time at which the query ran
   */
  public void handleResults(QCDgridQueryResults qr) {

    int docStart, docEnd;

    String results = qr.combinedResult();

    documents = new Vector();
    docsAsStrings = new Vector();
    numDocuments = 0;

    /*
     * First split the String into documents (at the <?xml version...> tags)
     */
    docStart = 0;

    while (docStart < (results.length() - 1)) {
      docEnd = results.indexOf("<?xml", docStart + 1);
      if (docEnd == -1)
        docEnd = results.length();

      String thisDoc = results.substring(docStart, docEnd);
      docsAsStrings.add(thisDoc);
      numDocuments++;

      docStart = docEnd;
    }

    for (int i = 0; i < numDocuments; i++) {
      /*
       * Now convert each document to a byte array for use in byte array input
       * streams
       */
      String thisDoc = (String) docsAsStrings.elementAt(i);
      byte[] docAsBytes = thisDoc.getBytes();

      /*
       * Parse each document using DOM and add them to the vector
       */
      try {
        DocumentBuilderFactory docbuildfact = DocumentBuilderFactory
            .newInstance();
        DocumentBuilder docbuild = docbuildfact.newDocumentBuilder();
        Document dom = docbuild.parse(new ByteArrayInputStream(docAsBytes));
        documents.add(dom);
      } catch (Exception e) {
        System.err.println("Exception occurred while parsing results: " + e);
      }
    }
  }
}
