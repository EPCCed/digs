/***********************************************************************
*
*   Filename:   background-modify.h
*
*   Authors:    James Perry              (jamesp)   EPCC.
*
*   Purpose:    Code for modifying files in DiGS
*
*   Contents:   Function prototypes related to modifying and locking
*               files on grid
*
*   Used in:    Background process on main node of DiGS
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2007-2008 The University of Edinburgh
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

#ifndef BACKGROUND_MODIFY_H
#define BACKGROUND_MODIFY_H

void savePendingModList();
void loadPendingModList();

int isNewModification(char *lfn, char *host);
int canModify(char *lfn, char *user);
int addFileModification(char *lfn, char *host, char *md5sum,
			char *size, char *modtime);

int handleNewModification(char *lfn, char *realLfn, char *host);
int tryModificationsOnHost(char *host);

int lockFile(char *lfn, char *user);
int unlockFile(char *lfn, char *user);
int lockDirectory(char *lfn, char *user);
int unlockDirectory(char *lfn, char *user);

char *fileLockedBy(char *lfn);
int modificationsPending(char *lfn);

int canLockFile(char *lfn, char *user);
int canUnlockFile(char *lfn, char *user);
int canLockDirectory(char *lfn, char *user);
int canUnlockDirectory(char *lfn, char *user);

#endif
