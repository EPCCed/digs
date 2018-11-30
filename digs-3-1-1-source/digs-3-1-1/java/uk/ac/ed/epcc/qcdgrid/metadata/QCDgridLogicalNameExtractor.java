/***********************************************************************
 *
 *   Filename:   QCDgridLogicalNameExtractor.java
 *
 *   Authors:    James Perry            (jamesp)   EPCC.
 *
 *   Purpose:    This is a SAX content handler class used for
 *               extracting a logical filename from a QCD metadata
 *               document
 *
 *   Contents:   QCDgridLogicalNameExtractor
 *
 *   Used in:    Metadata handling tools
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2003 The University of Edinburgh
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
/*
 * FIXME: this needs to be fixed properly, in order to use the logical
 * filename paths from the config document. Currently hacked together
 * for HackLatt '05
 */
package uk.ac.ed.epcc.qcdgrid.metadata;

import java.io.*;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;
import uk.ac.ed.epcc.qcdgrid.metadata.validator.ValidationException;
import uk.ac.ed.epcc.qcdgrid.metadata.PrefixHandler;

/**
 * SAX content handler class for extracting the logical filename from a QCDgrid
 * metadata document
 */
public class QCDgridLogicalNameExtractor extends DefaultHandler {

  /** Logical filename extracted */
  private String logicalFileName;

  /** Flags indicating where in the document nesting we are */
  private boolean inMarkovStep;

  private boolean inValue;

  private boolean inPointerToValue;

  private boolean inDataLfn;

  /**
   * Extracts the LFN from a file object. Assumes the file is a valid config
   * document.
   */
  public static String getLFNFromMetadataDocument(File metadatafile)
      throws QCDgridException {
    String lfn;
    try {
      byte[] data = new byte[(int) metadatafile.length()];
      FileInputStream is = new FileInputStream(metadatafile);
      is.read(data);
      String content = new String(data);
      QCDgridLogicalNameExtractor qlne = new QCDgridLogicalNameExtractor(
          content);
      lfn = qlne.getLogicalFileName();
    } catch (ParserConfigurationException cfe) {
      throw new QCDgridException("Could not parse metadata document");
    } catch (SAXException se) {
      throw new QCDgridException("Could not parse metadata document");
    } catch (IOException iox) {
      throw new QCDgridException("Could not parse metadata document");
    }

    //RADEK I could verify the prefix here for all files with metadata  
    return PrefixHandler.handlePrefix(lfn);
  }

  /**
   * Extracts the LFN from a string representation of the metadata.
   */
  public static String getLFNFromMetadataDocument(String docAsString)
      throws QCDgridException
  // throws ValidationException, QCDgridException
  {
    String lfn;
    try {
      QCDgridLogicalNameExtractor lne = new QCDgridLogicalNameExtractor(
          docAsString);
      lfn = lne.getLogicalFileName();
    } catch (ParserConfigurationException cfe) {
      throw new QCDgridException("Could not parse metadata document");
    } catch (SAXException se) {
      throw new QCDgridException("Could not parse metadata document");
    } catch (IOException iox) {
      throw new QCDgridException("Could not parse metadata document");
    }
    return lfn;
  }

  /**
   * Constructor: creates a new logical name extractor and parses the given
   * document with it
   * 
   * @param doc
   *          the document content to parse
   */
  public QCDgridLogicalNameExtractor(String doc)
      throws ParserConfigurationException, SAXException, IOException

  {
    ByteArrayInputStream is = new ByteArrayInputStream(doc.getBytes());
    SAXParserFactory spf = SAXParserFactory.newInstance();
    SAXParser sp = spf.newSAXParser();
    sp.parse(is, this);
  }

  /**
   * @return the logical filename found
   */
  public String getLogicalFileName() {
    System.out.println("lfn2: " + logicalFileName.trim());
    return logicalFileName.trim();
  }

  /**
   * Handles raw character data from the document. If we're inside the
   * pointer_to_value tag, this is part of the logical name we want
   */
  public void characters(char[] ch, int start, int length) {
    /*
     * if (inPointerToValue) { logicalFileName = logicalFileName + new
     * String(ch, start, length); }
     */
    if (inDataLfn) {
      logicalFileName = logicalFileName + new String(ch, start, length);
      //System.out.println("lfn:" + logicalFileName);
    }
  }

  /**
   * Handles end of the document - no need to do anything
   */
  public void endDocument() {

  }

  /**
   * Handles start of document - initialise state variables
   */
  public void startDocument() {
    inMarkovStep = false;
    // inValue=false;
    // inPointerToValue=false;
    inDataLfn = false;
    logicalFileName = "";
  }

  /**
   * Handles the end of an element. Update our nesting state variables to
   * reflect this
   */

  public void endElement(String nameSpaceURI, String name, String qname) {
    // System.out.println("Element end: "+qname);
    if (qname.equals("markovStep")) {
      inMarkovStep = false;
      // inValue=false;
      // inPointerToValue=false;
      inDataLfn = false;
    }
    /*
     * else if (qname.equals("value")) { inValue=false; inPointerToValue=false; }
     * else if (qname.equals("pointer_to_value")) { inPointerToValue=false; }
     */
    else if (qname.equals("dataLFN")) {
      inDataLfn = false;
    }
  }

  /**
   * Handles the start of an element. Update our nesting state variables to
   * reflect this
   */
  public void startElement(String nameSpaceURI, String name, String qname,
      Attributes atts) {
    //System.out.println("Element start: "+qname);
    if (qname.equals("markovStep")) {
      inMarkovStep = true;
    }
    /*
     * else if (qname.equals("value")) { if (inMarkovStep) { inValue=true; } }
     * else if (qname.equals("pointer_to_value")) { if
     * ((inMarkovStep)&&(inValue)) { inPointerToValue=true; } }
     */
    else if (qname.equals("dataLFN")) {
      if (inMarkovStep) {
        System.out.println("inDataLFN");
        inDataLfn = true;
      }
    }
  }
}
