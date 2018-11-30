/***********************************************************************
*
*   Filename:   background-delete.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Code related to deleting files on QCDgrid
*
*   Contents:   Deletion function prototypes
*
*   Used in:    Background process on main node of QCDgrid
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

#ifndef BACKGROUND_DELETE_H
#define BACKGROUND_DELETE_H

/*
 * Functions for making the pending deletion list persistent
 */
int loadPendingDeleteList();

/*
 * Tries to process any pending deletions on the specified host. Could
 * be called, for example, when the node comes back from being dead or
 * disabled
 */
void tryDeletesOnHost(char *host);

/*
 * Attempts to delete all copies of the logical file specified
 */
void deleteLogicalFile(char *lfn);

/*
 * Attempts to delete an entire directory from the grid
 */
void deleteDirectory(char *lfn);

/*
 * Works out whether a user has permissions to delete a file or
 * directory on the grid
 */
int canDeleteFile(char *lfn, char *user);
int canDeleteDirectory(char *lfn, char *user);

#endif
