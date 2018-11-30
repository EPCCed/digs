/***********************************************************************
 *
 *   Filename:   setup-se-test.c
 *
 *   Authors:    James Perry (jamesp)   EPCC.
 *
 *   Purpose:    Sets up the environment ready for running the storage
 *               element test suite. Creates the required files and
 *               directory structures both on the local machine and on
 *               the SE used for the tests.
 *
 *   Contents:   Setup code for the tests.
 *
 *   Used in:    SE testing
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2010 The University of Edinburgh
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

#include <time.h>
#include <sys/stat.h>

#include "misc.h"
#include "node.h"
#include "config.h"

void oom()
{
  globus_libc_fprintf(stderr, "Out of memory\n");
  exit(1);
}

char *getProperty(char *prop)
{
  char *result = getFirstConfigValue("setest", prop);
  if (!result) {
    globus_libc_fprintf(stderr, "Missing property %s in config file\n", prop);
    exit(1);
  }
  return result;
}

int createMainNodeList()
{
  FILE *f;
  char *localPath;
  char *mnlName;
  char *mainNode;
  char *mainNodePath;
  char *mainNodeType;
  char *niNode;
  char *niNodePath;

  localPath = getProperty("local_file_path");
  mainNode = getProperty("valid_se");
  mainNodePath = getProperty("valid_se_path");
  mainNodeType = getProperty("valid_se_type");
  niNode = getProperty("no_inbox_se");
  niNodePath = getProperty("no_inbox_se_path");
  
  if (safe_asprintf(&mnlName, "%s/mainnodelist.conf", localPath) < 0) {
    oom();
  }

  f = fopen(mnlName, "w");
  if (!f) {
    fprintf(stderr, "Error creating %s\n", mnlName);
    return 0;
  }

  /* valid SE */
  fprintf(f, "node=%s\n", mainNode);
  fprintf(f, "site=Edinburgh\n");
  fprintf(f, "disk=50000000\n");
  fprintf(f, "path=%s\n", mainNodePath);
  fprintf(f, "type=%s\n", mainNodeType);
  fprintf(f, "inbox=%s/NEW\n", mainNodePath);
  fprintf(f, "multidisk=1\n");
  fprintf(f, "data=500000\n");
  fprintf(f, "data1=500000\n\n");

  /* SE without inbox */
  fprintf(f, "node=%s\n", niNode);
  fprintf(f, "site=Edinburgh\n");
  fprintf(f, "disk=0\n");
  fprintf(f, "path=%s\n", niNodePath);
  fprintf(f, "type=globus\n");
  fprintf(f, "data=50\n\n");

  /* SE without service running */
  fprintf(f, "node=garnet.epcc.ed.ac.uk\n");
  fprintf(f, "site=Edinburgh\n");
  fprintf(f, "disk=0\n");
  fprintf(f, "path=/non/existent/path\n");
  fprintf(f, "inbox=/non/existent/inbox\n");
  fprintf(f, "type=globus\n");
  fprintf(f, "data=50\n\n");  

  /* SE that doesn't exist */
  fprintf(f, "node=ral\n");
  fprintf(f, "site=RAL\n");
  fprintf(f, "disk=0\n");
  fprintf(f, "path=/non/existent/path\n");
  fprintf(f, "inbox=/non/existent/inbox\n");
  fprintf(f, "type=globus\n");
  fprintf(f, "data=50\n");

  fclose(f);
  return 1;
}

int createNodesConf()
{
  char *localPath;
  char *nodesConfName;
  char *mainNode;
  char *mainNodePath;
  FILE *f;

  localPath = getProperty("local_file_path");
  mainNode = getProperty("valid_se");
  mainNodePath = getProperty("valid_se_path");

  if (safe_asprintf(&nodesConfName, "%s/nodes.conf", localPath) < 0) {
    oom();
  }

  f = fopen(nodesConfName, "w");
  if (!f) {
    globus_libc_fprintf(stderr, "Error writing to %s\n", nodesConfName);
    globus_libc_free(nodesConfName);
    return 0;
  }

  fprintf(f, "node = %s\n", mainNode);
  fprintf(f, "path = %s\n", mainNodePath);
  fprintf(f, "mdc = %s\n", mainNode);
  fclose(f);
  
  globus_libc_free(nodesConfName);

  return 1;
}

int createLocalFileStructure(char *path)
{
  /* as this is just a test suite it should be safe to use fixed size buffers */
  char buf[2000];
  char *testDataPath;
  FILE *f;

  testDataPath = getProperty("test_data_path");

  /* remove temp/otherDownloadedFile if present - it's created by the tests */
  sprintf(buf, "%s/testData/temp/otherDownloadedFile", path);
  unlink(buf);

  /* create testData - don't check for errors as it might already be present */
  sprintf(buf, "%s/testData", path);
  mkdir(buf, 0750);

  /* put largeFile and file1.txt in there */
  /* check existence of the test data files first */
  sprintf(buf, "%s/file1.txt", testDataPath);
  f = fopen(buf, "rb");
  if (!f) {
    fprintf(stderr, "Cannot find test file %s\n", buf);
    return 0;
  }
  fclose(f);
  sprintf(buf, "%s/largeFile", testDataPath);
  f = fopen(buf, "rb");
  if (!f) {
    fprintf(stderr, "Cannot find test file %s\n", buf);
    return 0;
  }
  fclose(f);
  sprintf(buf, "cp %s/file1.txt %s/testData/file1.txt", testDataPath, path);
  system(buf);
  sprintf(buf, "cp %s/largeFile %s/testData/largeFile", testDataPath, path);
  system(buf);

  /* create temp dir in there */
  sprintf(buf, "%s/testData/temp", path);
  mkdir(buf, 0750);

  /* create noWritePermissions dir and chmod it to 550 */
  sprintf(buf, "%s/testData/noWritePermissions", path);
  mkdir(buf, 0550);

  return 1;
}

int copyFileToSE(struct storageElement *se, char *localPath, char *remotePath)
{
  digs_error_code_t result;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  int handle;
  int progress;
  digs_transfer_status_t status;

  globus_libc_printf("Copying %s to %s\n", localPath, remotePath);
  
  result = se->digs_startPutTransfer(errbuf, se->name, localPath, remotePath,
				     &handle);
  if (result != DIGS_SUCCESS) {
    return 0;
  }

  do {
    globus_libc_usleep(10000);
    if (se->digs_monitorTransfer(errbuf, handle, &status, &progress) != DIGS_SUCCESS) {
      return 0;
    }
  } while (status == DIGS_TRANSFER_IN_PROGRESS);

  result = se->digs_endTransfer(errbuf, handle);
  if (result != DIGS_SUCCESS) {
    return 0;
  }

  return 1;
}

int createRemoteFileStructure()
{
  char hostname[256];
  char *desthost;
  char *destuser;
  char *setype;
  char *curruser;
  char *localpath;
  char buf[2000], buf2[2000];
  char *sedir;
  char *testdatadir;
  FILE *f;
  struct tm *tminfo;
  time_t ft;

  /*
   * first check if we can do this the easy way! If we're testing
   * a globus SE, and it's on the machine we're running on, and
   * we're the same user as we will be mapped to for the tests,
   * just copy the files locally instead of using the SE interface.
   */
  globus_libc_gethostname(hostname, 256);
  desthost = getProperty("valid_se");
  destuser = getProperty("username");
  setype = getProperty("valid_se_type");
  curruser = getenv("LOGNAME");

  sedir = getProperty("valid_se_path");
  testdatadir = getProperty("test_data_path");
  localpath = getProperty("local_file_path");

  if ((!strcmp(hostname, desthost)) && (!strcmp(setype, "globus")) &&
      (!strcmp(destuser, curruser))) {
    struct stat statbuf;

    globus_libc_printf("Using local method to setup SE\n");

    /* delete things created by the tests: */
    /*
     * uploadedFile, otherUploadedFile
     * NEW/myDir-DIR-anotherDir-DIR-file1.txt
     * NEW/myDir-DIR-anotherDir-DIR-largeFile.txt
     */
    sprintf(buf, "%s/uploadedFile", sedir);
    unlink(buf);
    sprintf(buf, "%s/otherUploadedFile", sedir);
    unlink(buf);
    sprintf(buf, "%s/NEW/myDir-DIR-anotherDir-DIR-file1.txt", sedir);
    unlink(buf);
    sprintf(buf, "%s/NEW/myDir-DIR-anotherDir-DIR-largeFile.txt", sedir);
    unlink(buf);
    
    /* copy in file1.txt, largeFile and fileForMoving.txt. set file1.txt to 640 */
    sprintf(buf, "cp %s/file1.txt %s/file1.txt", testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/fileForMoving.txt", testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/largeFile %s/largeFile", testdatadir, sedir);
    system(buf);
    sprintf(buf, "%s/file1.txt", sedir);
    chmod(buf, 0640);

    /* create NEW directory for inbox */
    sprintf(buf, "%s/NEW", sedir);
    mkdir(buf, 0750);
    
    /* create data, data1 and sub-directories */
    sprintf(buf, "%s/data", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "%s/data/fruit", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "%s/data/fruit/apple", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "%s/data/fruit/apple/cooking", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "cp %s/file1.txt %s/data/fruit/forbidden-LOCKED",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/data/fruit/orange",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/data/fruit/apple/cox",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/data/fruit/apple/grannySmith",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/data/fruit/apple/cooking/bramley",
	    testdatadir, sedir);
    system(buf);

    sprintf(buf, "%s/data1", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "%s/data1/veg", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "%s/data1/veg/root", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "cp %s/file1.txt %s/data1/veg/kale",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/data1/veg/root/carrot",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "cp %s/file1.txt %s/data1/veg/root/potato",
	    testdatadir, sedir);
    system(buf);

    /* create noWriteAccessDir and sub-dirs, set it to 550 */
    sprintf(buf, "%s/noWriteAccessDir", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "%s/noWriteAccessDir/dirInsideNoWriteAccess", sedir);
    mkdir(buf, 0550);
    sprintf(buf, "cp %s/file1.txt %s/noWriteAccessDir/file1.txt",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "%s/noWriteAccessDir/file1.txt", sedir);
    chmod(buf, 0440);
    sprintf(buf, "%s/noWriteAccessDir", sedir);
    chmod(buf, 0550);
    
    /* create noReadAccessDir and sub-dirs, set it to 000 */
    sprintf(buf, "%s/noReadAccessDir", sedir);
    mkdir(buf, 0750);
    sprintf(buf, "cp %s/file1.txt %s/noReadAccessDir/noReadAccessFile.txt",
	    testdatadir, sedir);
    system(buf);
    sprintf(buf, "%s/noReadAccessDir/noReadAccessFile.txt", sedir);
    chmod(buf, 0000);
    sprintf(buf, "%s/noReadAccessDir", sedir);
    chmod(buf, 0000);
    
    /* create timeDir */
    sprintf(buf, "%s/timeDir", sedir);
    mkdir(buf, 0750);

    /* get timestamps */
    globus_libc_printf("Retrieving times for modification time tests...\n");
    sprintf(buf, "%s/file1.txt", sedir);
    stat(buf, &statbuf);
    tminfo = localtime(&statbuf.st_mtime);
    sprintf(buf, "%s/filemodtime", localpath);
    f = fopen(buf, "w");
    if (!f) {
      fprintf(stderr, "Unable to create file %s\n", buf);
      return 0;
    }
    fprintf(f, "%s", asctime(tminfo));
    fclose(f);

    sprintf(buf, "%s/timeDir", sedir);
    stat(buf, &statbuf);
    tminfo = localtime(&statbuf.st_mtime);
    sprintf(buf, "%s/dirmodtime", localpath);
    f = fopen(buf, "w");
    if (!f) {
      fprintf(stderr, "Unable to create file %s\n", buf);
      return 0;
    }
    fprintf(f, "%s", asctime(tminfo));
    fclose(f);
  }
  else
  {    
    struct storageElement *se;
    digs_error_code_t result;
    char errbuf[MAX_ERROR_MESSAGE_LENGTH];

    globus_libc_printf("Using SE interface method to set up SE\n");

    /* Get SE structure */
    se = getNode(desthost);
    if (!se) {
      fprintf(stderr, "Error getting node structure for %s\n", desthost);
      return 0;
    }

    /* delete things created by the tests */
    sprintf(buf, "%s/uploadedFile", sedir);
    se->digs_rm(errbuf, desthost, buf);
    sprintf(buf, "%s/otherUploadedFile", sedir);
    se->digs_rm(errbuf, desthost, buf);
    sprintf(buf, "%s/NEW/myDir-DIR-anotherDir-DIR-file1.txt", sedir);
    se->digs_rm(errbuf, desthost, buf);
    sprintf(buf, "%s/NEW/myDir-DIR-anotherDir-DIR-largeFile.txt", sedir);
    se->digs_rm(errbuf, desthost, buf);
    
    /* copy in file1.txt, largeFile and fileForMoving.txt. set file1.txt to 640 */
    sprintf(buf, "%s/file1.txt", testdatadir);
    sprintf(buf2, "%s/file1.txt", sedir);
    copyFileToSE(se, buf, buf2);
    se->digs_setPermissions(errbuf, buf2, desthost, "0640");
    sprintf(buf2, "%s/fileForMoving.txt", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf, "%s/largeFile", testdatadir);
    sprintf(buf2, "%s/largeFile", sedir);
    copyFileToSE(se, buf, buf2);

    /* create NEW directory for inbox */
    sprintf(buf, "%s/NEW", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    
    /* create data, data1 and sub-directories */
    sprintf(buf, "%s/data", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/data/fruit", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/data/fruit/apple", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/data/fruit/apple/cooking", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/data1", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/data1/veg", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/data1/veg/root", sedir);
    se->digs_mkdir(errbuf, desthost, buf);

    sprintf(buf, "%s/file1.txt", testdatadir);
    sprintf(buf2, "%s/data/fruit/forbidden-LOCKED", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf2, "%s/data/fruit/orange", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf2, "%s/data/fruit/apple/cox", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf2, "%s/data/fruit/apple/grannySmith", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf2, "%s/data/fruit/apple/cooking/bramley", sedir);
    copyFileToSE(se, buf, buf2);

    sprintf(buf2, "%s/data1/veg/kale", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf2, "%s/data1/veg/root/carrot", sedir);
    copyFileToSE(se, buf, buf2);
    sprintf(buf2, "%s/data1/veg/root/potato", sedir);
    copyFileToSE(se, buf, buf2);


    /* create noWriteAccessDir and sub-dirs, set it to 550 */
    sprintf(buf, "%s/noWriteAccessDir", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/noWriteAccessDir/dirInsideNoWriteAccess", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    se->digs_setPermissions(errbuf, buf, desthost, "0550");
    sprintf(buf, "%s/file1.txt", testdatadir);
    sprintf(buf2, "%s/noWriteAccessDir/file1.txt", sedir);
    copyFileToSE(se, buf, buf2);
    se->digs_setPermissions(errbuf, buf2, desthost, "0440");
    sprintf(buf, "%s/noWriteAccessDir", sedir);
    se->digs_setPermissions(errbuf, buf, desthost, "0550");

    /* create noReadAccessDir and sub-dirs, set it to 000 */
    sprintf(buf, "%s/noReadAccessDir", sedir);
    se->digs_mkdir(errbuf, desthost, buf);
    sprintf(buf, "%s/file1.txt", testdatadir);
    sprintf(buf2, "%s/noReadAccessDir/noReadAccessFile.txt", sedir);
    copyFileToSE(se, buf, buf2);
    se->digs_setPermissions(errbuf, buf2, desthost, "0000");
    sprintf(buf, "%s/noReadAccessDir", sedir);
    se->digs_setPermissions(errbuf, buf, desthost, "0000");
    
    /* create timeDir */
    sprintf(buf, "%s/timeDir", sedir);
    se->digs_mkdir(errbuf, desthost, buf);

    globus_libc_printf("Retrieving times for modification time tests...\n");
    sprintf(buf, "%s/file1.txt", sedir);
    se->digs_getModificationTime(errbuf, buf, desthost, &ft);
    tminfo = localtime(&ft);
    sprintf(buf, "%s/filemodtime", localpath);
    f = fopen(buf, "w");
    if (!f) {
      fprintf(stderr, "Unable to create file %s\n", buf);
      return 0;
    }
    fprintf(f, "%s", asctime(tminfo));
    fclose(f);

    sprintf(buf, "%s/timeDir", sedir);
    se->digs_getModificationTime(errbuf, buf, desthost, &ft);
    tminfo = localtime(&ft);
    sprintf(buf, "%s/dirmodtime", localpath);
    f = fopen(buf, "w");
    if (!f) {
      fprintf(stderr, "Unable to create file %s\n", buf);
      return 0;
    }
    fprintf(f, "%s", asctime(tminfo));
    fclose(f);
  }
  return 1;
}

/*
 * If no arguments are given, the config file is assumed to be called
 * setest.conf and located in the current directory. Alternatively the
 * full path to the file can be given as an argument.
 */
int main(int argc, char *argv[])
{
  char *configFileName;
  char *localPath;

  /*
   * Process arguments
   */
  if (argc > 2) {
    globus_libc_printf("Usage: %s [<SE test config file name>]\n",
		       argv[0]);
    return 0;
  }

  if (argc == 2) {
    configFileName = argv[1];
  }
  else {
    configFileName = "setest.conf";
  }

  /*
   * Read config file
   */
  globus_libc_printf("Reading SE test config from %s...\n", configFileName);
  if (!loadConfigFile(configFileName, "setest")) {
    globus_libc_fprintf(stderr, "Error reading SE test config - not found or invalid format\n");
    return 1;
  }

  /*
   * Write node config files
   */
  globus_libc_printf("Creating nodes.conf...\n");
  if (!createNodesConf()) {
    return 1;
  }

  globus_libc_printf("Creating mainnodelist.conf...\n");
  if (!createMainNodeList()) {
    return 1;
  }

  /*
   * Activate SE interface using node config files
   */
  globus_libc_printf("Activating SE interface...\n");
  localPath = getProperty("local_file_path");
  activateGlobusModules();
  startupReplicationSystem();
  getNodeInfoFromLocalFile(localPath);

  /*
   * Create local file structure
   */
  globus_libc_printf("Creating local file structure...\n");
  if (!createLocalFileStructure(localPath)) {
    return 1;
  }

  /*
   * Create remote file structure on SE
   */
  globus_libc_printf("Creating remote file structure...\n");
  if (!createRemoteFileStructure()) {
    return 1;
  }

  globus_libc_printf("Success!\n");
  return 0;
}
