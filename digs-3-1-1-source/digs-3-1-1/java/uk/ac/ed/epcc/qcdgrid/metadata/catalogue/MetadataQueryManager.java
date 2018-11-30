/***********************************************************************
*
*   Filename:   MetadataQueryManager.java
*
*   Authors:    Daragh Byrne     (daragh)   EPCC.
*
*   Purpose:    An interface that allows metadata documents to be retrieved
*               from a repository.
*
*   Contents:   MetadataDocumentRetriever class.
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

public interface MetadataQueryManager {
	QCDgridQueryResults getXPathQueryResults( String query );
}
