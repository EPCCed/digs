/**
 * MDCinfo.java
 *
 * This file was auto-generated from WSDL
 * by the Apache Axis 1.3 Oct 05, 2005 (05:23:37 EDT) WSDL2Java emitter.
 */

package org.lqcd.ildg.mdc;

public class MDCinfo  implements java.io.Serializable {
    private java.lang.String groupName;

    private java.lang.String groupURL;

    private java.lang.String[] queryTypes;

    private java.lang.String version;

    public MDCinfo() {
    }

    public MDCinfo(
           java.lang.String groupName,
           java.lang.String groupURL,
           java.lang.String[] queryTypes,
           java.lang.String version) {
           this.groupName = groupName;
           this.groupURL = groupURL;
           this.queryTypes = queryTypes;
           this.version = version;
    }


    /**
     * Gets the groupName value for this MDCinfo.
     * 
     * @return groupName
     */
    public java.lang.String getGroupName() {
        return groupName;
    }


    /**
     * Sets the groupName value for this MDCinfo.
     * 
     * @param groupName
     */
    public void setGroupName(java.lang.String groupName) {
        this.groupName = groupName;
    }


    /**
     * Gets the groupURL value for this MDCinfo.
     * 
     * @return groupURL
     */
    public java.lang.String getGroupURL() {
        return groupURL;
    }


    /**
     * Sets the groupURL value for this MDCinfo.
     * 
     * @param groupURL
     */
    public void setGroupURL(java.lang.String groupURL) {
        this.groupURL = groupURL;
    }


    /**
     * Gets the queryTypes value for this MDCinfo.
     * 
     * @return queryTypes
     */
    public java.lang.String[] getQueryTypes() {
        return queryTypes;
    }


    /**
     * Sets the queryTypes value for this MDCinfo.
     * 
     * @param queryTypes
     */
    public void setQueryTypes(java.lang.String[] queryTypes) {
        this.queryTypes = queryTypes;
    }


    /**
     * Gets the version value for this MDCinfo.
     * 
     * @return version
     */
    public java.lang.String getVersion() {
        return version;
    }


    /**
     * Sets the version value for this MDCinfo.
     * 
     * @param version
     */
    public void setVersion(java.lang.String version) {
        this.version = version;
    }

    private java.lang.Object __equalsCalc = null;
    public synchronized boolean equals(java.lang.Object obj) {
        if (!(obj instanceof MDCinfo)) return false;
        MDCinfo other = (MDCinfo) obj;
        if (obj == null) return false;
        if (this == obj) return true;
        if (__equalsCalc != null) {
            return (__equalsCalc == obj);
        }
        __equalsCalc = obj;
        boolean _equals;
        _equals = true && 
            ((this.groupName==null && other.getGroupName()==null) || 
             (this.groupName!=null &&
              this.groupName.equals(other.getGroupName()))) &&
            ((this.groupURL==null && other.getGroupURL()==null) || 
             (this.groupURL!=null &&
              this.groupURL.equals(other.getGroupURL()))) &&
            ((this.queryTypes==null && other.getQueryTypes()==null) || 
             (this.queryTypes!=null &&
              java.util.Arrays.equals(this.queryTypes, other.getQueryTypes()))) &&
            ((this.version==null && other.getVersion()==null) || 
             (this.version!=null &&
              this.version.equals(other.getVersion())));
        __equalsCalc = null;
        return _equals;
    }

    private boolean __hashCodeCalc = false;
    public synchronized int hashCode() {
        if (__hashCodeCalc) {
            return 0;
        }
        __hashCodeCalc = true;
        int _hashCode = 1;
        if (getGroupName() != null) {
            _hashCode += getGroupName().hashCode();
        }
        if (getGroupURL() != null) {
            _hashCode += getGroupURL().hashCode();
        }
        if (getQueryTypes() != null) {
            for (int i=0;
                 i<java.lang.reflect.Array.getLength(getQueryTypes());
                 i++) {
                java.lang.Object obj = java.lang.reflect.Array.get(getQueryTypes(), i);
                if (obj != null &&
                    !obj.getClass().isArray()) {
                    _hashCode += obj.hashCode();
                }
            }
        }
        if (getVersion() != null) {
            _hashCode += getVersion().hashCode();
        }
        __hashCodeCalc = false;
        return _hashCode;
    }

    // Type metadata
    private static org.apache.axis.description.TypeDesc typeDesc =
        new org.apache.axis.description.TypeDesc(MDCinfo.class, true);

    static {
        typeDesc.setXmlType(new javax.xml.namespace.QName("urn:mdc.ildg.lqcd.org", "MDCinfo"));
        org.apache.axis.description.ElementDesc elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("groupName");
        elemField.setXmlName(new javax.xml.namespace.QName("", "groupName"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "string"));
        elemField.setNillable(true);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("groupURL");
        elemField.setXmlName(new javax.xml.namespace.QName("", "groupURL"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "string"));
        elemField.setNillable(true);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("queryTypes");
        elemField.setXmlName(new javax.xml.namespace.QName("", "queryTypes"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "string"));
        elemField.setNillable(true);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("version");
        elemField.setXmlName(new javax.xml.namespace.QName("", "version"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "string"));
        elemField.setNillable(true);
        typeDesc.addFieldDesc(elemField);
    }

    /**
     * Return type metadata object
     */
    public static org.apache.axis.description.TypeDesc getTypeDesc() {
        return typeDesc;
    }

    /**
     * Get Custom Serializer
     */
    public static org.apache.axis.encoding.Serializer getSerializer(
           java.lang.String mechType, 
           java.lang.Class _javaType,  
           javax.xml.namespace.QName _xmlType) {
        return 
          new  org.apache.axis.encoding.ser.BeanSerializer(
            _javaType, _xmlType, typeDesc);
    }

    /**
     * Get Custom Deserializer
     */
    public static org.apache.axis.encoding.Deserializer getDeserializer(
           java.lang.String mechType, 
           java.lang.Class _javaType,  
           javax.xml.namespace.QName _xmlType) {
        return 
          new  org.apache.axis.encoding.ser.BeanDeserializer(
            _javaType, _xmlType, typeDesc);
    }

}
