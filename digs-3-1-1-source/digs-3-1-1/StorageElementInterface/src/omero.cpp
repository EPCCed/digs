/*
 * OMERO storage element adaptor for DiGS
 *
 * To use:
 *  - the DiGS install directory should contain an "ice.config" file,
 *    which will contain the OMERO server address, username and
 *    password. May need root access on OMERO
 *  - in future may need omero-mapfile as well
 *
 * Current limitations:
 *  - only one OMERO server on a grid is supported
 *  - file transfers happen synchronously, so startPutTransfer and
 *    startGetTransfer don't return until the file has been transferred.
 *    This could be fixed by running them in a separate thread and
 *    implementing the monitor/end/cancel methods
 *  - many methods aren't implemented - only the essential ones are,
 *    and some of them don't do anything
 *  - the checksum operation might be slow as it copies the file to the
 *    local machine and then checksums it on there (OMERO doesn't support
 *    MD5)
 *  - may be memory/other resource leaks in some methods. Not sure whether
 *    the OMERO objects are intelligent enough to free themselves or not
 */

extern "C" {
  #include "node.h"
  #include "omero.h"
  #include "misc.h"
  #include "replica.h"
}

// Domain
#include <omero/client.h>
// Std
#include <iostream>
#include <cassert>
#include <vector>
#include <time.h>
#include <map>
#include <cstdio>

using namespace std;

/*
 * The name of the OMERO server - NULL if not connected yet.
 * Currently only one OMERO server on the grid is supported.
 */
static char *omeroServer_ = NULL;

// OMERO client object
static omero::client *omeroClient_ = NULL;

// OMERO service factory
static omero::api::ServiceFactoryPrx serviceFactory_;

// OMERO database interfaces
static omero::api::IAdminPrx adminService_;
static omero::api::IQueryPrx queryService_;
static omero::api::IUpdatePrx updateService_;

static int nextHandle_ = 0;

static char *filenamePrefix_ = NULL;

/*
 * Map from user certificate DNs to OMERO usernames
 */
struct userMapEntry_t
{
  char *dn;
  char *username;
};

static int userMapSize_ = 0;
static userMapEntry_t *userMap_ = NULL;

/*
 * Load the OMERO user map file
 */
static void loadOMEROMapFile()
{
  char *filename;
  if (asprintf(&filename, "%s/omero-mapfile", getQcdgridPath()) < 0) {
    logMessage(ERROR, "Out of memory in loadOMEROMapFile");
    return;
  }

  FILE *f = fopen(filename, "r");
  if (!f) {
    logMessage(WARN, "Cannot find OMERO map file");
    return;
  }

  char linebuf[500];
  int dnstart, dnend, unstart;
  while (!feof(f)) {
    fgets(linebuf, 500, f);
    if (feof(f)) break;
    removeCrlf(linebuf);

    if (linebuf[0] == '"') {
      dnstart = 1;
      char *quote = strchr(&linebuf[1], '"');
      if (!quote) {
	logMessage(WARN, "Missing quote in OMERO map file");
	continue;
      }
      dnend = quote - linebuf;

      unstart = dnend + 2; // skip quote and space
    }
    else {
      dnstart = 0;
      char *space = strchr(&linebuf[0], ' ');
      if (!space) {
	logMessage(WARN, "Missing space in OMERO map file");
	continue;
      }
      dnend = space - linebuf;

      unstart = dnend + 1;
    }

    userMapSize_++;
    userMap_ = (userMapEntry_t *)globus_libc_realloc(userMap_, userMapSize_*sizeof(userMapEntry_t));
    if (!userMap_) {
      logMessage(ERROR, "Out of memory loading OMERO map");
      fclose(f);
      return;
    }
    userMap_[userMapSize_ - 1].dn = (char *)globus_libc_malloc((dnend - dnstart) + 1);
    userMap_[userMapSize_ - 1].username = safe_strdup(&linebuf[unstart]);
    if ((!userMap_[userMapSize_-1].dn) || (!userMap_[userMapSize_-1].username)) {
      logMessage(ERROR, "Out of memory loading OMERO map");
      fclose(f);
      return;
    }
    strncpy(userMap_[userMapSize_-1].dn, &linebuf[dnstart], dnend - dnstart);
    userMap_[userMapSize_-1].dn[dnend-dnstart] = 0;
  }
  fclose(f);

  /*for (int i = 0; i < userMapSize_; i++) {
    cout << "DN: " << userMap_[i].dn << endl << "UN: " << userMap_[i].username << endl;
    }*/
}

static char *userToDN(const char *user)
{
  int i;
  for (i = 0; i < userMapSize_; i++) {
    if (!strcmp(userMap_[i].username, user)) {
      return userMap_[i].dn;
    }
  }
  return NULL;
}

static char *dnToUser(const char *dn)
{
  int i;
  for (i = 0; i < userMapSize_; i++) {
    if (!strcmp(userMap_[i].dn, dn)) {
      return userMap_[i].username;
    }
  }
  return NULL;
}

/*
 * Connect to the OMERO server for the first time (if necessary)
 */
static int omeroStartup(const char *host)
{
  if (omeroServer_ != NULL) {
    if (!strcmp(omeroServer_, host)) {
      // already initialised
      return 1;
    }

    printf("Previous OMERO server: %s\n", omeroServer_);
    printf("New OMERO server: %s\n", host);
    logMessage(ERROR, "Only one OMERO server per grid is supported");
    return 0;
  }

  loadOMEROMapFile();

  if (asprintf(&filenamePrefix_, "%s/data/", getNodePath(host)) < 0) {
    logMessage(ERROR, "Out of memory");
    return 0;
  }

  // initialise on this server
  try {
    // create OMERO client object
    int argc = 2;
    char *argv[3];
    argv[0] = "digs";
    if (safe_asprintf(&argv[1], "--Ice.Config=%s/ice.config", getQcdgridPath()) < 0) {
      logMessage(ERROR, "Out of memory");
      return 0;
    }
    argv[2] = NULL;
    omeroClient_ = new omero::client(argc, argv);
    globus_libc_free(argv[1]);

    // create service factory
    serviceFactory_ = omeroClient_->createSession();
    serviceFactory_->closeOnDestroy();

    // get database services
    adminService_ = omeroClient_->getSession()->getAdminService();
    queryService_ = omeroClient_->getSession()->getQueryService();
    updateService_ = omeroClient_->getSession()->getUpdateService();

    omeroServer_ = safe_strdup(host);
  }
  catch (omero::ServerError se) {
    logMessage(ERROR, "Error connecting to OMERO: %s", se.message.c_str());
    return 0;
  }

  return 1;
}

/*
 * Retrieves the OMERO OriginalFile object for a file
 */
static omero::model::OriginalFileIPtr getOMEROFile(const char *name)
{
  omero::model::IObjectPtr obj = queryService_->findByString("OriginalFile", "path", name);
  omero::model::OriginalFileIPtr res = omero::model::OriginalFileIPtr::dynamicCast(obj);
  return res;
}

/*
 * Converts the name from DiGS into an OMERO path
 */
static char *toOMEROName(const char *name)
{
  if (strncmp(name, filenamePrefix_, strlen(filenamePrefix_))) {
    return NULL;
  }

  return safe_strdup(&name[strlen(filenamePrefix_)]);
}

static char *getFileFormat(const char *filename)
{
  char *dot = strrchr(filename, '.');
  if (dot == NULL) return "TIFF";
  dot++;

  if ((!strcasecmp(dot, "jpg")) || (!strcasecmp(dot, "jpeg"))) {
    return "JPEG";
  }
  if ((!strcasecmp(dot, "bmp")) || (!strcasecmp(dot, "dib"))) {
    return "BMP";
  }
  if (!strcasecmp(dot, "png")) {
    return "PNG";
  }

  return "TIFF";
}

/*
 * Puts a local file to an OMERO server.
 * filePath is the OMERO path for the file
 */
static int doOMEROPut(const char *localFile, const char *filePath)
{
  FILE *f;
  long long length = getFileLength(localFile);

  f = fopen(localFile, "rb");
  if (!f) {
    logMessage(ERROR, "Cannot open local file %s", localFile);
    return 0;
  }

  try {
    /*
     * Create OMERO original file object and set properties
     */
    omero::model::OriginalFileIPtr file = new omero::model::OriginalFileI();
    //char *fmt = getFileFormat(localFile);
    char *fmt = "JPEG";
    omero::model::FormatPtr format = omero::model::FormatPtr::dynamicCast(queryService_->findByString("Format", "value", fmt));
    
    const char *name = strrchr(filePath, '/');
    if (name == NULL) name = filePath;
    else name++;
    
    omero::RStringPtr nameptr = new omero::RString(name);
    omero::RStringPtr pathptr = new omero::RString(filePath);
    omero::RStringPtr sha1ptr = new omero::RString("pending");
    omero::RLongPtr size = new omero::RLong(length);

    file->setName(nameptr);
    file->setPath(pathptr);
    file->setSha1(sha1ptr);
    file->setSize(size);
    file->setFormat(format);
    
    // upload the data
    omero::model::OriginalFileIPtr ofs = omero::model::OriginalFileIPtr::dynamicCast(updateService_->saveAndReturnObject(file));
    omero::api::RawFileStorePrx raw = omeroClient_->getSession()->createRawFileStore();
    raw->setFileId(ofs->getId()->val);

    long long offset = 0;
    long long todo;
    while (offset < length) {
      if ((length - offset) >= 1048576) {
	todo = 1048576;
      }
      else {
	todo = length - offset;
      }
      
      Ice::ByteSeq byteseq;
      for (int i = 0; i < (int)todo; i++) {
	byteseq.push_back((unsigned char)fgetc(f));
      }
      
      raw->write(byteseq, offset, length);
      offset += todo;
    }
    
    fclose(f);
  }
  catch (omero::ServerError se) {
    fclose(f);
    logMessage(ERROR, "OMERO server error: %s", se.message.c_str());
    return 0;
  }

  return 1;
}

/*
 * Retrieves an OMERO file into a local file.
 * filePath is the full DiGS path, not necessarily the OMERO path
 */
static int doOMEROGet(const char *filePath, const char *localFile)
{
  FILE *f = fopen(localFile, "wb");
  if (!f) {
    logMessage(ERROR, "Cannot create local file %s", localFile);
    return 0;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    logMessage(ERROR, "File %s not in correct path", filePath);
    return 0;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;
    
    omero::api::RawFileStorePrx raw = omeroClient_->getSession()->createRawFileStore();
    raw->setFileId(file->id->val);
    if (!raw->exists()) {
      fclose(f);
      logMessage(ERROR, "Tried to get non-existent file %s from OMERO", filePath);
      return 0;
    }
    
    long long length = file->getSize()->val;
    long long offset = 0;
    long long todo;
    while (offset < length) {
      if ((length - offset) >= 1048576) {
	todo = 1048576;
      }
      else {
	todo = length - offset;
      }
      
      Ice::ByteSeq byteseq = raw->read(offset, todo);
      for (int i = 0; i < (int)todo; i++) {
	unsigned char c1 = byteseq.at(i);
	fputc(c1, f);
      }
      
      offset += todo;
    }
    fclose(f);
  }
  catch (omero::ServerError se) {
    logMessage(ERROR, "OMERO server error: %s", se.message.c_str());
    fclose(f);
    return 0;
  }

  return 1;
}

/***********************************************************************
 *   digs_error_code_t digs_getLength_omero(char *errorMessage, 
 * 		const char *filePath,const char *hostname, long long int *fileLength)
 * 
 * 	Gets a size in bytes of a file on a node.
 * 
 *   Parameters:                                                 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 fileLength		Size of file is stored in variable fileLength, 		O
 * 					or -1 if unsuccessful
 * 
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getLength_omero(char *errorMessage, const char *filePath,
		const char *hostname, long long int *fileLength)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    *fileLength = file->getSize()->val;
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * 
 *digs_error_code_t digs_getChecksum_omero (char *errorMessage, 
 * const char *filePath,const char *hostname, char **fileChecksum,
 * digs_checksum_type_t checksumType)
 * 
 * Gets a 32 character checksum of a file on a node using the defined 
 * checksum type.
 * Hex digits will be returned in uppercase.
 *
 * Returned checksum is stored in variable fileChecksum.
 * Note that fileChecksum should be freed 
 * accordingly to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 fileChecksum	the checksum is stored in variable fileChecksum. 	O
 * 	 checksumType	the checkdigs_checksum_type_t						I
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/

digs_error_code_t digs_getChecksum_omero(char *errorMessage,
		const char *filePath, const char *hostname, char **fileChecksum,
		digs_checksum_type_t checksumType)
{
  if (checksumType != DIGS_MD5_CHECKSUM) {
    strcpy(errorMessage, "Unsupported checksum type requested");
    return DIGS_UNSUPPORTED_CHECKSUM_TYPE;
  }

  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  /*
   * OMERO can only do SHA-1, so need to copy the file to local and md5sum it
   */
  char *tmpfile = getTemporaryFile();
  if (tmpfile == NULL) {
    strcpy(errorMessage, "Creating temporary file failed");
    return DIGS_UNKNOWN_ERROR;
  }

  if (!doOMEROGet(filePath, tmpfile)) {
    unlink(tmpfile);
    globus_libc_free(tmpfile);
    strcpy(errorMessage, "Error fetching file from OMERO for checksum");
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  unsigned char cksum[16];
  computeMd5Checksum(tmpfile, cksum);

  unlink(tmpfile);
  globus_libc_free(tmpfile);

  if (safe_asprintf(fileChecksum, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
		    cksum[0], cksum[1], cksum[2], cksum[3], cksum[4], cksum[5], cksum[6], cksum[7],
		    cksum[8], cksum[9], cksum[10], cksum[11], cksum[12], cksum[13], cksum[14], cksum[15]) < 0) {
    strcpy(errorMessage, "Out of memory");
    return DIGS_UNKNOWN_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_isDirectory_omero (char *errorMessage,
 * const char *filePath, const char *hostname, int *isDirectory)
 * 
 * Checks if a file on a node is a directory.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 isDirectory	1 if the path is to a directory, else 0				O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_isDirectory_omero(char *errorMessage,
		const char *filePath, const char *hostname, int *isDirectory)
{
  // Not used yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 * digs_error_code_t digs_doesExist_omero (char *errorMessage,
 * const char *filePath, const char *hostname, int *doesExist)
 * 
 * Checks if a file/directory exists on a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 doesExist		1 if the specified file exists, else 0				O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_doesExist_omero(char *errorMessage,
		const char *filePath, const char *hostname, int *doesExist)
{
  // Not used yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 * digs_error_code_t digs_ping_omero(char *errorMessage, const char * hostname);
 * 
 * Pings node and checks that it has a data directory.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I								
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_ping_omero(char *errorMessage, const char * hostname)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  // do a scan to check it's responding
  char **list;
  int listlength;

  digs_error_code_t result = digs_scanNode_omero(errorMessage, hostname,
						 &list, &listlength, 0);
  digs_free_string_array_omero(&list, &listlength);

  return result;
}


/***********************************************************************
 *   void digs_free_string_omero(char **string) 
 *
 * 	 Free string
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 string 		the string to be freed								I
 ***********************************************************************/
void digs_free_string_omero(char **string)
{
  globus_libc_free(*string);
  *string = NULL;
}

/***********************************************************************
 *  digs_error_code_t digs_getOwner_omero(char *errorMessage, const char 
 * *filePath, const char *hostname, char **ownerName);
 * 
 * Gets the owner of the file.
 *
 * Returned owner is stored in variable ownerName.
 * Note that owner should be freed 
 * accordingly to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                                	 	[I/O]
 *	
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)				O
 * 	 filePath 		the full path to the file								I
 *   hostname  		the FQDN of the host to contact          				I
 * 	 ownerName		the owner of the file is stored in variable ownerName. 	O
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getOwner_omero(char *errorMessage,
		const char *filePath, const char *hostname, char **ownerName)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    omero::model::DetailsPtr details = file->getDetails();
    omero::model::ExperimenterPtr owner = details->owner;
    omero::RLongPtr ownerid = owner->id;
    owner = adminService_->getExperimenter(ownerid->val);

    char *dn = userToDN(owner->omeName->val.c_str());
    if (dn == NULL) {
      sprintf(errorMessage, "No DN mapping found for user %s", owner->omeName->val.c_str());
      return DIGS_AUTH_FAILED;
    }

    *ownerName = safe_strdup(dn);
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_getGroup_omero(char *errorMessage,
 * const char *filePath, const char *hostname, char **groupName);
 * 
 * Gets the group of a file on a node.
 *
 * Returned group name is stored in variable groupName.
 * Note that groupName should be freed 
 * accordingly to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 groupName		the group name of the file is stored in variable 	0
 * 					groupName. 									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getGroup_omero(char *errorMessage,
		const char *filePath, const char *hostname, char **groupName)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    omero::model::DetailsPtr details = file->getDetails();
    omero::model::ExperimenterGroupPtr group = details->group;
    omero::RLongPtr groupid = group->id;
    group = adminService_->getGroup(groupid->val);
    *groupName = safe_strdup(group->name->val.c_str());
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_setGroup_omero(char *errorMessage, 
 * const char *filePath, const char *hostname, char *groupName);
 * 
 * Sets up/changes a group of a file/directory on a node (chgrp).
 * 
 * Relies on chgrp command existing on the storage element.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 groupName		the group name of the file is stored in variable 	I
 * 					groupName. 									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_setGroup_omero(char *errorMessage,
		const char *filePath, const char * hostname, const char *groupName)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    // FIXME: is it safe to assume that group will have same name on OMERO server?
    adminService_->changeGroup(file, groupName);
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_getPermissions_omero(char *errorMessage,
 *		const char *filePath, const char *hostname, char **permissions);
 * 
 * Gets the permissions of a file on a node.
 *
 * Returned permissions are stored in variable permissions, in a numberic
 * representation. For example 754 would mean:	
 * owner: read, write and execute permissions,
 * group: read and execute permissions,
 * others: only read permissions. 
 * 
 * Note that permissions should be freed 
 * according to the way it was allocated in the implementation.
 * Use digs_free_string to free it.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 permissions	the permissions of the file/dir is stored in 
 * 					variable permissions in decimal fomat.				0									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getPermissions_omero(char *errorMessage,
		const char *filePath, const char *hostname,  char **permissions)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    omero::model::DetailsPtr details = file->getDetails();
    omero::model::PermissionsPtr perms = details->permissions;
    *permissions = safe_strdup("0000");
    if (perms->isUserRead()) (*permissions)[1] += 4;
    if (perms->isUserWrite()) (*permissions)[1] += 2;
    if (perms->isGroupRead()) (*permissions)[2] += 4;
    if (perms->isGroupWrite()) (*permissions)[2] += 2;
    if (perms->isWorldRead()) (*permissions)[3] += 4;
    if (perms->isWorldWrite()) (*permissions)[3] += 2;
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_setPermissions_omero(char *errorMessage, 
 * const char *filePath, const char *hostname, const char *permissions);
 * 
 * Sets up/changes permissions of a file/directory on a node. 
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 permissions	the permissions of the file/dir is stored in 
 * 					variable permissions in octal fomat.				I									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_setPermissions_omero(char *errorMessage,
		const char *filePath, const char *hostname, const char *permissions)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    omero::model::DetailsPtr details = file->getDetails();
    omero::model::PermissionsPtr perms = details->permissions;
 
    /*
     * Permissions that come in are in the form of an octal string, e.g. "0640" or "0644"
     */
    int ownerperm = permissions[1] - '0';
    int groupperm = permissions[2] - '0';
    int worldperm = permissions[3] - '0';
    
    if (ownerperm & 4) perms->setUserRead(true);
    else perms->setUserRead(false);
    if (ownerperm & 2) perms->setUserWrite(true);
    else perms->setUserWrite(false);
    
    if (groupperm & 4) perms->setGroupRead(true);
    else perms->setGroupRead(false);
    if (groupperm & 2) perms->setGroupWrite(true);
    else perms->setGroupWrite(false);

    if (worldperm & 4) perms->setWorldRead(true);
    else perms->setWorldRead(false);
    if (worldperm & 2) perms->setWorldWrite(true);
    else perms->setWorldWrite(false);
    
    adminService_->changePermissions(file, perms);
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 *  digs_error_code_t digs_getModificationTime_omero(char *errorMessage, 
 *		const char *filePath, const char *hostname, 
 * 		time_t *modificationTime);
 * 
 *     Returns the modification time of a file located on a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 filePath 		the full path to the file							I
 *   hostname  		the FQDN of the host to contact          			I
 * 	 modificationTime	the time of the last modification of the file	0									
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_getModificationTime_omero(char *errorMessage,
		const char *filePath, const char *hostname,
		time_t *modificationTime)
{
  // Not used except with inbox yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

///***********************************************************************
//* partially implemented get_list.
// ***********************************************************************/
//void digs_getList_omero(struct digs_result_t *result,
//		const char *filePath, const char *hostname, time_t *fileTime,
//		digs_fileTimeType_t *fileTimeType);

/***********************************************************************
 *digs_error_code_t digs_startPutTransfer_omero(char *errorMessage,
 *		const char *localPath, const char *hostname, const char *SURL,
 *		int *handle);
 *
 * Start a put operation (push) of a local file to a remote hostname with a 
 * specified remote location (SURL). A handle is returned to uniquely 
 * identify this transfer.
 * 
 * If a file with the chosen name already exists at the remote location, it will 
 * be overwriten without a warning.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 SURL 			the remote location to put the file to				I	
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startPutTransfer_omero(char *errorMessage,
		const char *hostname, const char *localPath, const char *SURL, int *handle)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  // just do this synchronously for now
  char *path = toOMEROName(SURL);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", SURL);
    return DIGS_FILE_NOT_FOUND;
  }
  if (!doOMEROPut(localPath, path)) {
    strcpy(errorMessage, "Error uploading file to OMERO");
    globus_libc_free(path);
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  // now get file owner and set it in OMERO
  char *ownerdn = getRLSAttribute(path, "submitter");
  if (ownerdn != NULL) {
    char *username = dnToUser(ownerdn);
    if (username != NULL) {
      try {
	omero::model::OriginalFileIPtr file = getOMEROFile(path);
	adminService_->changeOwner(file, username);
      }
      catch (omero::ServerError se) {
	snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "Error setting file owner to %s",
		 username);
	globus_libc_free(ownerdn);
	globus_libc_free(path);
	return DIGS_AUTH_FAILED;
      }
    }
    globus_libc_free(ownerdn);
  }

  globus_libc_free(path);

  *handle = nextHandle_++;

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 *digs_error_code_t digs_startCopyToInbox_omero(char *errorMessage,
 *		const char *hostname, const char *localPath, , const char *lfn,
 *		int *handle) 
 * 
 * This method is effectively a wrapper around digs_startPutTransfer()
 * that firstly resolves the target SURL based on the specific 
 * configuration of the SE (identified by its hostname). Not all
 * SEs have Inboxes. If this function is called on an SE that does not
 * have an Inbox, then an appropriate error is returned.
 *
 * A handle is returned to uniquely identify this transfer.
 * 
 * If a file with the chosen name already exists at the remote location, it will 
 * be overwriten without a warning.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 *   lfn    		Logical name for file on grid          			    I
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startCopyToInbox_omero(char *errorMessage,
		const char *hostname, const char *localPath, const char *lfn,
		int *handle)
{
  // No inbox on OMERO
  errorMessage[0] = 0;
  return DIGS_NO_INBOX;
}

/***********************************************************************
 *digs_error_code_t digs_monitorTransfer_omero (char *errorMessage, int handle,
 * digs_transferStatus_t *status, int *percentComplete);
 * 
 * Checks a progress of a transfer, identified by a handle, by
 * returning its status. Also returns the percentage of the transfer that has 
 * been completed.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage		an error description string	(expects to have
 * 						MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 			the id of the transfer								I
 * 	 status				the status of the transfer							O	
 * 	 percentComplete	percentage of the transfer that has been completed
 * 						In this implementation goes from 0 directly to 100	O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_monitorTransfer_omero(char *errorMessage, int handle, 
		digs_transfer_status_t *status, int *percentComplete)
{
  /*
   * We do the whole transfer synchronously in the put/get function, so just return success here
   */
  *status = DIGS_TRANSFER_DONE;
  *percentComplete = 100;

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_endTransfer_omero(char *errorMessage, int handle)
 * 
 * Cleans up a transfer identified by a handle. Checks that the file 
 * has been transferred correctly (using checksum) and removes the 
 * LOCKED postfix if it has. If the transfer has not completed 
 * correctly, removes partially transferred files.
 *
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 		the id of the transfer								I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_endTransfer_omero(char *errorMessage, int handle)
{
  // nothing to do, transfers are synchronous
  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_cancelTransfer_omero(char *errorMessage, int handle)
 * 
 * Cancels and cleans up a transfer identified by a handle. Removes any
 * partially or completely transferred files.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 * 	 handle 		the id of the transfer								I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_cancelTransfer_omero(char *errorMessage, int handle)
{
  // nothing to do, transfers are synchronous
  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_mkdir_omero(char *errorMessage, const char *hostname,
		const char *filePath);
 * 
 * Creates a directory on a node.
 * Will return an error if the directory already exists.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePath 		the path of the directory to be created				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mkdir_omero(char *errorMessage, const char *hostname,
		const char *filePath)
{
  // Not used yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 * digs_error_code_t digs_mkdirtree_omero(char *errorMessage, 
 * const char *hostname, const char *filePath);
 * 
 * Creates a tree of directories at one go on a node 
 * (e.g. 'mkdir lnf:/ukqcd/DWF')
 * Will return an error if the directory already exists.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePath 		the path of the directory to be created				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mkdirtree_omero(char *errorMessage,
		const char *hostname, const char *filePath )
{
  // Not used except with inbox yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 *digs_error_code_t digs_startGetTransfer_omero(char *errorMessage,
 *		const char *hostname, const char *SURL, const char *localPath,
 *		int *handle);
 *
 * Starts a get operation (pull) of a file (SURL) from a  
 * remote hostname to local destination file path. A handle is
 * returned to uniquely identify this transfer.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 SURL 			the remote location to get the file from			I	
 * 	 localPath 		the full path to the local location to get 
 * 					the file from										I
 * 	 handle 		the id of the transfer								O					
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_startGetTransfer_omero(char *errorMessage,
		const char *hostname, const char *SURL, const char *localPath,
		int *handle)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  // do it synchronously for now
  char *path = toOMEROName(SURL);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", SURL);
    return DIGS_FILE_NOT_FOUND;
  }
  if (!doOMEROGet(path, localPath)) {
    strcpy(errorMessage, "Error getting file from OMERO");
    globus_libc_free(path);
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }
  globus_libc_free(path);

  *handle = nextHandle_++;

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_mv_omero(char *errorMessage, const char *hostname, 
 * const char *filePathFrom, const char *filePathTo);
 * 
 * Moves/Renames a file on an SE (hostname). 
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePathFrom	the path to get the file from						I	
 * 	 filePathTo		the path put the file to							I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_mv_omero(char *errorMessage, const char *hostname, 
  const char *filePathFrom, const char *filePathTo)
{
  // Not used yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 * digs_error_code_t digs_rm_omero(char *errorMessage, const char *hostname, 
 * const char *filePath);
 * 
 * Deletes a file from a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 filePath 		the path of the directory to be removed				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rm_omero(char *errorMessage, const char *hostname,
		const char *filePath)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  char *path = toOMEROName(filePath);
  if (path == NULL) {
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "File %s not in correct path", filePath);
    return DIGS_FILE_NOT_FOUND;
  }
  try {
    omero::model::OriginalFileIPtr file = getOMEROFile(path);
    globus_libc_free(path);
    path = NULL;

    updateService_->deleteObject(file);
  }
  catch (omero::ServerError se) {
    globus_libc_free(path);
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }

  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_rmdir_omero(char *errorMessage, const char *hostname, 
 * const char *filePath);
 * 
 * Deletes a directory from a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 dirPath 		the path of the directory to be removed				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rmdir_omero(char *errorMessage, const char *hostname, 
  const char *dirPath)
{
  // Not used yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 * digs_error_code_t digs_rmr_omero(char *errorMessage, const char *hostname, 
 * const char *filePath);
 * 
 * Deletes a directory with subdirectories (recursively) from a node.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 * 	 dirPath 		the path of the directory to be removed				I			
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_rmr_omero(char *errorMessage, const char *hostname,
		const char *filePath)
{
  // Not used yet
  errorMessage[0] = 0;
  return DIGS_NO_SERVICE;
}

/***********************************************************************
 *digs_error_code_t digs_copyFromInbox_omero(char *errorMessage, 
 * const char *hostname, const char *lfn, const char *targetPath);
 * 
 * The function is effectively a wrapper to digs_mv operation.
 * 
 * Moves a file (identified by lfn) from the Inbox folder to
 * permanent storage on an SE (identified by hostname), with a 
 * specified destination file path. 
 * Not all SEs have Inboxes. If this function is called on an SE that 
 * does not have an Inbox, then an appropriate error is returned.
 * 
 * If a file with the chosen name already exists at the remote location, it 
 * will be overwriten without a warning.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 *   lfn    		Logical name for file on grid          			    I
 * 	 targetPath		the full path to the SE location to put the 
 * 					file to.											I		
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_copyFromInbox_omero(char *errorMessage,
		const char *hostname, const char *lfn, const char *targetPath)
{
  // No inbox on OMERO
  errorMessage[0] = 0;
  return DIGS_NO_INBOX;
}

/***********************************************************************
 * digs_error_code_t digs_scanNode_omero(char *errorMessage, 
 * const char *hostname, char ***list, int *listLength, int allFiles);
 * 
 * Gets a list of all the DiGS file locations. Recursive directory
 * listing of all the DiGS data files
 * Setting the allFiles flag to zero will hide any temporary (locked) 
 * files.
 *
 * Returned an array of the file paths is stored in variable list. Note    
 * that list should be freed accordingly to the way it was allocated in 
 * the implementation! Use digs_free_string_array.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 *   list    		An array of the filepaths below the top dir. 	    O	
 *   listLength   	The length of the array of filepaths			    O	
 * 	 allFiles		Show all files or hide any temporary (locked files) I	
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_scanNode_omero(char *errorMessage,
		const char *hostname, char ***list, int *listLength, int allFiles)
{
  if (!omeroStartup(hostname)) {
    strcpy(errorMessage, "Error connecting to OMERO");
    return DIGS_NO_CONNECTION;
  }

  try {
    omero::api::IObjectList olist = queryService_->findAll("OriginalFile", NULL);

    *listLength = olist.size();
    *list = (char **)globus_libc_malloc(olist.size() * sizeof(char *));
    if (!(*list)) {
      strcpy(errorMessage, "Out of memory");
      return DIGS_UNKNOWN_ERROR;
    }
    
    for (unsigned int i = 0; i < olist.size(); i++) {
      omero::model::OriginalFileIPtr file = omero::model::OriginalFileIPtr::dynamicCast(olist.at(i));
      char *fn;
      if (asprintf(&fn, "%s%s", filenamePrefix_, file->getPath()->val.c_str()) < 0) {
	strcpy(errorMessage, "Out of memory");
	return DIGS_UNKNOWN_ERROR;
      }
      (*list)[i] = fn;
    }
  }
  catch (omero::ServerError se) {
    // FIXME: memory leaks on error here
    snprintf(errorMessage, MAX_ERROR_MESSAGE_LENGTH, "OMERO error: %s", se.message.c_str());
    return DIGS_UNSPECIFIED_SERVER_ERROR;
  }
  
  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

/***********************************************************************
 * digs_error_code_t digs_scanInbox_omero(char *errorMessage, 
 * const char *hostname, char ***list, int *listLength, int allFiles);
 * 
 * Gets a list of all the DiGS file locations in the inbox. Recursive directory
 * listing of all the files under inbox.
 * Setting the allFiles flag to zero will hide any temporary (locked) 
 * files.
 *
 * Returned an array of the file paths is stored in variable list. Note    
 * that list should be freed accordingly to the way it was allocated in 
 * the implementation! Use digs_free_string_array.
 * 
 *   Parameters:                                                	 [I/O]
 *
 * 	 errorMessage	an error description string	(expects to have
 * 					MAX_ERROR_MESSAGE_LENGTH assigned already)			O
 *   hostname  		the FQDN of the host to contact          			I	
 *   list    		An array of the filepaths below the top dir. 	    O	
 *   listLength   	The length of the array of filepaths			    O	
 * 	 allFiles		Show all files or hide any temporary (locked files) I	
 *    
 *   Returns: A DiGs error code (DIGS_SUCCESS if successful).
 ***********************************************************************/
digs_error_code_t digs_scanInbox_omero(char *errorMessage,
		const char *hostname, char ***list, int *listLength, int allFiles)
{
  // No inbox on OMERO
  errorMessage[0] = 0;
  return DIGS_NO_INBOX;
}

/***********************************************************************
 * 	void digs_free_string_array_omero(char ***arrayOfStrings, int *listLength);
 *
 * 	Free an array of strings according to the correct implementation.
 * 
 *   Parameters:                                                 [I/O]	
 * 
 * 	 arrayOfStrings The array of strings to be freed	 		  I	
 *   listLength   	The length of the array of filepaths		  I/O
 ***********************************************************************/
void digs_free_string_array_omero(char ***arrayOfStrings, int *listLength)
{
  int i;
  for (i = 0; i < *listLength; i++) {
    globus_libc_free(*arrayOfStrings[i]);
    *arrayOfStrings[i] = NULL;
  }
  globus_libc_free(*arrayOfStrings);
  *arrayOfStrings = NULL;
  *listLength = 0;
}

/***********************************************************************
 * void digs_housekeeping_omero(char *errorMessage, char *hostname);
 *
 * Perform periodic housekeeping tasks (currently just delete stray
 * -LOCKED files)
 * 
 * Parameters:                                                 [I/O]	
 * 
 *   errorMessage  Receives error message if an error occurs      O
 *   hostname      Name of host to perform housekeeping on      I
 *
 * Returns DIGS_SUCCESS on success or an error code on failure.
 ***********************************************************************/
digs_error_code_t digs_housekeeping_omero(char *errorMessage, char *hostname)
{
  // don't do anything yet, won't have -LOCKED files on OMERO
  errorMessage[0] = 0;
  return DIGS_SUCCESS;
}

