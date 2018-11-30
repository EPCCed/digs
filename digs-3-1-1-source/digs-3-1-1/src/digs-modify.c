/***********************************************************************
*
*   Filename:   digs-modify.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to modify a file
*               on DiGS
*
*   Contents:   Main function
*
*   Used in:    Day to day usage of DiGS
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2008 The University of Edinburgh
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

#include <globus_common.h>

#include "qcdgrid-client.h"
#include "misc.h"


/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of digs-modify executable
*    
*   Parameters:                                            [I/O]
*
*     Physical file containing new file contents            I
*     Logical filename of file to be changed                I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char usage[] = "Usage: digs-modify <local filename> <grid filename>\n";
    char *ssLFN;
    int result;

    if (argc != 3)
    {
	globus_libc_printf(usage);
	return 1;
    }

    if (!qcdgridInit(0)) 
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	return 1;
    }
    atexit(qcdgridShutdown);


    ssLFN = substituteChars(argv[2], "//", "/");

    result = qcdgridModifyFile(argv[1], ssLFN);
    globus_libc_free(ssLFN);

    return !result;
}

