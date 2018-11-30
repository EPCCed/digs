/***********************************************************************
*
*   Filename:   qcdgrid-checksum.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Utility to return the MD5 checksum of a file on the
*               QCDgrid
*
*   Contents:   Main function
*
*   Used in:    Executed remotely as a Globus job by the control thread
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

/* Need this to support files larger than 2GB */
#define __USE_LARGEFILE64

#include <sys/stat.h>

#include "md5.h"

/***********************************************************************
*   long long getFileLength(char *filename)
*    
*   Gets the length of a file on the local storage system
*    
*   Parameters:                                [I/O]
*
*     filename  Full pathname of file           I
*    
*   Returns: length of file in bytes, or -1 on error
***********************************************************************/
static long long getFileLength(char *filename)
{
    struct stat64 statbuf;

    if (stat64(filename, &statbuf)<0)
    {
	return -1;
    }

    return (long long) statbuf.st_size;
}

/***********************************************************************
*   void computeMd5Checksum(char *filename, unsigned char checksum[16])
*
*   Calculates the MD5 checksum of the specified local file    
*    
*   Parameters:                                [I/O]
*
*     filename  Name of file to work on         I
*     checksum  Checksum of the file              O
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
static int computeMd5Checksum(char *filename, unsigned char checksum[16])
{
    md5_state_t md5State;
    long long fileSize;
    FILE *f;
    static char buffer[1024];

    fileSize = getFileLength(filename);
    if (fileSize < 0)
    {
	return 0;
    }

    md5_init(&md5State);

    f = fopen(filename, "rb");
    if (!f)
    {
	return 0;
    }

    while(fileSize >= 1024)
    {
	if (fread(buffer, 1, 1024, f) != 1024)
	{
	    fclose(f);
	    return 0;
	}
	md5_append(&md5State, (unsigned char *)buffer, 1024);
	fileSize -= 1024;
    }

    if (fileSize > 0)
    {
	if (fread(buffer, 1, fileSize, f) != fileSize)
	{
	    fclose(f);
	    return 0;
	}
	md5_append(&md5State, (unsigned char *)buffer, fileSize);
    }

    md5_finish(&md5State, checksum);
    
    fclose(f);
    return 1;
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of qcdgrid-checksum executable
*    
*   Parameters:                                        [I/O]
*
*     argv[1]  Full name of file to get checksum for    I
*    
*   Returns: 0 on success, 1 on error
*            Prints MD5 checksum as hex to standard output, -1 on
*            error
***********************************************************************/
int main(int argc, char *argv[])
{
    unsigned char checksum[16];
    int i;

    if (argc != 2) {
	printf("-1");
	return 1;
    }

    if (!computeMd5Checksum(argv[1], checksum))
    {
	printf("-1");
	return 1;
    }
    
    for (i = 0; i < 16; i++)
    {
	printf("%02X", checksum[i]);
    }
    
    return 0;
}
