/***********************************************************************
*
*   Filename:   ObservableStateType.java
*
*   Authors:    Daragh Byrne    (daragh) EPCC
*
*   Purpose:    Defines the type of a state transition. At present, 
*               this is only used by QCDgridSubmitter when uploading files,
*               so only one type of state change is exported.
*
*   Contents:   ObservableStateType class
*             
*   Used in:    QCDgridSubmitter, QCDgridObserver
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
 * Exports a set of types of state change. Type safe enumeration pattern.
 */
public class ObservableStateType {

    /**
     * Used to notify of progress operating on multiple files.
     */
    public static final ObservableStateType MULTI_FILE_ACTION_PROGRESS 
	= new ObservableStateType();

    /**
     * Used when passing a general String based message.
     */
    public static final ObservableStateType GENERAL_MESSAGE = new ObservableStateType();
    
    /*
     * Used to notify of errors when submitting files.
     */
    public static final ObservableStateType ERROR_STATE
	= new ObservableStateType();

    public static final ObservableStateType FINISHED = new ObservableStateType();

    private ObservableStateType(){}

}
