/***********************************************************************
*
*   Filename:   diskspace.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Functions to get disk space
*
*   Contents:   Functions to get disk space
*
*   Used in:    get-disk-space program and QCDgrid utilities
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2003-2006 The University of Edinburgh
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
#include <ctype.h>

#include "diskspace.h"

void *globus_libc_realloc(void *, int);

int safe_asprintf (char **ptr, const char *template, ...);
int safe_getline (char **lineptr, int *n, FILE *stream);
void globus_libc_fprintf(FILE *f, char *str);
void globus_libc_free(void *ptr);

/*
 * FIXME: this whole module is a bit of a hack and it would be really
 * good if there was a nicer and more robust way to do it. On Linux we
 * could use statfs in fact, but this appears not to be portable to
 * other UNIXes
 */

/***********************************************************************
*   char *skipOneColumn(char *str)
*    
*   Utility function to skip past one column in the output from the df
*   command
*    
*   Parameters:                                [I/O]
*
*    str  Pointer to column to skip past        I
*    
*   Returns: pointer to next column
***********************************************************************/
static char *skipOneColumn(char *str)
{
    char *tmp;

    tmp = str;
    while (!isspace(*tmp)) tmp++;
    while (isspace(*tmp)) tmp++;
    return tmp;
}

/***********************************************************************
*   long long getFreeSpaceMmdf(char *path)
*    
*   Returns free disk space in kilobytes at the path specified, on
*   systems using GPFS
*    
*   Parameters:                                [I/O]
*
*    path  path to check disk free on           I
*    
*   Returns: disk space in kilobytes
***********************************************************************/
long long getFreeSpaceMmdf(char *path)
{
    FILE *f;
    int bufSize;
    char *lineBuffer;
    char *numStart;
    char *numEnd;
    long long result;

    char target[20];
    int targetnum;
    int i;

    i = strlen(path) - 1;
    targetnum = 0;
    if (isdigit(path[i]))
    {
	targetnum = path[i] - '0';
	if (isdigit(path[i-1]))
	{
	    targetnum = targetnum + ((path[i-1] - '0') * 10);
	}
    }
    targetnum++;

    sprintf(target, "gpfs%dnsd", targetnum + 16);

    result = (long long) 0;

    /* Parse the output */
    f = popen("mmdf gpfs", "r");

    bufSize = 0;
    lineBuffer = NULL;
    safe_getline(&lineBuffer, &bufSize, f);

    while (!feof(f))
    {
	if (!strncmp(lineBuffer, target, strlen(target)))
	{
	    numStart = skipOneColumn(lineBuffer);
	    numStart = skipOneColumn(numStart);
	    numStart = skipOneColumn(numStart);
	    numStart = skipOneColumn(numStart);
	    numStart = skipOneColumn(numStart);

	    numEnd = numStart;
	    while (isdigit(*numEnd)) numEnd++;
	    result = strtoll(numStart, NULL, 10);
	}

	safe_getline(&lineBuffer, &bufSize, f);
    }
    globus_libc_free(lineBuffer);
    fclose(f);
    return result;
}

/***********************************************************************
*   long long getFreeSpace(char *path)
*    
*   Returns free disk space in kilobytes at the path specified
*    
*   Parameters:                                [I/O]
*
*    path  path to check disk free on           I
*    
*   Returns: disk space in kilobytes
***********************************************************************/
long long getFreeSpace(char *path)
{
    FILE *f;
    int bufSize;
    char *lineBuffer;
    char *commandBuffer;
    char *numStart;
    char *numEnd;
    int isLinux;
    int suffix;
    long long result;

    /* Invoke df */
    if (safe_asprintf(&commandBuffer, "df %s", path)<0) 
    {
	globus_libc_fprintf(stderr, "Out of memory\n");
	return -1;
    }

    /* Parse the output */
    f = popen(commandBuffer, "r");
    globus_libc_free(commandBuffer);
    if (!f)
    {
	globus_libc_fprintf(stderr, "popen failed in getFreeSpace\n");
	return -1;
    }

    bufSize = 0;
    lineBuffer = NULL;
    safe_getline(&lineBuffer, &bufSize, f);

    /*
     * Linux and Solaris (and probably other unixes) produce
     * different output here. Work out which it is. Solaris
     * version doesn't produce a line of column headings, so
     * the first line will start with '/'
     */
    if (lineBuffer[0] == '/')
    {
	isLinux = 0;

	/* Solaris version. Number we want follows a closed bracket */
	numStart = strchr(lineBuffer, ')');
	if (!numStart)
	{
	    globus_libc_free(lineBuffer);
	    globus_libc_fprintf(stderr, "'df' returned unexpected output\n");
	    return 1;
	}

	/* Skip space 'til we get to the actual number */
	while ((*numStart) && (!isdigit(*numStart)))
	{
	    numStart++;
	}
	if (!(*numStart))
	{
	    globus_libc_free(lineBuffer);
	    globus_libc_fprintf(stderr, "'df' returned unexpected output\n");
	    return 1;
	}
    }
    else
    {
	isLinux = 1;

	/* Ignore first line on Linux, it's column headings */
	/* Get line we're interested in */
	safe_getline(&lineBuffer, &bufSize, f);

	/*
	 * Get a second line from just below if there is one. df sometimes
	 * splits output over two lines to make the columns line up better
	 */
	if (!feof(f))
	{
	    char *lineBuffer2 = NULL;
	    int bufSize2 = 0;
	    safe_getline(&lineBuffer2, &bufSize2, f);

	    if (lineBuffer2)
	    {
		lineBuffer = globus_libc_realloc(lineBuffer,
						 strlen(lineBuffer)+strlen(lineBuffer2)+1);
		strcat(lineBuffer, lineBuffer2);
		globus_libc_free(lineBuffer2);
	    }
	}

	numStart = skipOneColumn(lineBuffer);
	numStart = skipOneColumn(numStart);
	numStart = skipOneColumn(numStart);
    }

    suffix = 1;

    numEnd = numStart;
    while (isdigit(*numEnd)) numEnd++;

    /* Deal with new dfs that display in megabytes/gigabytes */
    if ((*numEnd) == 'M')
    {
	suffix = 1000;
    }
    else if ((*numEnd) == 'G')
    {
	suffix = 1000000;
    }

    *numEnd = 0;

    result = strtoll(numStart, NULL, 10);
    result *= suffix;

    fclose(f);

    if (lineBuffer)
    {
	globus_libc_free(lineBuffer);
    }

    return result;
}
