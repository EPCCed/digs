/***********************************************************************
*
*   Filename:   config.h
*
*   Authors:    James Perry            (jamesp)   EPCC.
*
*   Purpose:    Config file parser for QCDgrid
*
*   Contents:   Function prototypes for the config parser
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

#ifndef CONFIG_H
#define CONFIG_H

/*
 * Returns the first value associated with the given name in a config file, or
 * NULL if no values.
 *
 * The string returned should not be freed by the caller.
 */
char *getFirstConfigValue(char *id, char *name);

/*
 * Returns the first value associated with the given name in a config file,
 * as an integer.
 */
int getConfigIntValue(char *id, char *name, int def);

/*
 * Returns the next value associated with the given name in a config file, or
 * NULL if no more values.
 *
 * The string returned should not be freed by the caller.
 */
char *getNextConfigValue(char *id, char *name);

/*
 * Returns the key and value from the next line of the config file.
 *
 * The strings returned should not be freed by the caller.
 */
char *getNextConfigKeyValue(char *id, char **value);

/*
 * These functions can be used together to return to a particular
 * position in the file (useful, for example, for checking for
 * optional parameters without messing up the parsing)
 */
void setConfigPosition(char *id, int pos);
int getConfigPosition(char *id);

/*
 * Reads the config file specified, registering it under the given ID
 * string.
 *
 * Returns 1 on success, 0 on error
 */
int loadConfigFile(char *filename, char *id);

/*
 * Frees all the resources associated with the specified config file
 */
void freeConfigFile(char *id);

#endif
