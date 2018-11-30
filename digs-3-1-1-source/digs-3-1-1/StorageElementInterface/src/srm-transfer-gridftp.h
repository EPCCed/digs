/***********************************************************************
 *
 *   Filename:   srm-transfer-gridftp.h
 *
 *   Authors:    James Perry     (jamesp)   EPCC.
 *
 *   Purpose:    GridFTP protocol transfer module for SRM adaptor
 *
 *   Contents:   GridFTP protocol transfer module for SRM adaptor
 *
 *   Used in:    Control thread, clients
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2003-2010 The University of Edinburgh
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

#ifndef _SRM_TRANSFER_GRIDFTP_H_
#define _SRM_TRANSFER_GRIDFTP_H_

#include "misc.h"

/*
 * Gets the MD5 checksum of a remote file
 */
digs_error_code_t srm_gsiftp_checksum(char *errorMessage, char *turl,
				      char **fileChecksum);

/*
 * Initiates a get transfer
 */
digs_error_code_t srm_gsiftp_startGetTransfer(char *errorMessage,
					      char *hostname,
					      char *turl,
					      char *localFile,
					      int *handle);

/*
 * Gets the status of a transfer in progress
 */
digs_error_code_t
srm_gsiftp_monitorTransfer(char *errorMessage, int handle,
			   digs_transfer_status_t *status,
			   int *percentComplete);

/*
 * Finishes a transfer, checksumming the file
 */
digs_error_code_t srm_gsiftp_endTransfer(char *errorMessage, int handle);

/*
 * Cancels a transfer in progress
 */
digs_error_code_t srm_gsiftp_cancelTransfer(char *errorMessage,
					    int handle);

/*
 * Initiates a put transfer
 */
digs_error_code_t srm_gsiftp_startPutTransfer(char *errorMessage,
					      char *hostname,
					      char *turl,
					      char *localFile,
					      int *handle);

/*
 * Performs a 3rd party GridFTP copy
 */
digs_error_code_t srm_gsiftp_thirdPartyCopy(char *errorMessage,
					    const char *hostname,
					    char *sourceTurl,
					    char *targetTurl);

#endif
