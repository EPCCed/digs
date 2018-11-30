/***********************************************************************
*
*   Filename:   jobgridftp.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Provides a simple interface to Globus FTP functions to
*               the job submission
*
*   Contents:   Copy to local function
*
*   Used in:    Called by job submission to retrieve files from grid
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <globus_ftp_client.h>
#include <globus_gass_copy.h>

#include "jobgridftp.h"

extern FILE *jobWrapperLog_;

/***********************************************************************
*   int doGassCopy(char *from, char *to)
*
*   Copies one file://, gsiftp:// or ftp:// URL to another
*    
*   Parameters:                                                     [I/O]
*
*    from  Pointer to the URL to copy from                           I
*    to    Pointer to the URL to copy to                             I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
int doGassCopy(char *from, char *to)
{
    globus_gass_copy_handle_t handle;
    globus_gass_copy_attr_t srcAttr;
    globus_gass_copy_attr_t destAttr;

/*    fprintf(jobWrapperLog_, "Entering gass copy function...\n");*/

    /* Initialise handle and source and dest attributes */
    if (globus_gass_copy_handle_init(&handle, NULL)!=GLOBUS_SUCCESS)
    {
	fprintf(jobWrapperLog_, "Error initialising handle\n");
	return 0;
    }

/*    printf("Successfully initialised handle\n");*/

    if (globus_gass_copy_attr_init(&srcAttr)!=GLOBUS_SUCCESS) 
    {
	fprintf(jobWrapperLog_, "Error initialising source attributes\n");
	globus_gass_copy_handle_destroy(&handle);
	return 0;
    }
    
/*    printf("Successfully initialised source attributes\n");*/

    if (globus_gass_copy_attr_init(&destAttr)!=GLOBUS_SUCCESS) 
    {
	fprintf(jobWrapperLog_, "Error initialising dest attributes\n");
	globus_gass_copy_handle_destroy(&handle);
	return 0;
    }

/*    printf("Successfully initialised dest attributes\n");*/

    /* Actually perform the copy */
    if (globus_gass_copy_url_to_url(&handle, from, &srcAttr, to, &destAttr)!=
	GLOBUS_SUCCESS)
    {
	fprintf(jobWrapperLog_, "Error doing actual copy\n");
	globus_gass_copy_handle_destroy(&handle);
	return 0;
    }

/*    printf("Finished performing copy\n");*/

    globus_gass_copy_handle_destroy(&handle);

/*    printf("Leaving GASS copy function\n");*/
    return 1;
}

/***********************************************************************
*   int copyToLocal(char *remoteHost, char *remoteFile, char *localFile)
*
*   Copies a file to the local file system from a remote host using
*   GridFTP
*    
*   Parameters:                                                     [I/O]
*
*     remoteHost  FQDN of host to copy to                            I
*     remoteFile  Full pathname to file on remote machine            I
*     localFile   Full pathname to file on local machine             I
*    
*   Returns: 1 on success, 0 on error
***********************************************************************/
/* Copies a file from a remote machine to local storage */
int copyToLocal(char *remoteHost, char *remoteFile, char *localFile)
{
    char *localUrl, *remoteUrl;

    if (asprintf(&localUrl, "file://%s", localFile)<0)
    {
	return 0;
    }
    if (asprintf(&remoteUrl, "gsiftp://%s%s", remoteHost, remoteFile)<0)
    {
	return 0;
    }

    if (!doGassCopy(remoteUrl, localUrl))
    {
	free(localUrl);
	free(remoteUrl);
	return 0;
    }

    free(localUrl);
    free(remoteUrl);
    return 1;
}
