/***********************************************************************
*
*   Filename:   enable-qcdgrid-node.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Implementation of command line utility to enable a
*               previously disabled node on the QCDgrid
*
*   Contents:   Main function
*
*   Used in:    System administration of QCDgrid
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
*   Main entry point of enable-qcdgrid-node executable. Sets everything
*   up, then sends a message to the main node
*    
*   Parameters:                                [I/O]
*
*    argv[1]  FQDN of node to enable            I
*    
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    if (argc!=2) 
    {
	globus_libc_printf("Usage: %s <name>\n", argv[0]);
	return 0;
    }

    if (!qcdgridInit(0)) 
    {
	globus_libc_fprintf(stderr, "Error loading grid config\n");
	return 1;
    }
    atexit(qcdgridShutdown);

    if (!qcdgridEnableNode(argv[1]))
    {
	return 1;
    }

    return 0;
}
