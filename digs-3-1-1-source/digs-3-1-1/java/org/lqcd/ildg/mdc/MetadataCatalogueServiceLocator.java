/**
 * MetadataCatalogueServiceLocator.java
 *
 * This file was auto-generated from WSDL
 * by the Apache Axis 1.3 Oct 05, 2005 (05:23:37 EDT) WSDL2Java emitter.
 */

package org.lqcd.ildg.mdc;

public class MetadataCatalogueServiceLocator extends org.apache.axis.client.Service implements org.lqcd.ildg.mdc.MetadataCatalogueService {

    public MetadataCatalogueServiceLocator() {
    }


    public MetadataCatalogueServiceLocator(org.apache.axis.EngineConfiguration config) {
        super(config);
    }

    public MetadataCatalogueServiceLocator(java.lang.String wsdlLoc, javax.xml.namespace.QName sName) throws javax.xml.rpc.ServiceException {
        super(wsdlLoc, sName);
    }

    // Use to get a proxy class for MetadataCatalogue
    private java.lang.String MetadataCatalogue_address = "http://localhost:8080/axis/services/MetadataCatalogue";

    public java.lang.String getMetadataCatalogueAddress() {
        return MetadataCatalogue_address;
    }

    // The WSDD service name defaults to the port name.
    private java.lang.String MetadataCatalogueWSDDServiceName = "MetadataCatalogue";

    public java.lang.String getMetadataCatalogueWSDDServiceName() {
        return MetadataCatalogueWSDDServiceName;
    }

    public void setMetadataCatalogueWSDDServiceName(java.lang.String name) {
        MetadataCatalogueWSDDServiceName = name;
    }

    public org.lqcd.ildg.mdc.MDCInterface getMetadataCatalogue() throws javax.xml.rpc.ServiceException {
       java.net.URL endpoint;
        try {
            endpoint = new java.net.URL(MetadataCatalogue_address);
        }
        catch (java.net.MalformedURLException e) {
            throw new javax.xml.rpc.ServiceException(e);
        }
        return getMetadataCatalogue(endpoint);
    }

    public org.lqcd.ildg.mdc.MDCInterface getMetadataCatalogue(java.net.URL portAddress) throws javax.xml.rpc.ServiceException {
        try {
            org.lqcd.ildg.mdc.MdcSoapBindingStub _stub = new org.lqcd.ildg.mdc.MdcSoapBindingStub(portAddress, this);
            _stub.setPortName(getMetadataCatalogueWSDDServiceName());
            return _stub;
        }
        catch (org.apache.axis.AxisFault e) {
            return null;
        }
    }

    public void setMetadataCatalogueEndpointAddress(java.lang.String address) {
        MetadataCatalogue_address = address;
    }

    /**
     * For the given interface, get the stub implementation.
     * If this service has no port for the given interface,
     * then ServiceException is thrown.
     */
    public java.rmi.Remote getPort(Class serviceEndpointInterface) throws javax.xml.rpc.ServiceException {
        try {
            if (org.lqcd.ildg.mdc.MDCInterface.class.isAssignableFrom(serviceEndpointInterface)) {
                org.lqcd.ildg.mdc.MdcSoapBindingStub _stub = new org.lqcd.ildg.mdc.MdcSoapBindingStub(new java.net.URL(MetadataCatalogue_address), this);
                _stub.setPortName(getMetadataCatalogueWSDDServiceName());
                return _stub;
            }
        }
        catch (java.lang.Throwable t) {
            throw new javax.xml.rpc.ServiceException(t);
        }
        throw new javax.xml.rpc.ServiceException("There is no stub implementation for the interface:  " + (serviceEndpointInterface == null ? "null" : serviceEndpointInterface.getName()));
    }

    /**
     * For the given interface, get the stub implementation.
     * If this service has no port for the given interface,
     * then ServiceException is thrown.
     */
    public java.rmi.Remote getPort(javax.xml.namespace.QName portName, Class serviceEndpointInterface) throws javax.xml.rpc.ServiceException {
        if (portName == null) {
            return getPort(serviceEndpointInterface);
        }
        java.lang.String inputPortName = portName.getLocalPart();
        if ("MetadataCatalogue".equals(inputPortName)) {
            return getMetadataCatalogue();
        }
        else  {
            java.rmi.Remote _stub = getPort(serviceEndpointInterface);
            ((org.apache.axis.client.Stub) _stub).setPortName(portName);
            return _stub;
        }
    }

    public javax.xml.namespace.QName getServiceName() {
        return new javax.xml.namespace.QName("urn:mdc.ildg.lqcd.org", "MetadataCatalogueService");
    }

    private java.util.HashSet ports = null;

    public java.util.Iterator getPorts() {
        if (ports == null) {
            ports = new java.util.HashSet();
            ports.add(new javax.xml.namespace.QName("urn:mdc.ildg.lqcd.org", "MetadataCatalogue"));
        }
        return ports.iterator();
    }

    /**
    * Set the endpoint address for the specified port name.
    */
    public void setEndpointAddress(java.lang.String portName, java.lang.String address) throws javax.xml.rpc.ServiceException {
        
if ("MetadataCatalogue".equals(portName)) {
            setMetadataCatalogueEndpointAddress(address);
        }
        else 
{ // Unknown Port Name
            throw new javax.xml.rpc.ServiceException(" Cannot set Endpoint Address for Unknown Port" + portName);
        }
    }

    /**
    * Set the endpoint address for the specified port name.
    */
    public void setEndpointAddress(javax.xml.namespace.QName portName, java.lang.String address) throws javax.xml.rpc.ServiceException {
        setEndpointAddress(portName.getLocalPart(), address);
    }

}
