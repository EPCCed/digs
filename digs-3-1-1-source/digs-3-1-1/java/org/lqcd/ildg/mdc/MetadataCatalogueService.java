/**
 * MetadataCatalogueService.java
 *
 * This file was auto-generated from WSDL
 * by the Apache Axis 1.3 Oct 05, 2005 (05:23:37 EDT) WSDL2Java emitter.
 */

package org.lqcd.ildg.mdc;

public interface MetadataCatalogueService extends javax.xml.rpc.Service {
    public java.lang.String getMetadataCatalogueAddress();

    public org.lqcd.ildg.mdc.MDCInterface getMetadataCatalogue() throws javax.xml.rpc.ServiceException;

    public org.lqcd.ildg.mdc.MDCInterface getMetadataCatalogue(java.net.URL portAddress) throws javax.xml.rpc.ServiceException;
}
