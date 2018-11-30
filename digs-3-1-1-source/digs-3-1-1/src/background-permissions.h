/***********************************************************************
*
*   Filename:   background-permissions.h
*
*   Authors:    Radoslaw Ostrowski            (radek)   EPCC.
*
*   Purpose:    Code specific for changing file permissions
*
*   Contents:   Prototypes of new file functions
*
*   Used in:    Background process on main node of QCDgrid
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

#ifndef BACKGROUND_PERMISSIONS_H
#define BACKGROUND_PERMISSIONS_H


/*
 * Functions for making the permissions change addition list persistent
 */
void savePendingPermissionsList();
int loadPendingPermissionsList();

void removePendingPermissions(char *lfn);
void addPendingPermissions(int numParams, char ** params);
/*
 * Gets attributes from pendingpermissions list
 */
void getPendingPermission(char *lfn, char **recursive, char **group, char **permissions);

int handlePermissionsChanges();

#endif
