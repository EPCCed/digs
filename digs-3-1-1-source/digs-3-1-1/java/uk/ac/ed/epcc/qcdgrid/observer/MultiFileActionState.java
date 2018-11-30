/***********************************************************************
*
*   Filename:   FileSubmitState.java
*
*   Authors:    Daragh Byrne    (daragh) EPCC
*
*   Purpose:    Value class that contains the status of a file upload 
*               operation.
*
*   Contents:   FileSubmitState class
*             
*   Used in:    
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
*/

package uk.ac.ed.epcc.qcdgrid.observer;

/*
 * Contains the current state of a multi file operation.
 */
public class MultiFileActionState
{
    private int      numFilesTotal_;
    private int      numFilesUploaded_;
    private String   lastUploadedFileName_;
    private String   lastUploadedMetadataFileName_;

    /*
     * @return The total number of files that are to be uploaded.
     */
    public int getNumFilesTotal(){
	return numFilesTotal_;
    }

    public void setNumFilesTotal( int numFilesTotal ){
	this.numFilesTotal_ = numFilesTotal;
    }

    /*
     * @return The current number of files that have been uploaded.
     */
    public int getNumFilesProcessed(){
	return numFilesUploaded_;
    }

    public void setNumFilesProcessed( int numFilesUploaded ){
	this.numFilesUploaded_ = numFilesUploaded;
    }

    /*
     * @return The name of the last uploaded file.
     */
    public String getLastProcessedFileName(){
	return lastUploadedFileName_;
    }

    public void setLastProcessedFileName( String lufn ){
	this.lastUploadedFileName_ = lufn;
    }

    /*
     * @return The name of the last uploaded metadata file
     */
    public String getLastProcessedMetadataFileName(){
	return this.lastUploadedMetadataFileName_;
    }
    public void setLastProcessedMetadataFileName( String lumfn ){
	this.lastUploadedMetadataFileName_ = lumfn;
    }

    public MultiFileActionState(){

    }
}
