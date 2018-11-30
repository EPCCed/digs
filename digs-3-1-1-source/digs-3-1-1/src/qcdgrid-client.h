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
*   Copyright (c) 2003, 2007 The University of Edinburgh
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

/*======================================================================
 *
 * General useful functions
 *
 *====================================================================*/
/*
 * qcdgridInit should be called before using any other QCDgrid
 * functions. It sets up Globus, loads the configuration and does
 * everything else required to bring a grid client up.
 *
 * secondaryOK should be 1 if the read-only backup node can be used
 * (in the event that the main control node can't be contacted). If 0,
 * the backup node will not be used and the function will fail if the
 * main node is not available.
 *
 * Returns 1 on success, 0 on error.
 */
int qcdgridInit(int secondaryOK);

/*
 * Can be called at the end of a client command to release the
 * resources used in the QCDgrid library.
 */
void qcdgridShutdown();

/*
 * Sends a "ping" message to the control thread to determine if it is
 * up and running. Returns 1 on success, 0 on failure. Note: this will
 * always fail if the user is not authorised on the control node.
 */
int qcdgridPing();


/*======================================================================
 *
 * Node state changing functions
 *
 * All of these functions will only work if the user is a grid
 * administrator. They all return 1 on success and 0 on failure. In all
 * cases the "node" parameter is the fully qualified domain name of the
 * node being altered.
 *
 * For qcdgridAddNode, the "site" parameter is a string identifying the
 * physical site where the node resides. This is only used to ensure
 * that replicas of data are at different geographical locations. The
 * "path" parameter is the path to the QCDgrid software installation on
 * the new node.
 *
 *====================================================================*/
int qcdgridRemoveNode(char *node);
int qcdgridAddNode(char *node, char *site, char *path);
int qcdgridDisableNode(char *node);
int qcdgridEnableNode(char *node);
int qcdgridRetireNode(char *node);
int qcdgridUnretireNode(char *node);

/*======================================================================
 *
 * File/directory getting functions
 *
 * These functions transfer files from the grid to the local machine.
 * They return 1 on success and 0 on failure. qcdgridGetFile copies a
 * single file (logical filename lfn) to local storage (under the name
 * pfn). qcdgridGetDirectory copies an entire logical directory
 * (logical name ldn) to the physical directory pdn.
 *
 *====================================================================*/
int qcdgridGetFile(char *lfn, char *pfn);
int qcdgridGetDirectory(char *ldn, char *pdn);

/*======================================================================
 *
 * File/directory putting functions
 *
 * These functions put local files onto the grid. They return 1 on
 * success and 0 on failure. qcdgridPutFile puts a single file (pfn in
 * the local filesystem) onto the grid (under logical name lfn).
 * qcdgridPutDirectory does the same for an entire directory of files.
 *
 *====================================================================*/
int qcdgridPutFile(char *pfn, char *lfn, char *permissions);
int qcdgridPutDirectory(char *pfn, char *lfn, char *permissions);
int qcdgridModifyFile(char *pfn, char *lfn);

/*======================================================================
 *
 * Listing
 *
 *====================================================================*/
/*
 * Returns a list of all the logical filenames on the grid. The list is
 * terminated by a NULL pointer. It should be freed after use by calling
 * qcdgridDestroyList. The function returns NULL on failure.
 */
char **qcdgridList();

/*
 * Frees the list returned by qcdgridList
 */
void qcdgridDestroyList(char **list);

/*======================================================================
 *
 * Deleting
 *
 * Deletes a logical file or directory from the grid. Returns 1 on
 * success, 0 on failure. The user must have sufficient privileges to
 * perform this operation.
 *
 *====================================================================*/
int qcdgridDeleteFile(char *lfn);
int qcdgridDeleteDirectory(char *lfn);

/*======================================================================
 *
 * Touching
 *
 * These functions are used to register interest in having a file (or
 * logical directory) stored on a certain storage element. "lfn"
 * specifies the logical file or directory name, "dest" gives the fully
 * qualified domain name of the desired storage element. If "dest" is
 * NULL, a suitable location based on the local node preference list
 * will be chosen automatically. Both functions return 1 on success and
 * 0 on failure.
 *
 *====================================================================*/
int qcdgridTouchFile(char *lfn, char *dest);
int qcdgridTouchDirectory(char *lfn, char *dest);


/*
 * Contacts control thread and asks to change permissions on
 * given file or recursively on given directory
 */
int digsMakeP(char *permissions, char *lfn, int recursive);



#endif
