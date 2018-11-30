/***********************************************************************
*
*   Filename:   WebServiceMetadataDocumentRetriever.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    An implementation of the MetadataDocumentRetriever interface
*               that uses a metadata catalogue web service to retrieve a 
*               particular document.
*
*   Contents:   WebSerivceMetadataDocumentRetriever class
*
*   Used in:    Java client tools
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2006 The University of Edinburgh
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

package uk.ac.ed.epcc.qcdgrid.metadata.catalogue;

import org.lqcd.ildg.mdc.MdcSoapBindingStub;
import org.lqcd.ildg.mdc.MetadataCatalogueServiceLocator;
import org.lqcd.ildg.mdc.MetadataResult;

import uk.ac.ed.epcc.qcdgrid.QCDgridException;

import java.rmi.RemoteException;
import javax.xml.rpc.ServiceException;


/**
 * An implementation of the MetadataDocumentRetriever interface
*               that uses a metadata catalogue web service to retrieve a 
*               particular document.
 * @author daragh
 *
 */
public class WebServiceMetadataDocumentRetriever implements
    MetadataDocumentRetriever {

  // The service from which to download the document.
  private MdcSoapBindingStub webService_;
  
  public WebServiceMetadataDocumentRetriever() {
  }
  
  
  /**
   * Download the document using the ID (ensemble URI) and the service url.
   */
  public String getEnsembleMetadataDocument(String id, String hostInfo)
    throws QCDgridException {
    ////System.out.println("WebServiceMetadataDocumentRetriever:getEnsMetadataDoc");
    String ret = null;
    try{
      MetadataCatalogueServiceLocator locator = new MetadataCatalogueServiceLocator();
      locator.setMetadataCatalogueEndpointAddress(hostInfo);
      webService_ = (MdcSoapBindingStub)locator.getMetadataCatalogue();
      MetadataResult mr = webService_.getEnsembleMetadata(id);
      if(mr!=null){
        ret = mr.getDocument();
      }
      else {
        throw new QCDgridException("Could not find document " + id);
      }
    }
    catch(ServiceException se){
      throw new QCDgridException(se.getMessage());
    }
    catch(RemoteException re){
      throw new QCDgridException(re.getMessage());
    }
    return ret;
  }

  /**
   * Download the document using the ID (configuration LFN) and the service url..
   */
  public String getConfigurationMetadataDocument(String id, String hostInfo)
      throws QCDgridException {
    String ret = null;
    ////System.out.println("WebServiceMetadataDocumentRetriever:getCfgMetadataDoc" + id + " " + hostInfo);
    try{
      MetadataCatalogueServiceLocator locator = new MetadataCatalogueServiceLocator();
      ////System.out.println("got locator");
      locator.setMetadataCatalogueEndpointAddress(hostInfo);
      ////System.out.println("set address");
      webService_ = (MdcSoapBindingStub)locator.getMetadataCatalogue();
      if(webService_ == null){
        ////System.out.println("Null webservice object");
      }
      else{
        ////System.out.println("got webservice object");
      }
      MetadataResult mr  = webService_.getConfigurationMetadata(id);
      if(mr!=null){
        ret = mr.getDocument();
      }
      else {
        throw new QCDgridException("Could not find document " + id);
      }
    }
    catch(ServiceException se){
      ////System.out.println("getEnsMetadataDoc:serviceException");
      throw new QCDgridException(se.getMessage());
    }
    catch(RemoteException re){
      ////System.out.println("getEnsMetadataDoc:remoteException");
      throw new QCDgridException(re.getMessage());
    }
    return ret;
  }

  /**
   * Not supported yet
   */
  public String getCorrelatorMetadataDocument(String id, String hostInfo)
      throws QCDgridException {
    return "";
  }
}
