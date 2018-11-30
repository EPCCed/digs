/***********************************************************************
*
*   Filename:   qcdgrid-job-status.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Returns the status of an already submitted batch job
*
*   Contents:   Main function
*
*   Used in:    QCDgrid client tools for job submission
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

#include "qcdgrid-js.h"
#include "qcdgrid-client.h"
#include "jobio.h"

char *jobDirectory_;

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Top level main function of QCDgrid job submission utility
*    
*   Parameters:                                             [I/O]
*
*     argv[1]  job ID (directory it's in)                    I
*
*   Returns: 0 on success, 1 on failure
************************************************************************/
int main(int argc, char *argv[])
{
    FILE *jcFile;
    char *jcFilename;

    if (argc<2)
    {
	printf("Usage: qcdgrid-job-status <job ID>\n");
	return 0;
    }

    /* job ID is directory */
    jobDirectory_=argv[1];

    /*
     * Determine whether we're in 'client only' mode.
     * If we are, there'll be a file containing the GRAM contact string
     * for the job
     */
    if (asprintf(&jcFilename, "%s/jobcontact", jobDirectory_)<0)
    {
	fprintf(stderr, "Out of memory\n");
	return 1;
    }

    jcFile=fopen(jcFilename, "r");
    if (jcFile)
    {
	/* In 'client only' mode */

    }
    else
    {
	/* In server mode */

    }

    return 0;
}
