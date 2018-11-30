/***********************************************************************
*
*   Filename:   qcdgrid-ping.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to check
*               whether the QCDgrid control thread is running
*
*   Contents:   Main function
*
*   Used in:    Day-to-day usage of QCDgrid
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

#include <globus_common.h>

#include "qcdgrid-client.h"

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of qcdgrid-ping executable. Sets everything
*   up, then sends a message to the main node
*    
*   Parameters:                                [I/O]
*
*    (none)
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    if (argc!=1) 
    {
	globus_libc_printf("Usage: digs-ping\n");
	return 0;
    }

    if (!qcdgridInit(0)) 
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	return 1;
    }
    atexit(qcdgridShutdown);

    if (!qcdgridPing())
    {
	globus_libc_printf("No response from control thread\n");
    }
    else
    {
	globus_libc_printf("Control thread is alive\n");
    }

    return 0;
}
