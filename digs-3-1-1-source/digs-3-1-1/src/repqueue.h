/***********************************************************************
*
*   Filename:   repqueue.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    This module provides a mechanism for queueing up
*               pending replication operations and executing them in
*               order of priority
*
*   Contents:   Enumerations and function prototypes
*
*   Used in:    QCDgrid central control thread
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

#ifndef REPQUEUE_H
#define REPQUEUE_H

/*
 * Reasons for replication of a file
 */
enum { REPTYPE_TOOFEWCOPIES,    /* there aren't enough copies just now */
       REPTYPE_REQUESTED,       /* user requested file be moved nearer */
};

enum { REPSTAGE_WAITING,    /* Waiting for a free gridftp slot to do transfer */
       REPSTAGE_3RDPARTY,   /* transferring */
       REPSTAGE_DONE,       /* transfer complete, waiting to register */
       REPSTAGE_DELETEME,   /* Completed. Wanting to be removed from the queue */
       REPSTAGE_ERROR,      /* An error occurred */
       REPSTAGE_GETTING,    /* The get operation is in progress */
       REPSTAGE_WAITING2,   /* Waiting for free slot to do second part of transfer */
       REPSTAGE_PUTTING,    /* The put operation is in progress */
};

void addToReplicationQueue(char *from, char *to, char *lfn, int reason);
void updateReplicationQueue();
void buildAllowedInconsistenciesList();

#endif
