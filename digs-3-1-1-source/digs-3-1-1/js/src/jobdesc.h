/***********************************************************************
*
*   Filename:   jobdesc.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Parses job description file and makes its contents
*               accessible through a simple interface
*
*   Contents:   Job description parser function prototypes
*
*   Used in:    QCDgrid job submission
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

#ifndef JOBDESC_H
#define JOBDESC_H

/*
 * Search functions
 */
char *getJobDescriptionEntry(char *name);
char *getNextJobDescriptionEntry(char *name);

/*
 * Parser function
 */
int parseJobDescription(FILE *jobDescription);

#endif
