/**
 * QueryResults.java
 *
 * This file was auto-generated from WSDL
 * by the Apache Axis 1.3 Oct 05, 2005 (05:23:37 EDT) WSDL2Java emitter.
 */

package org.lqcd.ildg.mdc;

public class QueryResults  implements java.io.Serializable {
    private java.lang.String statusCode;

    private java.lang.String queryTime;

    private int totalResults;

    private int startIndex;

    private int numberOfResults;

    private java.lang.String[] results;

    public QueryResults() {
    }

    public QueryResults(
           java.lang.String statusCode,
           java.lang.String queryTime,
           int totalResults,
           int startIndex,
           int numberOfResults,
           java.lang.String[] results) {
           this.statusCode = statusCode;
           this.queryTime = queryTime;
           this.totalResults = totalResults;
           this.startIndex = startIndex;
           this.numberOfResults = numberOfResults;
           this.results = results;
    }


    /**
     * Gets the statusCode value for this QueryResults.
     * 
     * @return statusCode
     */
    public java.lang.String getStatusCode() {
        return statusCode;
    }


    /**
     * Sets the statusCode value for this QueryResults.
     * 
     * @param statusCode
     */
    public void setStatusCode(java.lang.String statusCode) {
        this.statusCode = statusCode;
    }


    /**
     * Gets the queryTime value for this QueryResults.
     * 
     * @return queryTime
     */
    public java.lang.String getQueryTime() {
        return queryTime;
    }


    /**
     * Sets the queryTime value for this QueryResults.
     * 
     * @param queryTime
     */
    public void setQueryTime(java.lang.String queryTime) {
        this.queryTime = queryTime;
    }


    /**
     * Gets the totalResults value for this QueryResults.
     * 
     * @return totalResults
     */
    public int getTotalResults() {
        return totalResults;
    }


    /**
     * Sets the totalResults value for this QueryResults.
     * 
     * @param totalResults
     */
    public void setTotalResults(int totalResults) {
        this.totalResults = totalResults;
    }


    /**
     * Gets the startIndex value for this QueryResults.
     * 
     * @return startIndex
     */
    public int getStartIndex() {
        return startIndex;
    }


    /**
     * Sets the startIndex value for this QueryResults.
     * 
     * @param startIndex
     */
    public void setStartIndex(int startIndex) {
        this.startIndex = startIndex;
    }


    /**
     * Gets the numberOfResults value for this QueryResults.
     * 
     * @return numberOfResults
     */
    public int getNumberOfResults() {
        return numberOfResults;
    }


    /**
     * Sets the numberOfResults value for this QueryResults.
     * 
     * @param numberOfResults
     */
    public void setNumberOfResults(int numberOfResults) {
        this.numberOfResults = numberOfResults;
    }


    /**
     * Gets the results value for this QueryResults.
     * 
     * @return results
     */
    public java.lang.String[] getResults() {
        return results;
    }


    /**
     * Sets the results value for this QueryResults.
     * 
     * @param results
     */
    public void setResults(java.lang.String[] results) {
        this.results = results;
    }

    private java.lang.Object __equalsCalc = null;
    public synchronized boolean equals(java.lang.Object obj) {
        if (!(obj instanceof QueryResults)) return false;
        QueryResults other = (QueryResults) obj;
        if (obj == null) return false;
        if (this == obj) return true;
        if (__equalsCalc != null) {
            return (__equalsCalc == obj);
        }
        __equalsCalc = obj;
        boolean _equals;
        _equals = true && 
            ((this.statusCode==null && other.getStatusCode()==null) || 
             (this.statusCode!=null &&
              this.statusCode.equals(other.getStatusCode()))) &&
            ((this.queryTime==null && other.getQueryTime()==null) || 
             (this.queryTime!=null &&
              this.queryTime.equals(other.getQueryTime()))) &&
            this.totalResults == other.getTotalResults() &&
            this.startIndex == other.getStartIndex() &&
            this.numberOfResults == other.getNumberOfResults() &&
            ((this.results==null && other.getResults()==null) || 
             (this.results!=null &&
              java.util.Arrays.equals(this.results, other.getResults())));
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
        if (getStatusCode() != null) {
            _hashCode += getStatusCode().hashCode();
        }
        if (getQueryTime() != null) {
            _hashCode += getQueryTime().hashCode();
        }
        _hashCode += getTotalResults();
        _hashCode += getStartIndex();
        _hashCode += getNumberOfResults();
        if (getResults() != null) {
            for (int i=0;
                 i<java.lang.reflect.Array.getLength(getResults());
                 i++) {
                java.lang.Object obj = java.lang.reflect.Array.get(getResults(), i);
                if (obj != null &&
                    !obj.getClass().isArray()) {
                    _hashCode += obj.hashCode();
                }
            }
        }
        __hashCodeCalc = false;
        return _hashCode;
    }

    // Type metadata
    private static org.apache.axis.description.TypeDesc typeDesc =
        new org.apache.axis.description.TypeDesc(QueryResults.class, true);

    static {
        typeDesc.setXmlType(new javax.xml.namespace.QName("urn:mdc.ildg.lqcd.org", "QueryResults"));
        org.apache.axis.description.ElementDesc elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("statusCode");
        elemField.setXmlName(new javax.xml.namespace.QName("", "statusCode"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "string"));
        elemField.setNillable(true);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("queryTime");
        elemField.setXmlName(new javax.xml.namespace.QName("", "queryTime"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "string"));
        elemField.setNillable(true);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("totalResults");
        elemField.setXmlName(new javax.xml.namespace.QName("", "totalResults"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "int"));
        elemField.setNillable(false);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("startIndex");
        elemField.setXmlName(new javax.xml.namespace.QName("", "startIndex"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "int"));
        elemField.setNillable(false);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("numberOfResults");
        elemField.setXmlName(new javax.xml.namespace.QName("", "numberOfResults"));
        elemField.setXmlType(new javax.xml.namespace.QName("http://www.w3.org/2001/XMLSchema", "int"));
        elemField.setNillable(false);
        typeDesc.addFieldDesc(elemField);
        elemField = new org.apache.axis.description.ElementDesc();
        elemField.setFieldName("results");
        elemField.setXmlName(new javax.xml.namespace.QName("", "results"));
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
