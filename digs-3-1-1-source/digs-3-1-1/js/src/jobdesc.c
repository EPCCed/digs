/***********************************************************************
*
*   Filename:   jobdesc.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Parses job description file and makes its contents
*               accessible through a simple interface
*
*   Contents:   Job description parser and search functions
*
*   Used in:    QCDgrid job submission
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

#include "jobdesc.h"

/*
 * Store the job description file entries in
 * a simple data structure
 */
struct jobDescriptionEntry_s
{
    char *name;
    char *value;
};

/*
 * Array of job description entries from file
 */
static struct jobDescriptionEntry_s *jobDescriptionEntries_=NULL;

/*
 * Number of entries in above array
 */
static int numJobDescriptionEntries_=0;

/*
 * Search position within job description file
 */
static int currentJobDescriptionEntry_=0;

/***********************************************************************
*   char *getNextJobDescriptionEntry(char *name)
*    
*   Returns the value of the next job description entry with the given
*   name. Starts search from last entry returned
*    
*   Parameters:                                        [I/O]
*
*     name  Entry name to search for                    I
*    
*   Returns: entry's value, NULL if no more entries with this name
***********************************************************************/
char *getNextJobDescriptionEntry(char *name)
{
    for (; currentJobDescriptionEntry_<numJobDescriptionEntries_; currentJobDescriptionEntry_++)
    {
	if (!strcmp(jobDescriptionEntries_[currentJobDescriptionEntry_].name, name))
	{
	    currentJobDescriptionEntry_++; /* start next search from next entry */
	    return jobDescriptionEntries_[currentJobDescriptionEntry_-1].value;
	}
    }
    return NULL;
}

/***********************************************************************
*   char *getJobDescriptionEntry(char *name)
*    
*   Returns the value of the first job description entry with the
*   given name
*    
*   Parameters:                                        [I/O]
*
*     name  Entry name to search for                    I
*    
*   Returns: entry's value, NULL if no entries with this name
***********************************************************************/
char *getJobDescriptionEntry(char *name)
{
    currentJobDescriptionEntry_=0;
    return getNextJobDescriptionEntry(name);
}

/***********************************************************************
*   int parseJobDescription(FILE *jobDescription)
*    
*   Parses the job description file, creating internal data structures
*   containing the information from it
*    
*   Parameters:                                        [I/O]
*
*     jobDescription  file to read job description from I
*    
*   Returns: 1 on success, 0 on failure
***********************************************************************/
int parseJobDescription(FILE *jobDescription)
{
    char *lineBuffer=NULL;
    int lineBufferLength=0;
    int nameStart, nameEnd, valueStart, valueEnd;
    int lineNumber=1;

    /*
     * Free old structure if any
     */
    if (jobDescriptionEntries_)
    {
	free(jobDescriptionEntries_);
	numJobDescriptionEntries_=0;
    }

    /* Read the first line */
    if (getline(&lineBuffer, &lineBufferLength, jobDescription)<0)
    {
	error("Error reading first line of job description\n");
	return 0;
    }

    /* Continue until the end of the file */
    while (!feof(jobDescription))
    {
	/* Parse the line */
	/* Find the start of the name field */
	nameStart=0;
	while ((lineBuffer[nameStart])&&(isspace(lineBuffer[nameStart])))
	{
	    nameStart++;
	}
	/* Check for blank or comment lines */
	if ((lineBuffer[nameStart])&&(lineBuffer[nameStart]!='#')&&
	    (lineBuffer[nameStart]!=13)&&(lineBuffer[nameStart]!=10))
	{
	    /* Find the end of the name field */
	    nameEnd=nameStart;
	    while ((lineBuffer[nameEnd])&&(!isspace(lineBuffer[nameEnd]))&&
		   (lineBuffer[nameEnd]!='='))
	    {
		nameEnd++;
	    }

	    if ((lineBuffer[nameEnd]!=' ')&&(lineBuffer[nameEnd]!='='))
	    {
		error("Syntax error in job description, line %d: `%s'\n",
		      lineNumber, lineBuffer);
		free(lineBuffer);
		return 0;
	    }

	    /* Find start of value field */
	    valueStart=nameEnd;
	    while ((lineBuffer[valueStart])&&
		   ((lineBuffer[valueStart]=='=')||(isspace(lineBuffer[valueStart]))))
	    {
		valueStart++;
	    }

	    if ((!lineBuffer[valueStart])||(lineBuffer[valueStart]==13)||(lineBuffer[valueStart]==10))
	    {
		/* '=' is followed by end of line, value is empty - allow this case */
		valueEnd=valueStart;
	    }
	    else
	    {
		valueEnd=valueStart;
		/* Find end of value field */
		while ((lineBuffer[valueEnd])&&(lineBuffer[valueEnd]!='#')&&
		       (lineBuffer[valueEnd]!=13)&&(lineBuffer[valueEnd]!=10))
		{
		    valueEnd++;
		}
	    }
	    
	    /* Create a new structure entry */
	    numJobDescriptionEntries_++;
	    jobDescriptionEntries_=realloc(jobDescriptionEntries_,
					   numJobDescriptionEntries_*sizeof(struct jobDescriptionEntry_s));
	    if (!jobDescriptionEntries_)
	    {
		error("Out of memory while parsing job description\n");
		return 0;
	    }

	    /* Fill it */
	    jobDescriptionEntries_[numJobDescriptionEntries_-1].name=malloc((nameEnd-nameStart)+1);
	    jobDescriptionEntries_[numJobDescriptionEntries_-1].value=malloc((valueEnd-valueStart)+1);
	    if ((!jobDescriptionEntries_[numJobDescriptionEntries_-1].name)||
		(!jobDescriptionEntries_[numJobDescriptionEntries_-1].value))
	    {
		error("Out of memory while parsing job description\n");
		return 0;
	    }
	    strncpy(jobDescriptionEntries_[numJobDescriptionEntries_-1].name, &lineBuffer[nameStart],
		    nameEnd-nameStart);
	    jobDescriptionEntries_[numJobDescriptionEntries_-1].name[nameEnd-nameStart]=0;
	    strncpy(jobDescriptionEntries_[numJobDescriptionEntries_-1].value, &lineBuffer[valueStart],
		    valueEnd-valueStart);
	    jobDescriptionEntries_[numJobDescriptionEntries_-1].value[valueEnd-valueStart]=0;
	}

	/* Read the next line */
	lineNumber++;
	if (getline(&lineBuffer, &lineBufferLength, jobDescription)<0)
	{
	    break;
	}
    }

    if (lineBuffer)
    {
	free(lineBuffer);
    }
    /* Return success */
    return 1;
}
