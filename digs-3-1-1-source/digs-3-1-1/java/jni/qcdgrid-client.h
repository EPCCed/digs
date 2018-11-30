/***********************************************************************
*
*   Filename:   qcdgrid-client.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    QCDgrid client functions
*
*   Contents:   Prototypes for QCDgrid client library functions
*
*   Used in:    Command line utilities, APIs
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

#ifndef QCDGRID_CLIENT_H
#define QCDGRID_CLIENT_H

/*
 * General useful functions
 */
int qcdgridInit(int secondaryOK);
void qcdgridShutdown();
int qcdgridPing();

/*
 * Node state changing functions
 */
int qcdgridRemoveNode(char *node);
int qcdgridAddNode(char *node, char *site, char *path);
int qcdgridDisableNode(char *node);
int qcdgridEnableNode(char *node);
int qcdgridRetireNode(char *node);
int qcdgridUnretireNode(char *node);

/*
 * File/directory getting functions
 */
int qcdgridGetFile(char *lfn, char *pfn);
int qcdgridGetDirectory(char *ldn, char *pdn);

/*
 * File/directory putting functions
 */
int qcdgridPutFile(char *pfn, char *lfn, char *permissions);
int qcdgridPutDirectory(char *pfn, char *lfn, char *permissions);

/*
 * Listing
 */
char **qcdgridList();
void qcdgridDestroyList(char **list);

/*
 * Deleting
 */
int qcdgridDeleteFile(char *lfn);
int qcdgridDeleteDirectory(char *lfn);

/*
 * Touching
 */
int qcdgridTouchFile(char *lfn, char *dest);
int qcdgridTouchDirectory(char *lfn, char *dest);

#endif
