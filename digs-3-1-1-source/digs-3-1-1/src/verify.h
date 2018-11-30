/***********************************************************************
*
*   Filename:   verify.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    To verify that QCDgrid replica catalogue matches
*               actual files on grid
*
*   Contents:   Verification function prototypes
*
*   Used in:    Background process and command line verification
*               utility
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

#ifndef VERIFY_H
#define VERIFY_H

/*
 * Function to verify the consistency of the grid
 */
int runGridVerification(char *node, int interactive);

/*
 * Function to run checksums on all nodes of the grid
 */
int runChecksums(int maxChecksums);

/*
 * Counters which can be used to report status of verification
 */
extern int inconsistencies_, unrepairedInconsistencies_;

#endif
