/***********************************************************************
*
*   Filename:   config.c
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Config file parser for QCDgrid
*
*   Contents:   Functions to read config files and return their contents
*               in a useful format
*
*   Used in:    Called by most QCDgrid commands
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

#include "misc.h"
#include "config.h"

/*
 * This structure represents one (non-blank, non-comment) line read from a
 * config file, which consists of 'name=value'
 */
typedef struct configLine_s
{
    int lineNo;
    char *name;
    char *value;
} configLine_t;

/*
 * This structure represents a config file which has been read from disk.
 */
typedef struct configFile_s
{
    char *name;
    int linesUsed;
    int linesAllocated;
    configLine_t *lines;
    int searchPos;
} configFile_t;

/*
 * Array of config files loaded already
 */
static int numConfigFiles_ = 0;
static configFile_t *configFiles_ = NULL;

/***********************************************************************
*   int configFileNumFromName(char *name)
*
*   Translates a config file name to its index in the array of loaded
*   config files. Should only be used internally
*    
*   Parameters:                                           [I/O]
*
*     name  The ID of the config file in question          I
*
*   Returns: index of fle in configFiles_ array, or -1 if not found
************************************************************************/
static int configFileNumFromName(char *name)
{
    int i;

    logMessage(1, "configFileNumFromName(%s)", name);

    i = 0;
    while(i < numConfigFiles_)
    {
	if (!strcmp(configFiles_[i].name, name))
	{
	    return i;
	}
	i++;
    }

    logMessage(3, "Could not find config file %s", name);
    return -1;
}

/***********************************************************************
*   char *getNextConfigValue(char *id, char *name)
*
*   Gets the next value associated with a name in the given config file
*    
*   Parameters:                                           [I/O]
*
*     id    The ID of the config file in question          I
*     name  The name of the attribute to look for          I
*
*   Returns: next value found, or NULL if no more found
************************************************************************/
char *getNextConfigValue(char *id, char *name)
{
    int fn;
    
    fn = configFileNumFromName(id);
    if (fn < 0)
    {
	return NULL;
    }

    while (configFiles_[fn].searchPos < configFiles_[fn].linesUsed)
    {
	if (!strcmp(configFiles_[fn].lines[configFiles_[fn].searchPos].name,
		    name))
	{
	    configFiles_[fn].searchPos++;
	    return &(configFiles_[fn].lines[configFiles_[fn].searchPos-1]
		     .value[0]);
	}
	configFiles_[fn].searchPos++;
    }
    return NULL;
}

/***********************************************************************
*   char *getNextConfigKeyValue(char *id, char **value)
*
*   Gets the next key and value pair from the config file
*    
*   Parameters:                                           [I/O]
*
*     id    The ID of the config file in question          I
*     value The value associated with the next key         I
*
*   Returns: next key found, or NULL if at end of file
************************************************************************/
char *getNextConfigKeyValue(char *id, char **value)
{
    int fn;
    char *key;

    fn = configFileNumFromName(id);
    if ((fn < 0) || (configFiles_[fn].searchPos >= configFiles_[fn].linesUsed))
    {
	if (value)
	{
	    *value = NULL;
	}
	return NULL;
    }

    key = configFiles_[fn].lines[configFiles_[fn].searchPos].name;
    if (value)
    {
	*value = configFiles_[fn].lines[configFiles_[fn].searchPos].value;
    }
    configFiles_[fn].searchPos++;
    return key;
}

/***********************************************************************
*   char *getFirstConfigValue(char *id, char *name)
*
*   Gets the first value associated with a name in the given config file
*    
*   Parameters:                                           [I/O]
*
*     id    The ID of the config file in question          I
*     name  The name of the attribute to look for          I
*
*   Returns: first value found, or NULL if none in the file
************************************************************************/
char *getFirstConfigValue(char *id, char *name)
{
    int fn;

    fn = configFileNumFromName(id);
    if (fn < 0)
    {
	return NULL;
    }

    configFiles_[fn].searchPos = 0;
    return getNextConfigValue(id, name);
}

/***********************************************************************
*   int getConfigIntValue(char *id, char *name, int def)
*
*   Gets the first value associated with a name in the given config file
*   as an integer
*    
*   Parameters:                                                [I/O]
*
*     id       The ID of the config file in question            I
*     name     The name of the attribute to look for            I
*     def      The value to return if not found in config file  I
*
*   Returns: first value found, or def if none found in file
************************************************************************/
int getConfigIntValue(char *id, char *name, int def)
{
    char *strval;
    int intval;

    strval = getFirstConfigValue(id, name);
    if (strval == NULL)
    {
	intval = def;
    }
    else
    {
	intval = atoi(strval);
    }
    return intval;
}

/***********************************************************************
*   int getConfigPosition(char *id)
*
*   Gets the current search position of a config file
*    
*   Parameters:                                           [I/O]
*
*     id    The ID of the config file in question          I
*
*   Returns: search position, or -1 if file not found
************************************************************************/
int getConfigPosition(char *id)
{
    int fn;

    fn = configFileNumFromName(id);
    if (fn < 0)
    {
	return -1;
    }
    return configFiles_[fn].searchPos;
}

/***********************************************************************
*   void setConfigPosition(char *id, int pos)
*
*   Sets the current search position of a config file
*    
*   Parameters:                                           [I/O]
*
*     id    The ID of the config file in question          I
*     pos   Position to set as search position             I
*
*   Returns: (void)
************************************************************************/
void setConfigPosition(char *id, int pos)
{
    int fn;

    fn = configFileNumFromName(id);
    if (fn < 0)
    {
	return;
    }
    configFiles_[fn].searchPos = pos;
}

/***********************************************************************
*   void processConfigLine(char *line, configFile_t *file, int num, 
*                          char *filename)
*
*   Parses a raw config line, turning it into an entry in the specified
*   configFile_t structure
*    
*   Parameters:                                                  [I/O]
*
*     line      The raw text of the line read from the file       I
*     file      Points to the file structure to add the line to   I
*     num       The line number within the file                   I
*     filename  The name of the file being processed. Used only
*               for error reporting                               I
*
*   Returns: (void)
************************************************************************/
static void processConfigLine(char *line, configFile_t *file, int num, char *filename)
{
    int i;
    int nameEnd;
    int valueStart;
    int valueEnd;

    /* Skip past leading white space */
    while(isspace(*line))
    {
	line++;
    }

    /* Remove CR/LF/comment from end */
    i = 0;
    while(line[i])
    {
	if ((line[i] == 13) || (line[i] == 10) || (line[i] == '#'))
	{
	    line[i] = 0;
	    break;
	}
	i++;
    }

    /* See if this is a blank line */
    if (!isalpha(*line))
    {
	return;
    }

    /* Parse into name and value */
    i=0;
    while ((line[i]) && (line[i] != ' ') && (line[i] != '='))
    {
	i++;
    }
    if (!line[i])
    {
	logMessage(3, "Error parsing line %d of file %s: `%s'", num,
		   filename, line);
	return;
    }
    nameEnd = i;

    while ((isspace(line[i])) || (line[i] == '='))
    {
	i++;
    }
    if (!line[i])
    {
	logMessage(3, "Error parsing line %d of file %s: `%s'", num,
		   filename, line);
	return;
    }
    valueStart = i;

    valueEnd = strlen(line) - 1;
    while (isspace(line[valueEnd]))
    {
	valueEnd--;
    }
    valueEnd++;

    line[nameEnd] = 0;
    line[valueEnd] = 0;

    /* Create a new line of config file */
    if (file->linesUsed >= file->linesAllocated)
    {
	file->lines = globus_libc_realloc(file->lines, 
					  (file->linesAllocated+10)*sizeof(configLine_t));
	if (!file->lines)
	{
	    errorExit("Out of memory in processConfigLine");
	}
	file->linesAllocated += 10;
    }
    
    file->lines[file->linesUsed].lineNo = num;
    file->lines[file->linesUsed].name = safe_strdup(line);
    file->lines[file->linesUsed].value = safe_strdup(&line[valueStart]);
    if ((!file->lines[file->linesUsed].name) ||
	(!file->lines[file->linesUsed].value))
    {
	errorExit("Out of memory in processConfigLine");
    }
    file->linesUsed++;
}

/***********************************************************************
*   void freeConfigFile(char *id)
*
*   Frees all the resources associated with the specified config file
*    
*   Parameters:                                           [I/O]
*
**     id        ID string to associate with info read      I
*
*   Returns: (void)
************************************************************************/
void freeConfigFile(char *id)
{
    int num;
    int i;

    logMessage(1, "freeConfigFile(%s)", id);

    num = configFileNumFromName(id);
    if (num < 0)
    {
	return;
    }

    for (i = 0; i < configFiles_[num].linesUsed; i++)
    {
	globus_libc_free(configFiles_[num].lines[i].name);
	globus_libc_free(configFiles_[num].lines[i].value);
    }
    globus_libc_free(configFiles_[num].lines);
    globus_libc_free(configFiles_[num].name);

    numConfigFiles_--;

    for (i = num; i < numConfigFiles_; i++)
    {
	configFiles_[i] = configFiles_[i + 1];
    }
}

/***********************************************************************
*   int loadConfigFile(char *filename, char *id)
*
*   Loads the config file specified and parses it, associating the lines
*   found with the given ID string
*    
*   Parameters:                                           [I/O]
*
*     filename  Physical name of file to parse             I
*     id        ID string to associate with info read      I
*
*   Returns: 1 on success, 0 on error
************************************************************************/
int loadConfigFile(char *filename, char *id)
{
    FILE *f;
    int bufferSize = 0;
    char *lineBuffer = NULL;
    int lineNum = 1;

    logMessage(1, "loadConfigFile(%s,%s)", filename, id);
    
    /* Now actually read the file */
    f = fopen(filename, "r");
    if (!f)
    {
	logMessage(3, "Cannot open config file %s", filename);
	return 0;
    }

    /* Allocate a new structure to store this file */
    configFiles_ = globus_libc_realloc(configFiles_, 
				       (numConfigFiles_+1)*sizeof(configFile_t));
    if (!configFiles_)
    {
	errorExit("Out of memory in loadConfigFile");
    }

    /* Fill it with appropriate info */
    configFiles_[numConfigFiles_].name = safe_strdup(id);
    if (!configFiles_[numConfigFiles_].name)
    {
	errorExit("Out of memory in loadConfigFile");
    }
    configFiles_[numConfigFiles_].linesUsed = 0;
    configFiles_[numConfigFiles_].linesAllocated = 0;
    configFiles_[numConfigFiles_].lines = NULL;
    configFiles_[numConfigFiles_].searchPos = 0;

    do
    {
	if (safe_getline(&lineBuffer, &bufferSize, f) < 0)
	{
	    break;
	}
	processConfigLine(lineBuffer, &configFiles_[numConfigFiles_], lineNum,
			  filename);
	lineNum++;
    } while (!feof(f));

    globus_libc_free(lineBuffer);

    fclose(f);
    numConfigFiles_++;
    return 1;
}
