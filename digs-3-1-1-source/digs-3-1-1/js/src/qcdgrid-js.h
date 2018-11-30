/***********************************************************************
*
*   Filename:   qcdgrid-js.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    QCDgrid job submission header file
*
*   Contents:   Data types and function prototypes for QCDgrid job
*               submission software
*
*   Used in:    QCDgrid job submission system
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

#ifndef QCDGRID_JS_H
#define QCDGRID_JS_H

/*
 * Enumeration for messages passed over Globus socket from submitter
 * to controller
 */
enum { QJS_MSG_STDOUT,   /* Requesting new stdout data */
       QJS_MSG_STDERR,   /* Requesting new stderr data */
       QJS_MSG_KEEPALIVE,/* Message is a keepalive signal */
       QJS_MSG_OUTFILES, /* Requesting names of output files */
       QJS_MSG_STDIN,    /* Sending new stdin data */
       QJS_MSG_STATE     /* Requesting job state */
};

/*
 * Enumeration of possible job states
 */
enum { JOB_RUNNING, JOB_QUEUED, JOB_FINISHED, JOB_FETCHINGINPUT, 
       JOB_STORINGOUTPUT };

typedef struct hostInfo_s
{
    char *name;                /* fully qualified domain name */
    char *tmpDir;              /* temporary directory path */
    char *qcdgridDir;          /* path to QCDgrid software */
    int batchSystem;           /* which batch system (see enum in batch.h) */
    char *pbsPath;             /* path to PBS binaries */
    char *extraContact;        /* extra info for Globus job manager contact */
    char *extraRsl;            /* extra info for Globus RSL string */
} hostInfo_t;

/*
 * Enumeration of possible batch system types
 */
enum { BATCH_NONE, BATCH_PBS };

#endif
