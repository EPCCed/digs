/***********************************************************************
*
*   Filename:   digs-chmod.c
*
*   Authors:    Radoslaw Ostrowski           (radek)   EPCC.
*
*   Purpose:    Utility to check and or change permissions of files
*
*   Contents:   Main function
*
*   Used in:    Executed remotely as a Globus job by the control thread
*
*   Contact:    epcc-support@epcc.ed.ac.uk
*
*   Copyright (c) 2007 The University of Edinburgh
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
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/types.h>


/* Need this to support files larger than 2GB */
#define __USE_LARGEFILE64


int checkFilePermissions(const char *file, const char *permissions)
{
  struct stat statbuf;
  /* this bit is set in statbuf.st_mode*/
  int firstBit = 0100000;

  if (stat(file, &statbuf) < 0)
  {
    //error
    return -1;
  }


  //private
   //readable and writable by user: S_IRUSR | S_IWUSR
  // readable by group: S_IRGRP
  // if the mode is 100640
  if ((strcmp("private", permissions) == 0)
      && ((S_IRUSR | S_IWUSR | S_IRGRP | firstBit) == statbuf.st_mode))
  {
    return 1;
  }

  
  //public
  //readable and writable by user: S_IRUSR | S_IWUSR
  //readable by group: S_IRGRP
  //readable by others S_IROTH
  // if the mode is 100644
  if ((strcmp("public", permissions) == 0) && 
      (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | firstBit) == statbuf.st_mode)
  {
    return 1;
  }

  // permissions are different
  return 0;
}


int changeFilePermissions(const char *file, const char *permissions)
{

  if (strcmp("private", permissions) == 0)
  {
    //printf("changing file %s permissions to private\n", file);

  //RADEK check if it is ok first before changing 
    if (checkFilePermissions(file, permissions) <= 0)
    {
      // readable to the owner and group, writable to owner only
      chmod(file, S_IWUSR|S_IRUSR|S_IRGRP);
    }
  }
  else if (strcmp("public", permissions) == 0)
  {
    //printf("changing file %s permissions to public\n", file);

  //RADEK check if it is ok first before changing 
    if (checkFilePermissions(file, permissions) <= 0)
    {
      // readable to the owner, group and world, writable to owner only
      chmod(file, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
    }
  }
  else
  {
    //printf("Argument 'permissions'=%s wasn't understood. Use: private or public\n", permissions);
    return 0;
  }

  return 1;
}

int changeDirectoryPermissions(const char *file, const char *permissions)
{
    /* Need to recursively loop over all files within the directory, in case
     * any of them are sub-directories. In this case the logical filenames
     * on the grid will have the same directory structure as the physical
     * filenames on disk */
    struct dirent *entry;
    DIR *dir;
    char *newpfn;

    // 
    if ((newpfn = malloc(256*sizeof(char))) == NULL)
    {
      //printf("Out of memory in changeDirectoryOwnership\n");
      return 0;
    }

    //printf("changeDirectoryPermissions(file=%s, permissions=%s)\n", file, permissions);

    // test radek - it's a directory
    // make directories readable to all always

    // DOESN'T WORK ON DIRS AND CAUSE FILES TO LOOSE THEIR PROPERTIES AND BECOME UNUSABLE!!!
    //if (!changeFilePermissions(file, "public"))
    //{
    // logMessage(5, "Error processing file %s", file);
    //}
    // WORKS ON FILES THOUGH


    /* Open directory */
    dir = opendir(file);
    if (!dir)
    {
        //printf("Unable to open directory %s\n", file);
        return 0;
    }

    /* Read each entry in turn */
    entry = readdir(dir);

    while (entry)
    {
      sprintf(newpfn, "%s/%s", file, entry->d_name);
      //printf("newpfn = %s\n", newpfn);
        /* Don't want to call recursively on '.' and '..' or we'll be in
         * trouble... */
        if (entry->d_name[0] != '.')
        {
            /* Find out if this is a subdirectory. Have to use 'stat' -
             * checking the entry->d_type field doesn't seem to work on
             * trumpton */
            struct stat statbuf;

            stat(newpfn, &statbuf);

            /* Handle a subdirectory */
            if (S_ISDIR(statbuf.st_mode))
            {
                /* and call self recursively. */
                /* NOTE: readdir returns a pointer to a statically allocated
                 * buffer, so we mustn't rely on entry pointing to the same
                 * thing when the recursive call returns */
              //        if (!qcdgridPutDirectory(newpfn, newlfn))

                if(!changeDirectoryPermissions(newpfn, permissions))
                {
                    closedir(dir);
                    free(newpfn);
                    return 0;
                }
            }
            else
            {
                /* Handle a regular file - just put it on the grid */
                //printf("%s\n", newpfn);
                if (!changeFilePermissions(newpfn, permissions))
                {
                    printf("Error processing file %s\n", newpfn);
		    //return or not?
                }
            }
        }
        free(newpfn);
        entry = readdir(dir);
    }

    closedir(dir);

    return 1;
}

/***********************************************************************
*   int main(int argc, char *argv[])
*    
*   Main entry point of qcdgrid-checksum executable
*    
*   Parameters:                                        [I/O]
*
*     argv[1]  private or public flag                   I
*     argv[2]  pfn to change permissions on             I
*
*   Returns: 0 on success, 1 on error
***********************************************************************/
int main(int argc, char *argv[])
{
    char *permissions;
    char *pfn;
    
    if (argc != 3) 
    {
	printf("-1");
	return 1;
    }
    
    permissions = argv[1];
    pfn = argv[2];

    if(!changeFilePermissions(pfn, permissions))
    {
      printf("-1");
      return 1;
    }
 
    return 0;
}
