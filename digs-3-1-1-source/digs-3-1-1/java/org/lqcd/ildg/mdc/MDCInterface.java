/**
 * MDCInterface.java
 *
 * This file was auto-generated from WSDL
 * by the Apache Axis 1.3 Oct 05, 2005 (05:23:37 EDT) WSDL2Java emitter.
 */

package org.lqcd.ildg.mdc;

public interface MDCInterface extends java.rmi.Remote {
    public org.lqcd.ildg.mdc.QueryResults doEnsembleURIQuery(java.lang.String queryFormat, java.lang.String queryString, int startIndex, int maxResults) throws java.rmi.RemoteException;
    public org.lqcd.ildg.mdc.QueryResults doConfigurationLFNQuery(java.lang.String queryFormat, java.lang.String queryString, int startIndex, int maxResults) throws java.rmi.RemoteException;
    public org.lqcd.ildg.mdc.MetadataResult getEnsembleMetadata(java.lang.String ensembleURI) throws java.rmi.RemoteException;
    public org.lqcd.ildg.mdc.MetadataResult getConfigurationMetadata(java.lang.String configurationLFN) throws java.rmi.RemoteException;
    public org.lqcd.ildg.mdc.MDCinfo getMDCinfo() throws java.rmi.RemoteException;
}
