/***********************************************************************
 *
 *   Filename:   globusSETest.c
 *
 *   Authors:    Eilidh Grant     (egrant1)   EPCC.
 *               James Perry      (jamesp)
 *
 *   Purpose:    Tests the methods on the storage elements.
 *
 *   Contents:   Unit tests.
 *
 *   Used in:    Called by runAllTests.c
 *
 *   Contact:    epcc-support@epcc.ed.ac.uk
 *
 *   Copyright (c) 2008-2010 The University of Edinburgh
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

#include "CuTest.h"
#include <globus_ftp_client.h>
#include "gridftp.h"
#include "misc.h"
#include "node.h"
#include "config.h"
#include <time.h>

char *valid_se_path;

static char *good_filepath_;
static char *large_good_filepath_;
static char *good_filepath_for_mv;

static char *node_list_dir_;

static char *non_existant_filepath_;
static char *no_read_access_path_;
static char *no_write_access_path_;
static char *no_write_access_empty_dir_;
static char *no_write_access_filepath_;
static char *no_write_access_file_;
static char *time_dir_;
static char *path_to_dir_;
static char *path_to_made_dir;
static char *path_to_file_in_new_dir;
static char *path_to_other_file_in_new_dir;
static char *dir_tree;
static char *file_in_dir_tree;
static char *middle_of_dir_tree;
char *file1lfn = "myDir/anotherDir/file1.txt";



/*MD5 checksum of file1 in testData*/
char *file1Checksum =     "24A14A0DACFA3C052A48B8A2672E8701";
/*MD5 checksum of largeFile in testData*/
char *largeFileChecksum = "48DAF5AC9BEA9A1A237562BE28030DA3";

/* owner of the file */
char *fileOwner;
/* owner of the file */
char *fileGroup;
char *otherGroup;
char *otherGroupThatUserIsNotMemberOf;

char *fileForUpload;
char *largeFileForUpload;
char *pathToStoreFile;
char *pathToStoreFileLocked;
char *pathToStoreOtherFile;
char *pathToStoreFileNoDir;
char *pathToNoDir;
char *pathToDownloadFileTo_;
char *pathToDownloadFileToLocked_;
char *pathToDownloadOtherFileTo_;
char *pathToDownloadOtherFileToLocked_;
char *no_write_access_pathToDownloadFileTo_;

char expectedFileTime[100];
char expectedDirTime[100];

/*Storage element good_se_ is running globus*/
static char *good_se_;

static char *good_se_type_;

/*Storage element no_globus_se_ is a node that is not running globus*/
static char *no_globus_se_ = "garnet.epcc.ed.ac.uk";
/*Storage element not_a_node_se_ is not a node */
static char *not_a_node_se_ = "ral";
/*Storage element doesn't have an inbox. */
static char *no_inbox_se_;

struct digs_result_t result_; 

/* The error description */
char error_description_[MAX_ERROR_MESSAGE_LENGTH+1];

char *makestring(char *s1, char *s2)
{
  char *result = NULL;
  if (safe_asprintf(&result, "%s%s", s1, s2) < 0) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }
  return result;
}

/*The test data must have been uploaded to the storage element.*/
void setup(void);

void initTest(void);

void checkResult(CuTest *tc, digs_error_code_t expected_error_code,
		digs_error_code_t actual_error_code) {

	printf("expected_error_code %d\n", expected_error_code);
	printf("actual_error_code %d\n", actual_error_code);
	printf("digs expected_error_code description %s\n", digsErrorToString(expected_error_code));
	printf("digs actual_error_code description %s\n", digsErrorToString(actual_error_code));

	printf("error message %s\n", error_description_);
	
	CuAssertStrEquals(tc, digsErrorToString(expected_error_code),
			digsErrorToString(actual_error_code));
	CuAssertIntEquals(tc, expected_error_code, actual_error_code);

}

void TestGetLengthSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	long long int expected = 27; /*length of file1 in testData*/
	long long int actual = 0;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getLength(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetLengthNoRemoteFile(CuTest *tc) {
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	long long int expected = -1; /*return -1 if file is not found.*/
	long long int actual = 0;
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getLength(error_description_, filepath, se.name, &actual);

	/* Would prefer DIGS_FILE_NOT_FOUND but the current globus implementation 
	 * doesn't give that much detail. */
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetLengthNoReadPermissionsOnRemoteDir(CuTest *tc) {
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	long long int expected = -1;
	long long int actual = 0;
	char *filepath = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getLength(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetLengthFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	long long int expected = -1;
	long long int actual = 0;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getLength(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_FILE_IS_DIR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetLengthNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	long long int expected = -1; /*return -1 if file is not found.*/
	long long int actual = 0;
	char *filepath = good_filepath_;
	char errorMessage[MAX_ERROR_MESSAGE_LENGTH];
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getLength(errorMessage, filepath, se.name, &actual);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetLengthNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	long long int expected = -1; /*return -1 if node is not found.*/
	long long int actual = 0;
	char *filepath = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getLength(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetChecksumSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	/*MD5 checksum of file1 in testData*/
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	se.digs_free_string(actualChecksum);
}

void TestFreeString(CuTest *tc) {

	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	se.digs_getChecksum(error_description_, filepath, se.name, actualChecksum,
			DIGS_MD5_CHECKSUM);

	se.digs_free_string(actualChecksum);

	CuAssertTrue(tc, *actualChecksum == NULL);
}

void TestGetChecksumNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedChecksum = ""; /* Return empty string if checksum fails. */
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	
	se.digs_free_string(actualChecksum);
}

void TestGetChecksumNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedChecksum = ""; /* Return empty string if checksum fails. */
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath  = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	
	se.digs_free_string(actualChecksum);
}

void TestGetChecksumFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedChecksum = ""; /* Return empty string if checksum fails. */
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath  = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_FILE_IS_DIR, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	se.digs_free_string(actualChecksum);
}

void TestGetChecksumNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedChecksum = ""; /* Return empty string if checksum fails. */
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath  = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	se.digs_free_string(actualChecksum);
}


void TestGetChecksumNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedChecksum = ""; /* Return empty string if checksum fails. */
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath  = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	se.digs_free_string(actualChecksum);
}

void TestGetChecksumUnsupportedType(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	/*MD5 checksum of file1 in testData*/
	char *expectedChecksum = "";
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;
	char *filepath  = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getChecksum(error_description_, filepath, se.name,
			actualChecksum, DIGS_CRC_CHECKSUM);

	checkResult(tc, DIGS_UNSUPPORTED_CHECKSUM_TYPE, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	se.digs_free_string(actualChecksum);
}

void TestDoesExistSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 1;
	int actual = 0;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestDoesNotExistSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestDoesExistNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestDoesExistFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 1; 
	int actual = 0;
	char *filepath  = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestDoesExistNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, expected, actual );
}
void TestDoesExistNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = no_read_access_path_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestIsDirectorySuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 1;
	int actual = 0;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_isDirectory(error_description_, filepath, se.name,
					&actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
	
}
void TestIsNotDirectorySuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_isDirectory(error_description_, filepath, se.name,
					&actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
}


void TestIsDirectoryNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);


	result = se.digs_isDirectory(error_description_, filepath, se.name,
					&actual);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestIsDirectoryNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_isDirectory(error_description_, filepath, se.name,
					&actual);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestIsDirectoryNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_isDirectory(error_description_, filepath, se.name,
					&actual);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestIsDirectoryNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 0; 
	int actual = 1;
	char *filepath  = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_isDirectory(error_description_, filepath, se.name,
					&actual);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestGetOwnerSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedOwner = fileOwner;
	char *unknownOwner= "unknown owner";
	char **actualOwner = &unknownOwner;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getOwner(error_description_, filepath, se.name,
			actualOwner);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedOwner, *actualOwner );
	
	se.digs_free_string(actualOwner);
}

void TestGetOwnerNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedOwner = "";/* Return empty string if getOwner fails. */
	char *unknownOwner= "unknown owner";
	char **actualOwner = &unknownOwner;
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getOwner(error_description_, filepath, se.name,
			actualOwner);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedOwner, *actualOwner );
	
	se.digs_free_string(actualOwner);
}

void TestGetOwnerNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedOwner = "";/* Return empty string if getOwner fails. */
	char *unknownOwner= "unknown owner";
	char **actualOwner = &unknownOwner;
	char *filepath = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getOwner(error_description_, filepath, se.name,
			actualOwner);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedOwner, *actualOwner );
	
	se.digs_free_string(actualOwner);
}

void TestGetOwnerFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedOwner = fileOwner;
	char *unknownOwner= "unknown owner";
	char **actualOwner = &unknownOwner;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getOwner(error_description_, filepath, se.name,
			actualOwner);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedOwner, *actualOwner );
	
	se.digs_free_string(actualOwner);
}

void TestGetOwnerNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedOwner = "";/* Return empty string if getOwner fails. */
	char *unknownOwner= "unknown owner";
	char **actualOwner = &unknownOwner;
	char *filepath = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getOwner(error_description_, filepath, se.name,
			actualOwner);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedOwner, *actualOwner );
	
	se.digs_free_string(actualOwner);
}
void TestGetOwnerNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedOwner = "";/* Return empty string if getOwner fails. */
	char *unknownOwner= "unknown owner";
	char **actualOwner = &unknownOwner;
	char *filepath = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getOwner(error_description_, filepath, se.name,
			actualOwner);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedOwner, *actualOwner );
	
	se.digs_free_string(actualOwner);
}

void TestGetGroupSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = fileGroup;
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestGetGroupNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup =  "";/* Return empty string if getGroup fails. */
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestGetGroupNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup =  "";/* Return empty string if getGroup fails. */
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);


	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestGetGroupFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = fileGroup;
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestGetGroupNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup =  "";/* Return empty string if getGroup fails. */
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestGetGroupNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup =  "";/* Return empty string if getGroup fails. */
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestSetGroupSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *originalGroup = fileGroup;
	char *expectedGroup = otherGroup;
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = large_good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	/* reset group*/
	result = se.digs_setGroup(error_description_, filepath, se.name,
			originalGroup);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* Check change was applied. */
	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);
}

void TestSetGroupNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = otherGroup;
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestSetGroupGroupDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = "notAGroup";
	char *filepath = large_good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_UNKNOWN_ERROR, result);
}

void TestSetGroupNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = otherGroup;
	char *filepath = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestSetGroupNoPermissionsToAddToGroup(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = otherGroupThatUserIsNotMemberOf;
	char *filepath = large_good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_UNKNOWN_ERROR, result);
}

void TestSetGroupNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = otherGroup;
	char *filepath = large_good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_UNKNOWN_ERROR, result);
}

void TestSetGroupNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedGroup = otherGroup;
	char *filepath = large_good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_UNKNOWN_ERROR, result);
}

void TestSetGroupFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *originalGroup = fileGroup;
	char *expectedGroup = otherGroup;
	char *unknownGroup= "unknown group";
	char **actualGroup = &unknownGroup;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	/* reset group*/
	result = se.digs_setGroup(error_description_, filepath, se.name,
			originalGroup);

	/* Change group */
	result = se.digs_setGroup(error_description_, filepath, se.name,
			expectedGroup);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* Check change was applied. */
	result = se.digs_getGroup(error_description_, filepath, se.name,
			actualGroup);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedGroup, *actualGroup );
	
	se.digs_free_string(actualGroup);

	/* reset group*/
	result = se.digs_setGroup(error_description_, filepath, se.name,
			originalGroup);
}

void TestGetPermissionsSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "0640";/* -rw-r-----  */
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
	
	se.digs_free_string(actualPermissions);
}

void TestGetPermissionsNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "";
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
	se.digs_free_string(actualPermissions);
}

void TestGetPermissionsNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "";
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = no_read_access_path_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
}

void TestGetPermissionsFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "0775";/* drwxrwxr-x  */
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
	se.digs_free_string(actualPermissions);
}

void TestGetPermissionsNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "";
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
}

void TestGetPermissionsNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "";
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
}

void TestSetPermissionsSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *originalPermissions = "0640";/* -rw-r-----  */
	char *expectedPermissions = "0764";/* -rwxrw-r--  */
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = large_good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			expectedPermissions);
	checkResult(tc, DIGS_SUCCESS, result);

	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions );
	
	se.digs_free_string(actualPermissions);
	
	// revert to original permissions.
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			originalPermissions);
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestSetPermissionsNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "0764";/* -rwxrw-r--  */
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			expectedPermissions);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestSetPermissionsNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "0764";/* -rwxrw-r--  */
	char *filepath = no_write_access_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			expectedPermissions);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestSetPermissionsOnDir(CuTest *tc) {


	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *originalPermissions = "0775";
	char *expectedPermissions = "0640";
	char *noPermissions = "no permissions set";
	char **actualPermissions= &noPermissions;
	char *filepath = path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			expectedPermissions);
	checkResult(tc, DIGS_SUCCESS, result);

	result = se.digs_getPermissions(error_description_, filepath, se.name,
			actualPermissions);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedPermissions, *actualPermissions);
	
	// revert to original permissions.
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			originalPermissions);
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestSetPermissionsNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "0764";/* -rwxrw-r--  */
	char *filepath = large_good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			expectedPermissions);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}
void TestSetPermissionsNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *expectedPermissions = "0764";/* -rwxrw-r--  */
	char *filepath = large_good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_setPermissions(error_description_, filepath, se.name,
			expectedPermissions);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestPutTransferSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *SURL = pathToStoreFile;
	int handle = -1;
	char *storage_element = good_se_;
	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has previously been created
	result = se.digs_rm(error_description_, se.name, SURL);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 50;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		if (se.digs_monitorTransfer(error_description_, handle, &status,
				&progress)!=DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);
	/* cleanup test.*/
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);

	/* check that file has been loaded correctly onto the remote machine. */

	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
}

void TestEndTransferChecksChecksum(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *SURL = pathToStoreFile;
	int handle = -1;
	char *storage_element = good_se_;
	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	/*
	 * This test doesn't work on SRM as there is no way to replace an
	 * uploaded file with a different one
	 */
	if (!strcmp(good_se_type_, "srm")) {
	  return;
	}

	// remove remote file if it has previously been created
	result = se.digs_rm(error_description_, se.name, SURL);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		if (se.digs_monitorTransfer(error_description_, handle, &status,
				&progress)!=DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	/* Replace uploaded file with another with the wrong checksum. */
	int otherHandle = 0;
	result = se.digs_startPutTransfer(error_description_, se.name,
			largeFileForUpload, pathToStoreFileLocked, &otherHandle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", otherHandle != -1);

	/* wait for update tranfer to complete */
	startTime = time(NULL);
	endTime = time(NULL);
	
	do {
		if (se.digs_monitorTransfer(error_description_, otherHandle, &status,
				&progress)!=DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	result = se.digs_endTransfer(error_description_, otherHandle);
	checkResult(tc, DIGS_SUCCESS, result);

	/*End of replace uploaded file with another with the wrong checksum. */
	
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_INVALID_CHECKSUM, result);

	/* check that locked file with wrong checksum has been removed from the remote machine. */
	int doesExist = 0;
	/* Check locked file removed. */
	se.digs_doesExist(error_description_, pathToStoreFileLocked, se.name, &doesExist);
	CuAssertIntEquals_Msg(tc, "Locked file was not removed. ", 0, doesExist);
	
	/* Check unlocked file has not been created. */
	se.digs_doesExist(error_description_, SURL, se.name, &doesExist);
	CuAssertIntEquals_Msg(tc, "Unlocked file shouldn't have been created when checksum is wrong.", 0, doesExist);
	
} 

void TestPutTransferCancelled(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = largeFileForUpload;
	char *SURL = pathToStoreFile;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	// remove remote file if it has previously been created
	result = se.digs_rm(error_description_, se.name, SURL);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait a little for transfer to start. */
	time_t startTime;
	time_t endTime;
	float timeOut = 10;
	startTime = time(NULL);
	endTime = time(NULL);

	do {//just killing time here
		endTime = time(NULL);
	} while (difftime(endTime, startTime) < timeOut);
	
	/* cancel the transfer */
	result = se.digs_cancelTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* file should not exist on the remote machine. */

	/* check that locked file with wrong checksum has been removed from the remote machine. */
	int doesExist = 0;
	/* Check locked file removed. */
	se.digs_doesExist(error_description_, pathToStoreFileLocked, se.name, &doesExist);
	CuAssertIntEquals_Msg(tc, "Locked file was not removed. ", 0, doesExist);
	
	/* Check unlocked file has not been created. */
	se.digs_doesExist(error_description_, SURL, se.name, &doesExist);
	CuAssertIntEquals_Msg(tc, "Unlocked file shouldn't have been created when checksum is wrong.", 0, doesExist);
}

void TestPutTransferRemoteDirDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *SURL = pathToStoreFileNoDir;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has previously been created
	se.digs_rm(error_description_, se.name, SURL);
	// and remote dir
	se.digs_rmdir(error_description_, se.name, pathToNoDir);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		if (se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */

	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);

	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been loaded correctly onto the remote machine. */

	char *expectedChecksum = file1Checksum;
	
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(
			error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	
	/* Tidy up*/
	se.digs_rm(error_description_, se.name, SURL);
	se.digs_rmdir(error_description_, se.name, pathToNoDir);
}
void TestPutTransferTwoDiffTransfersOneCancelled(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	digs_error_code_t resultOther = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *localPathOther = largeFileForUpload;
	char *SURL = pathToStoreFile;
	char *SURLOther = pathToStoreOtherFile;
	int handle = -1;
	int handleOther = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has previously been created
	result = se.digs_rm(error_description_, se.name, SURL);
	result = se.digs_rm(error_description_, se.name, SURLOther);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);
	

	/* start other transfer*/
	puts("start other transfer");

	resultOther = se.digs_startPutTransfer(error_description_, se.name, 
			localPathOther, SURLOther, &handleOther);
	
	checkResult(tc, DIGS_SUCCESS, resultOther);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handleOther != -1);

	/* Cancel other transfer. */
	resultOther = se.digs_cancelTransfer(
			error_description_, handleOther);
	checkResult(tc, DIGS_SUCCESS, resultOther);
	
	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(
			error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	/* file shouldn't have finished loading onto the remote machine. */
	
	/*MD5 checksum of largeFile in testData*/
	char *largeFChecksum = largeFileChecksum;
	char **actualChecksumOther = &largeFChecksum;

	resultOther = se.digs_getChecksum(
			error_description_, SURLOther, se.name,
			actualChecksumOther, DIGS_MD5_CHECKSUM);

	/* Unspecified server error indicates that the file does not exist remotely.
	 * If the file has been created, use the checksum to check that the whole 
	 * file has not been transferred. */
	if (result != DIGS_UNSPECIFIED_SERVER_ERROR) {
		checkResult(tc, DIGS_SUCCESS, result);

		/* checksum shouldn't match as file transfer cancelled before complete */
		CuAssertTrue(tc, ((strcmp(*actualChecksum,largeFileChecksum)) != 0));
	}
}

void TestPutTransferTwoDiffTransfers(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	digs_error_code_t resultOther = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *localPathOther = largeFileForUpload;
	char *SURL = pathToStoreFile;
	char *SURLOther = pathToStoreOtherFile;
	int handle = -1;
	int handleOther = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;
	digs_transfer_status_t statusOther = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has been created
	result = se.digs_rm(error_description_, se.name, SURL);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);

	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* start other transfer*/
	puts("start other transfer");

	resultOther = se.digs_startPutTransfer(error_description_, se.name, 
			localPathOther, SURLOther, &handleOther);
	
	checkResult(tc, DIGS_SUCCESS, resultOther);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handleOther != -1);

	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	/* wait for other transfer to complete */
	startTime = time(NULL);
	endTime = time(NULL);
	int progressOther = -1;

	do {
		if (se.digs_monitorTransfer(
				error_description_, handleOther, &statusOther, &progressOther)
				!=DIGS_SUCCESS) {
			break; //error
	}
		CuAssertTrue(tc,((0 <= progressOther) && (progressOther <= 100)));
		endTime = time(NULL);
	} while ((statusOther == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progressOther);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, statusOther);

	resultOther = se.digs_endTransfer(error_description_, handleOther);
	checkResult(tc, DIGS_SUCCESS, resultOther);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksumOther = largeFileChecksum;
	char **actualChecksumOther = &noChecksum;

	resultOther = se.digs_getChecksum(
			error_description_, SURLOther, se.name,
			actualChecksumOther, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, resultOther);
	CuAssertStrEquals(tc, expectedChecksumOther, *actualChecksumOther );
}

void TestPutTransferNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char SURL[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(SURL, no_write_access_path_);
	strcat(SURL, "/uploadedFile"); 
	char *localPath = fileForUpload;
	//char *SURL = pathToStoreFile;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;
	
	// remove remote file if it has been created
	result = se.digs_rm(error_description_, se.name, SURL);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	checkResult(tc, DIGS_SUCCESS, result);

	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		result = se.digs_monitorTransfer(error_description_, handle, &status,
				&progress);
		if (result !=DIGS_SUCCESS) {
			break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should not be complete.",0,progress);
	CuAssertIntEquals_Msg(tc,"Status have failed. ",DIGS_TRANSFER_FAILED, status);
	
	/* cleanup test - monitorTransfer should have failed, so cancelTransfer 
	 * instead of endTransfer.*/
	result = se.digs_cancelTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestPutTransferNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *SURL = pathToStoreFile;
	int handle = -1;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestPutTransferNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *SURL = pathToStoreFile;
	int handle = -1;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			SURL, &handle);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMkDirSuccessful(CuTest *tc) {
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 1; 
	int actual = 0;
	char *filepath  = path_to_made_dir;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	se.digs_rmdir(error_description_, se.name, filepath);
	
	result = se.digs_mkdir(error_description_, se.name, filepath);

	checkResult(tc, DIGS_SUCCESS, result);
	
	result = se.digs_doesExist(
			error_description_, filepath,
			se.name, &actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
	se.digs_rmdir(error_description_, se.name, filepath);
}

void TestMkDirNoRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_dir_);
	strcat(filepath, "/nonExistantDir/newDir"); 
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdir(error_description_, se.name, filepath);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMkDirNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, no_write_access_path_);
	strcat(filepath, "/newDir"); 
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	result = se.digs_mkdir(error_description_, se.name, filepath);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMkDirNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_made_dir);
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdir(error_description_, se.name, filepath);

	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMkDirNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_made_dir);
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdir(error_description_, se.name, filepath);

	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMkDirDirAlreadyExists(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_dir_);
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdir(error_description_, se.name, filepath);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMkDirTreeJustOneNewDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 1; 
	int actual = 0;
	char *filepath  = path_to_made_dir;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	se.digs_rmdir(error_description_, se.name, filepath);

	result = se.digs_mkdirtree(error_description_, se.name, filepath);

	checkResult(tc, DIGS_SUCCESS, result);
	
	result = se.digs_doesExist(
			error_description_, filepath,
			se.name, &actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
}

void TestMkDirTreeSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	int expected = 1; 
	int actual = 0;
	char *filepath = dir_tree;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	se.digs_rmr(error_description_, se.name, middle_of_dir_tree);
	result = se.digs_mkdirtree(error_description_, se.name, filepath);

	checkResult(tc, DIGS_SUCCESS, result);
	
	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, expected, actual );
	se.digs_rmdir(error_description_, se.name, middle_of_dir_tree);
}

void TestMkDirTreeNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, no_write_access_path_);
	strcat(filepath, "/newDir"); 
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdirtree(error_description_, se.name, filepath);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMkDirTreeNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_made_dir);
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdirtree(error_description_, se.name, filepath);

	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMkDirTreeNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_made_dir);
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	result = se.digs_mkdirtree(error_description_, se.name, filepath);

	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMkDirTreeDirAlreadyExists(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char filepath[MAX_ERROR_MESSAGE_LENGTH];
	strcpy(filepath, path_to_dir_);
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdirtree(error_description_, se.name, filepath);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestGetTransferSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *SURL = good_filepath_;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove local file if it has been created
	remove(localPath);

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that file has been downloaded correctly. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	getChecksum(localPath, actualChecksum);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	free(*actualChecksum);
}

void TestGetTransferCancelled(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *SURL = large_good_filepath_;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	// remove local if it has been created
	remove(localPath);

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);

	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait a little for transfer to start. */
	time_t startTime;
	time_t endTime;
	float timeOut = 30;
	startTime = time(NULL);
	endTime = time(NULL);

	do {//just killing time here
		endTime = time(NULL);
	} while (difftime(endTime, startTime) < timeOut);
	
	/* cancel the transfer */
	puts("before cancel");
	result = se.digs_cancelTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);

	/* file should not exist on the local machine. */
	int doesExist = 0;
	/* Check locked file removed. */
	FILE *fp = fopen(pathToDownloadFileTo_,"r");
	if( fp ) {
		doesExist = 1;
	fclose(fp);
	} 
	CuAssertIntEquals_Msg(tc, "Unlocked file shouldn't have been created when cancelled.", 0, doesExist);
	
	/* Check unlocked file has not been created. */
	fp = fopen(pathToDownloadFileToLocked_,"r");
	if( fp ) {
		doesExist = 1;
	fclose(fp);
	}
	CuAssertIntEquals_Msg(tc, "Locked file was not removed.", 0, doesExist);
}


void TestGetTransferRemoteFileDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *SURL = non_existant_filepath_;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove local file if it has been created
	remove(pathToDownloadFileTo_);

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		result = se.digs_monitorTransfer(
				error_description_, handle, &status, &progress);
		if (result != DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* Monitor transfer should  fail as the file doesn't exist. */
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	CuAssertIntEquals_Msg(tc,"Status should have failed",DIGS_TRANSFER_FAILED, status);
	CuAssertTrue(tc, (progress < 100));
	
	/* cleanup test.*/
	result = se.digs_cancelTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestGetTransferTwoDiffTransfersOneCancelled(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	digs_error_code_t resultOther = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *localPathOther = pathToDownloadOtherFileTo_;
	char *localPathOtherLocked = pathToDownloadOtherFileToLocked_;
	char *SURL = good_filepath_;
	char *SURLOther = large_good_filepath_;
	int handle = -1;
	int handleOther = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove local file if it has been created
	remove(pathToDownloadFileTo_);
	remove(pathToDownloadOtherFileTo_);

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);
	
	/* start other transfer*/
	puts("start other transfer");

	resultOther = se.digs_startGetTransfer(
			error_description_, se.name, SURLOther,
			localPathOther, &handleOther);
	
	checkResult(tc, DIGS_SUCCESS, resultOther);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handleOther != -1);

	/* Cancel other transfer. */
	resultOther = se.digs_cancelTransfer(
			error_description_, handleOther);
	checkResult(tc, DIGS_SUCCESS, resultOther);
	
	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	result = se.digs_endTransfer(
			error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been downloaded correctly. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	getChecksum(localPath, actualChecksum);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	free(*actualChecksum);

	/* other file shouldn't have finished loading onto the remote machine. */


	/* file should not exist on the local machine. */
	int doesExist = 0;
	/* Check unlocked file has not been created. */
	FILE *fp = fopen(localPathOther,"r");
	if( fp ) {
		doesExist = 1;
	fclose(fp);
	} 
	CuAssertIntEquals_Msg(tc, "Unlocked file shouldn't have been created when cancelled.", 0, doesExist);

	/* Check locked file removed. */
	fp = fopen(localPathOtherLocked,"r");
	if( fp ) {
		doesExist = 1;
	fclose(fp);
	}
	CuAssertIntEquals_Msg(tc, "Locked file was not removed.", 0, doesExist);
}

void TestGetTransferTwoDiffTransfers(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	digs_error_code_t resultOther = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *localPathOther = pathToDownloadOtherFileTo_;
	char *SURL = good_filepath_;
	char *SURLOther = large_good_filepath_;
	int handle = -1;
	int handleOther = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;
	digs_transfer_status_t statusOther = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove local file if it has been created
	remove(localPath);
	remove(localPathOther);

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* start other transfer*/
	puts("start other transfer");

	resultOther = se.digs_startGetTransfer(
			error_description_, se.name, SURLOther,
			localPathOther, &handleOther);
	
	checkResult(tc, DIGS_SUCCESS, resultOther);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handleOther != -1);


	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);

	/* check that file has been downloaded correctly. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	getChecksum(localPath, actualChecksum);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	free(*actualChecksum);

	/* wait for other transfer to complete */
	startTime = time(NULL);
	endTime = time(NULL);
	int progressOther = -1;

	do {
		if (se.digs_monitorTransfer(
				error_description_, handleOther, &statusOther, &progressOther)
				!=DIGS_SUCCESS) {
			break; //error
	}
		CuAssertTrue(tc,((0 <= progressOther) && (progressOther <= 100)));
		endTime = time(NULL);
	} while ((statusOther == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progressOther);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, statusOther);
	
	resultOther = se.digs_endTransfer(
			error_description_, handleOther);
	checkResult(tc, DIGS_SUCCESS, resultOther);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksumOther = largeFileChecksum;
	char **actualChecksumOther = &noChecksum;

	getChecksum(localPathOther, actualChecksumOther);
	CuAssertStrEquals(tc, expectedChecksumOther, *actualChecksumOther );
	free(*actualChecksumOther);
}


void TestGetTransferNoWritePermissionsOnLocalDir(CuTest *tc) {

	digs_error_code_t result = DIGS_SUCCESS;
	char *localPath = no_write_access_pathToDownloadFileTo_;
	char *SURL = good_filepath_;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);

	checkResult(tc, DIGS_UNKNOWN_ERROR, result);

	/* cleanup test.*/
	result = se.digs_endTransfer(
			error_description_, handle);
	checkResult(tc, DIGS_UNKNOWN_ERROR, result);
}

void TestGetTransferNoGlobusOnRemoteNode (CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *SURL = good_filepath_;
	int handle = -1;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);
	if(result != DIGS_SUCCESS){
		checkResult(tc, DIGS_NO_CONNECTION, result);
	}
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		result = se.digs_monitorTransfer(
				error_description_, handle, &status, &progress);
		if (result != DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* Monitor transfer should  fail as the file doesn't exist. */
	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals_Msg(tc,"Status should have failed",DIGS_TRANSFER_FAILED, status);
	CuAssertTrue(tc, (progress < 100));
	
	/* cleanup test.*/
	result = se.digs_cancelTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
}
void TestGetTransferNodeDoesNotExist (CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = pathToDownloadFileTo_;
	char *SURL = good_filepath_;
	int handle = -1;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_startGetTransfer(error_description_, se.name, SURL,
			localPath, &handle);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestCopyToInboxSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *lfn = "myDir/anotherDir/file1.txt";
	char *SURL = path_to_file_in_new_dir;
	int handle = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has been created. 
	result = se.digs_rm(error_description_, se.name, SURL);

	result = se.digs_startCopyToInbox(error_description_, se.name, localPath,
			lfn, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {
		if (se.digs_monitorTransfer(error_description_, handle, &status,
				&progress)!=DIGS_SUCCESS) {
			break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
}

void TestCopyToInboxNoInbox(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *lfn = "myDir/anotherDir/file1.txt";
	int handle = 1;
	char *storage_element = no_inbox_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_startCopyToInbox(error_description_, se.name, localPath,
			lfn, &handle);

	/* check that a handle is -1 as copy will fail */
	CuAssert(tc, "handle not set", handle == -1);
	checkResult(tc, DIGS_NO_INBOX, result);
}

void TestCopyToInboxTwoDiffTransfersOneCancelled(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	digs_error_code_t resultOther = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *localPathOther = largeFileForUpload;
	char *lfn = "myDir/anotherDir/file1.txt";
	char *lfnOther = "myDir/anotherDir/largeFile.txt";
	char *SURL = path_to_file_in_new_dir;
	char *SURLOther = path_to_other_file_in_new_dir;
	int handle = -1;
	int handleOther = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has been created.
	result = se.digs_rm(error_description_, se.name, SURL); 
	result = se.digs_rm(error_description_, se.name, SURLOther); 

	result = se.digs_startCopyToInbox(error_description_, se.name, localPath,
			lfn, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);
	

	/* start other transfer*/
	puts("start other transfer");

	resultOther = se.digs_startCopyToInbox(error_description_, se.name, localPathOther,
			lfnOther, &handleOther);
	
	checkResult(tc, DIGS_SUCCESS, resultOther);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handleOther != -1);

	/* Cancel other transfer. */
	resultOther = se.digs_cancelTransfer(
			error_description_, handleOther);
	checkResult(tc, DIGS_SUCCESS, resultOther);
	
	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(
			error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );

	/*other file shouldn't exist on the remote machine. */
	int doesExist = 0;
	se.digs_doesExist(error_description_, se.name, SURLOther, &doesExist);
	CuAssertIntEquals_Msg(tc, "Cancelling transfer should remove remote file.", 0, doesExist);
}

void TestCopyToInboxTwoDiffTransfers(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	digs_error_code_t resultOther = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *localPathOther = largeFileForUpload;
	char *lfn = "myDir/anotherDir/file1.txt";
	char *lfnOther = "myDir/anotherDir/largeFile.txt";
	char *SURL = path_to_file_in_new_dir;
	char *SURLOther = path_to_other_file_in_new_dir;
	int handle = -1;
	int handleOther = -1;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;
	digs_transfer_status_t statusOther = DIGS_TRANSFER_PREPARATION_COMPLETE;

	// remove remote file if it has been created
	result = se.digs_rm(error_description_, se.name, SURL); 
	result = se.digs_rm(error_description_, se.name, SURLOther); 

	result = se.digs_startCopyToInbox(error_description_, se.name, localPath,
			lfn, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);
	
	
	/* start other transfer*/
	puts("start other transfer");
	
	resultOther = se.digs_startCopyToInbox(error_description_, se.name, localPathOther,
			lfnOther, &handleOther);
	
	checkResult(tc, DIGS_SUCCESS, resultOther);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handleOther != -1);

	/* wait for first transfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	
	do {if( se.digs_monitorTransfer(
				error_description_, handle, &status, &progress)!=DIGS_SUCCESS){
		break; //error
	}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);
	
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(error_description_, SURL, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );


	/* wait for other transfer to complete */
	startTime = time(NULL);
	endTime = time(NULL);
	int progressOther = -1;

	do {
		if (se.digs_monitorTransfer(
				error_description_, handleOther, &statusOther, &progressOther)
				!=DIGS_SUCCESS) {
			break; //error
	}
		CuAssertTrue(tc,((0 <= progressOther) && (progressOther <= 100)));
		endTime = time(NULL);
	} while ((statusOther == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progressOther);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, statusOther);

	resultOther = se.digs_endTransfer(error_description_, handleOther);
	checkResult(tc, DIGS_SUCCESS, resultOther);
	
	/* check that file has been loaded correctly onto the remote machine. */
	char *expectedChecksumOther = largeFileChecksum;
	char **actualChecksumOther = &noChecksum;

	resultOther = se.digs_getChecksum(
			error_description_, SURLOther, se.name,
			actualChecksumOther, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, resultOther);
	CuAssertStrEquals(tc, expectedChecksumOther, *actualChecksumOther );
}

void TestCopyToInboxNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *lfn = "myDir/anotherDir/file1.txt";
	int handle = 1;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_startCopyToInbox(error_description_, se.name, localPath,
			lfn, &handle);

	/* check that a handle is -1 as copy will fail */
	CuAssert(tc, "handle not set", handle == -1);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestCopyToInboxNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *localPath = fileForUpload;
	char *lfn = "myDir/anotherDir/file1.txt";
	int handle = 1;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_startCopyToInbox(error_description_, se.name, localPath,
			lfn, &handle);

	/* check that a handle is -1 as copy will fail */
	CuAssert(tc, "handle not set", handle == -1);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestGetModificationTimeSuccessful(CuTest *tc) {
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	//char *expectedFileTime = "Mon Mar 15 16:44:29 2010\n";
	time_t actualFileTime= time(NULL); // Time now. 
	char *filepath = good_filepath_;
	char *storage_element = good_se_;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getModificationTime(error_description_, filepath, se.name,
			&actualFileTime);

	checkResult(tc, DIGS_SUCCESS, result);
	//Convert time_t to string of local time.
	struct tm *tminfo;
	tminfo = localtime( &actualFileTime);
	char *actualFileTimeStr = asctime(tminfo);
	CuAssertStrEquals(tc, expectedFileTime, actualFileTimeStr);
}

void TestGetModificationTimeNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	time_t actualFileTime= time(NULL); // Time now. 
	char *filepath = non_existant_filepath_;
	char *storage_element = good_se_;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getModificationTime(error_description_, filepath, se.name,
			&actualFileTime);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestGetModificationTimeNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	time_t actualFileTime= time(NULL); // Time now. 
	char *filepath = no_read_access_path_;
	char *storage_element = good_se_;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getModificationTime(error_description_, filepath, se.name,
			&actualFileTime);

	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}
void TestGetModificationTimeFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	//char *expectedFileTime = "Mon Mar 15 16:48:23 2010\n";
	time_t actualFileTime= time(NULL); // Time now. 
	char *filepath = time_dir_;
	char *storage_element = good_se_;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getModificationTime(error_description_, filepath, se.name,
			&actualFileTime);

	checkResult(tc, DIGS_SUCCESS, result);
	//Convert time_t to string of local time.
	struct tm *tminfo;
	tminfo = localtime( &actualFileTime);
	char *actualFileTimeStr = asctime(tminfo);
	CuAssertStrEquals(tc, expectedDirTime, actualFileTimeStr);
}

void TestGetModificationTimeNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	time_t actualFileTime= time(NULL); // Time now. 
	char *filepath = good_filepath_;
	char *storage_element = no_globus_se_;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getModificationTime(error_description_, filepath, se.name,
			&actualFileTime);

	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestGetModificationTimeNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	time_t actualFileTime= time(NULL); // Time now. 
	char *filepath = good_filepath_;
	char *storage_element = not_a_node_se_;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_getModificationTime(error_description_, filepath, se.name,
			&actualFileTime);

	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMoveSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = good_filepath_for_mv;
	char *newLocation = pathToStoreFile;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	//TODO remove remote file if it has been created
	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been transferred correctly. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(error_description_, newLocation, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	free(*actualChecksum);
	
	/* put the file back.*/
	result = se.digs_mv(error_description_, se.name, newLocation, oldLocation);
	
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestMoveNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = non_existant_filepath_;
	char *newLocation = pathToStoreFile;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMoveNoReadPermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = no_read_access_path_;
	char *newLocation = pathToStoreFile;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMoveNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = good_filepath_;
	char *newLocation = no_write_access_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestMoveFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = path_to_dir_;
	char *newLocation = pathToStoreFile;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_FILE_IS_DIR, result);
}

void TestMoveNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = good_filepath_;
	char *newLocation = pathToStoreFile;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMoveNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = good_filepath_;
	char *newLocation = pathToStoreFile;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestMoveNoDirToPutFileTo(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *oldLocation = good_filepath_;
	char *newLocation = pathToStoreFileNoDir;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	se.digs_rmr(error_description_, se.name, pathToNoDir);
	result = se.digs_mv(error_description_, se.name, oldLocation, newLocation);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}
/*Relies on TestPutTransferSuccessful having been run previously (to put in 
 * place the file to be removed. */
void TestRemoveSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = pathToStoreFile;
	char *storage_element = good_se_;
	int actual = 0;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rm(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been removed. */
	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
	
}

void TestRemoveNoRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  non_existant_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rm(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRemoveNoWritePermissionsOnRemoteFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  no_write_access_file_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rm(error_description_, se.name, filepath);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRemoveFileIsDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  path_to_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rm(error_description_, se.name, filepath);
	checkResult(tc, DIGS_FILE_IS_DIR, result);
}

void TestRemoveNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_rm(error_description_, se.name, filepath);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}
void TestRemoveNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rm(error_description_, se.name, filepath);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestRemoveDirSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = path_to_made_dir;
	char *storage_element = good_se_;
	int actual = 0;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdir(error_description_, se.name, filepath);
	
	result = se.digs_rmdir(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been removed. */
	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRemoveDirNoRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = path_to_made_dir;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_rmdir(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}
void TestRemoveDirNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = no_write_access_empty_dir_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_rmdir(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRemoveDirDirIsFile(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = good_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_rmdir(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRemoveDirNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  path_to_made_dir;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_rmdir(error_description_, se.name, filepath);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}
void TestRemoveDirNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath =  path_to_made_dir;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rmdir(error_description_, se.name, filepath);
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestRemoveDirDirNotEmpty(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *fullPathToDir = dir_tree;
	char *pathToNonEmptyDir =  middle_of_dir_tree;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);
	se.digs_mkdirtree(error_description_, se.name, fullPathToDir);
	result = se.digs_rmdir(error_description_, se.name, pathToNonEmptyDir);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRmrJustOneDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = path_to_made_dir;
	char *storage_element = good_se_;
	int actual = 0;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_mkdir(error_description_, se.name, filepath);
	result = se.digs_rmr(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been removed. */
	result = se.digs_doesExist(error_description_, filepath, se.name, &actual);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRmrNoWritePermissionsOnRemoteDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = no_write_access_filepath_;
	char *storage_element = good_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rmr(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRmrNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = good_filepath_;
	char *storage_element = no_globus_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rmr(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestRmrNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *filepath = good_filepath_;
	char *storage_element = not_a_node_se_;

	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_rmr(error_description_, se.name, filepath);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestRmrSeveralDirsWithFiles(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *fullPathToDir = dir_tree;
	char *pathToNonEmptyDir =  middle_of_dir_tree;
	char *pathToFileInNonEmptyDir = file_in_dir_tree;
	char *storage_element = good_se_;
	int actual = 0;
	char *localPath = fileForUpload;
	int handle = -1;

	struct storageElement se;
	se = *getNode(storage_element);

    se.digs_mkdirtree(error_description_, se.name, fullPathToDir);
    
    /*Start adding file to dir*/
	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			pathToFileInNonEmptyDir, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;
	
	do {
		if (se.digs_monitorTransfer(error_description_, handle, &status,
				&progress)!=DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	/* cleanup test.*/
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
    /*End of adding file to dir */
    
	result = se.digs_rmr(error_description_, se.name, pathToNonEmptyDir);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been removed. */
	result = se.digs_doesExist(error_description_, pathToNonEmptyDir, se.name,
			&actual);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRmrSeveralDirs(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *fullPathToDir = dir_tree;
	char *pathToNonEmptyDir =  middle_of_dir_tree;
	char *storage_element = good_se_;
	int actual = 0;

	struct storageElement se;
	se = *getNode(storage_element);

    se.digs_mkdirtree(error_description_, se.name, fullPathToDir);
	result = se.digs_rmr(error_description_, se.name, pathToNonEmptyDir);
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been removed. */
	result = se.digs_doesExist(error_description_, pathToNonEmptyDir, se.name,
			&actual);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestRmrFileInsteadOfDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *pathToFile =  non_existant_filepath_;
	char *storage_element = good_se_;
	int actual = 0;
	char *localPath = fileForUpload;
	int handle = -1;

	struct storageElement se;
	se = *getNode(storage_element);

    /*Start adding file to dir*/

	result = se.digs_startPutTransfer(error_description_, se.name, localPath,
			pathToFile, &handle);
	
	checkResult(tc, DIGS_SUCCESS, result);
	/* check that a handle is returned */
	CuAssert(tc, "handle not set", handle != -1);

	/* wait for update tranfer to complete */
	time_t startTime;
	time_t endTime;
	float timeOut = 500;
	startTime = time(NULL);
	endTime = time(NULL);
	int progress = -1;
	digs_transfer_status_t status = DIGS_TRANSFER_PREPARATION_COMPLETE;
	
	do {
		if (se.digs_monitorTransfer(error_description_, handle, &status,
				&progress)!=DIGS_SUCCESS) {
			break; //error
		}
		CuAssertTrue(tc,((0 <= progress) && (progress <= 100)));
		endTime = time(NULL);
	} while ((status == DIGS_TRANSFER_IN_PROGRESS) && (difftime(endTime,
			startTime) < timeOut));
	
	/* progress should be 100% complete */
	CuAssertIntEquals_Msg(tc,"Progress should be 100% complete.",100,progress);
	CuAssertIntEquals_Msg(tc,"Status should be done",DIGS_TRANSFER_DONE, status);

	/* cleanup test.*/
	result = se.digs_endTransfer(error_description_, handle);
	checkResult(tc, DIGS_SUCCESS, result);
    /*End of adding file to dir */
	result = se.digs_rmr(error_description_, se.name, pathToFile);
	checkResult(tc, DIGS_SUCCESS, result);

	/* check that file has been removed. */
	result = se.digs_doesExist(error_description_, pathToFile, se.name,
			&actual);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestCopyFromInboxSuccessful(CuTest *tc) {

	/* Run TestCopyToInbox to load up the file. */
	TestCopyToInboxSuccessful(tc);
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *targetPath = pathToStoreFile;
	char *storage_element = good_se_;
	char *lfn = file1lfn;
	
	struct storageElement se;
	se = *getNode(storage_element);

	// remove remote file if it has previously been created
	result = se.digs_rm(error_description_, se.name, targetPath);
	
	result = se.digs_copyFromInbox(error_description_, se.name, lfn, targetPath);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	/* check that file has been transferred correctly. */
	char *expectedChecksum = file1Checksum;
	char *noChecksum = "no checksum set";
	char **actualChecksum = &noChecksum;

	result = se.digs_getChecksum(error_description_, targetPath, se.name,
			actualChecksum, DIGS_MD5_CHECKSUM);

	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertStrEquals(tc, expectedChecksum, *actualChecksum );
	free(*actualChecksum);
	
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestCopyFromInboxNoInbox(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *targetPath = pathToStoreFile;
	char *storage_element = no_inbox_se_;
	char *lfn = file1lfn;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_copyFromInbox(error_description_, se.name, lfn, targetPath);
	
	checkResult(tc, DIGS_NO_INBOX, result);
}

void TestCopyFromInboxNoWritePermissions(CuTest *tc) {

	/* Run TestCopyToInbox to load up the file. */
	TestCopyToInboxSuccessful(tc);
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *targetPath = no_write_access_filepath_;
	char *storage_element = good_se_;
	char *lfn = file1lfn;
	
	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_copyFromInbox(error_description_, se.name, lfn, targetPath);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestCopyFromInboxNoFileInInbox(CuTest *tc) {

	/* Run TestCopyToInbox to load up the file. */
	TestCopyToInboxSuccessful(tc);
	
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *targetPath = no_write_access_filepath_;
	char *storage_element = good_se_;
	char *lfn = non_existant_filepath_;
	
	struct storageElement se;
	se = *getNode(storage_element);
	
	result = se.digs_copyFromInbox(error_description_, se.name, lfn, targetPath);
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

void TestCopyFromInboxNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *targetPath = pathToStoreFile;
	char *storage_element = no_globus_se_;
	char *lfn = file1lfn;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_copyFromInbox(error_description_, se.name, lfn, targetPath);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestCopyFromInboxNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *targetPath = pathToStoreFile;
	char *storage_element = not_a_node_se_;
	char *lfn = file1lfn;
	
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_copyFromInbox(error_description_, se.name, lfn, targetPath);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}
void testArraysMatch(CuTest *tc, char **oneArray, int lengthOfOneArray,
		char **anotherArray, int lengthOfAnotherArray) {
	CuAssertIntEquals_Msg(tc,"arrays should be the same length.",
			lengthOfOneArray,lengthOfAnotherArray);
	for(int i=0;i<lengthOfOneArray;i++){
		int match = 0;
		for (int j=0; j<lengthOfAnotherArray; j++){
			if (!strcmp(oneArray[i],anotherArray[j])){
				match =1;
			}
		}
		CuAssertIntEquals_Msg(tc,"Failed to match array.",1,match);
	}
}

void TestScanNodeSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = good_se_;
	
	char **expectedList;
	int lengthOfExpectedList = 8;
	expectedList = malloc(lengthOfExpectedList * sizeof(*expectedList));
	expectedList[0]=makestring(valid_se_path, "/data/fruit/apple/grannySmith");
	expectedList[1]=makestring(valid_se_path, "/data/fruit/apple/cox");
	expectedList[2]=makestring(valid_se_path, "/data/fruit/apple/cooking/bramley");
	expectedList[3]=makestring(valid_se_path, "/data/fruit/forbidden-LOCKED");
	expectedList[4]=makestring(valid_se_path, "/data/fruit/orange");
	expectedList[5]=makestring(valid_se_path, "/data1/veg/kale");
	expectedList[6]=makestring(valid_se_path, "/data1/veg/root/carrot");
	expectedList[7]=makestring(valid_se_path, "/data1/veg/root/potato");
	
	
	char ***list;
	list = globus_malloc(sizeof(char***));

	struct storageElement se;
	se = *getNode(storage_element);
	int lengthOfList = 0;
	int allFiles = 1;

	result = se.digs_scanNode(error_description_, se.name, list, &lengthOfList,
			allFiles);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	logMessage(DEBUG, "got this far at least");
	for (int i=0; i<lengthOfList; i++) {
		logMessage(DEBUG, "File in list test is (%s)", (*list)[i]);
	}
	
	testArraysMatch(tc, expectedList, lengthOfExpectedList, *list, lengthOfList);
	se.digs_free_string_array(list, &lengthOfList);
}
void TestScanNodeSuccessfulDoNotShowAll(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = good_se_;
	
	char **expectedList;
	int lengthOfExpectedList = 7;
	expectedList = malloc(lengthOfExpectedList * sizeof(*expectedList));
	expectedList[0]=makestring(valid_se_path, "/data/fruit/apple/grannySmith");
	expectedList[1]=makestring(valid_se_path, "/data/fruit/apple/cox");
	expectedList[2]=makestring(valid_se_path, "/data/fruit/apple/cooking/bramley");
	expectedList[3]=makestring(valid_se_path, "/data/fruit/orange");
	expectedList[4]=makestring(valid_se_path, "/data1/veg/kale");
	expectedList[5]=makestring(valid_se_path, "/data1/veg/root/carrot");
	expectedList[6]=makestring(valid_se_path, "/data1/veg/root/potato");
	
	char ***list;
	list = globus_malloc(sizeof(char***));

	struct storageElement se;
	se = *getNode(storage_element);
	int lengthOfList = 0;
	int allFiles = 0;

	result = se.digs_scanNode(error_description_, se.name, list, &lengthOfList,
			allFiles);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	logMessage(DEBUG, "got this far at least");
	for (int i=0; i<lengthOfList; i++) {
		logMessage(DEBUG, "File in list test is (%s)", (*list)[i]);
	}
	
	testArraysMatch(tc, expectedList, lengthOfExpectedList, *list, lengthOfList);
	se.digs_free_string_array(list, &lengthOfList);
}
void TestScanNodeNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = no_globus_se_;
	int lengthOfExpectedList = 0;
	
	char ***list;
	list = globus_malloc(sizeof(char***));

	struct storageElement se;
	se = *getNode(storage_element);
	int lengthOfList = 0;
	int allFiles = 1;

	result = se.digs_scanNode(error_description_, se.name, list, &lengthOfList,
			allFiles);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, lengthOfExpectedList, lengthOfList);

	se.digs_free_string_array(list, &lengthOfList);
}
void TestScanNodeNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = not_a_node_se_;
	int lengthOfExpectedList = 0;
	
	char ***list;
	list = globus_malloc(sizeof(char***));

	struct storageElement se;
	se = *getNode(storage_element);
	int lengthOfList = 0;
	int allFiles = 1;

	result = se.digs_scanNode(error_description_, se.name, list, &lengthOfList,
			allFiles);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
	CuAssertIntEquals(tc, lengthOfExpectedList, lengthOfList);

	se.digs_free_string_array(list, &lengthOfList);
}

void TestScanNodeNoDataDir(CuTest *tc) {
	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = no_inbox_se_;
	int lengthOfExpectedList = 0;
	
	char ***list;
	list = globus_malloc(sizeof(char***));
	char **temp;
	list = &temp;

	struct storageElement se;
	se = *getNode(storage_element);
	int lengthOfList = 0;
	int allFiles = 1;

	result = se.digs_scanNode(error_description_, se.name, list, &lengthOfList,
			allFiles);

	// TODO Should this give an error message when there is no data dir?
	checkResult(tc, DIGS_SUCCESS, result);
	CuAssertIntEquals(tc, lengthOfExpectedList, lengthOfList);

	se.digs_free_string_array(list, &lengthOfList);
}

void TestFreeStringArraySuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = good_se_;
	
	char ***list;
	list = malloc(sizeof(char***));
	char **temp;
	list = &temp;

	struct storageElement se;
	se = *getNode(storage_element);
	int lengthOfList = 0;
	int allFiles = 1;

	result = se.digs_scanNode(error_description_, se.name, list, &lengthOfList,
			allFiles);
	
	checkResult(tc, DIGS_SUCCESS, result);
	
	se.digs_free_string_array(list, &lengthOfList);

	CuAssertIntEquals_Msg(tc, "Expected length of List to be zero.",0, lengthOfList);
	CuAssertTrue(tc, *list == NULL);
	//TODO Should I free list here? seems to crash if I do.
}

void TestPingSuccessful(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = good_se_;
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_ping(error_description_, se.name);
	
	checkResult(tc, DIGS_SUCCESS, result);
}

void TestPingNoGlobusOnRemoteNode(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = no_globus_se_;
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_ping(error_description_, se.name);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestPingNodeDoesNotExist(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = not_a_node_se_;
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_ping(error_description_, se.name);
	
	checkResult(tc, DIGS_NO_CONNECTION, result);
}

void TestPingNoDataDir(CuTest *tc) {

	digs_error_code_t result = DIGS_UNKNOWN_ERROR;
	char *storage_element = no_inbox_se_;
	struct storageElement se;
	se = *getNode(storage_element);

	result = se.digs_ping(error_description_, se.name);
	
	checkResult(tc, DIGS_UNSPECIFIED_SERVER_ERROR, result);
}

CuSuite* StrUtilGetSuite() {
	CuSuite* suite;
	setup();
	suite = CuSuiteNew();
/* 	/\*getLength*\/ */
  	SUITE_ADD_TEST(suite, TestGetLengthSuccessful);
  	SUITE_ADD_TEST(suite, TestGetLengthNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestGetLengthNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestGetLengthFileIsDir);
  	SUITE_ADD_TEST(suite, TestGetLengthNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestGetLengthNodeDoesNotExist);
/* 	/\*getChecksum*\/ */
  	SUITE_ADD_TEST(suite, TestGetChecksumSuccessful);
  	SUITE_ADD_TEST(suite, TestGetChecksumNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestGetChecksumNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestGetChecksumFileIsDir);
  	SUITE_ADD_TEST(suite, TestGetChecksumNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestGetChecksumNodeDoesNotExist);
  	SUITE_ADD_TEST(suite, TestGetChecksumUnsupportedType);
/* 	/\*freeString*\/ */
 	SUITE_ADD_TEST(suite, TestFreeString);
/* 	/\*doesExist*\/ */
  	SUITE_ADD_TEST(suite, TestDoesExistSuccessful);
  	SUITE_ADD_TEST(suite, TestDoesNotExistSuccessful);
  	SUITE_ADD_TEST(suite, TestDoesExistNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestDoesExistFileIsDir);
  	SUITE_ADD_TEST(suite, TestDoesExistNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestDoesExistNodeDoesNotExist);
/* 	/\*isDirectory*\/ */
  	SUITE_ADD_TEST(suite, TestIsDirectorySuccessful);
  	SUITE_ADD_TEST(suite, TestIsNotDirectorySuccessful);
  	SUITE_ADD_TEST(suite, TestIsDirectoryNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestIsDirectoryNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestIsDirectoryNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestIsDirectoryNodeDoesNotExist);
/* 	/\*getOwner*\/ */
  	SUITE_ADD_TEST(suite, TestGetOwnerSuccessful);
  	SUITE_ADD_TEST(suite, TestGetOwnerNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestGetOwnerNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestGetOwnerFileIsDir);
  	SUITE_ADD_TEST(suite, TestGetOwnerNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestGetOwnerNodeDoesNotExist);
/* 	/\*getGroup*\/ */
   	SUITE_ADD_TEST(suite, TestGetGroupSuccessful);
   	SUITE_ADD_TEST(suite, TestGetGroupNoRemoteFile);
   	SUITE_ADD_TEST(suite, TestGetGroupNoReadPermissionsOnRemoteDir);
   	SUITE_ADD_TEST(suite, TestGetGroupFileIsDir);
   	SUITE_ADD_TEST(suite, TestGetGroupNoGlobusOnRemoteNode);
   	SUITE_ADD_TEST(suite, TestGetGroupNodeDoesNotExist);
/* 	/\*setGroup*\/ */
  	SUITE_ADD_TEST(suite, TestSetGroupSuccessful);
  	SUITE_ADD_TEST(suite, TestSetGroupNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestSetGroupNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestSetGroupNoPermissionsToAddToGroup);
   	SUITE_ADD_TEST(suite, TestSetGroupGroupDoesNotExist);
   	SUITE_ADD_TEST(suite, TestSetGroupFileIsDir);
 	SUITE_ADD_TEST(suite, TestSetGroupNoGlobusOnRemoteNode);
    	SUITE_ADD_TEST(suite, TestSetGroupNodeDoesNotExist); 
/* 	/\*getPermissions*\/ */
  	SUITE_ADD_TEST(suite, TestGetPermissionsSuccessful);
  	SUITE_ADD_TEST(suite, TestGetPermissionsNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestGetPermissionsNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestGetPermissionsFileIsDir);
  	SUITE_ADD_TEST(suite, TestGetPermissionsNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestGetPermissionsNodeDoesNotExist);
/* 	/\*setPermissions(chmod)*\/ */
  	SUITE_ADD_TEST(suite, TestSetPermissionsSuccessful);
  	SUITE_ADD_TEST(suite, TestSetPermissionsNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestSetPermissionsNoWritePermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestSetPermissionsOnDir);
  	SUITE_ADD_TEST(suite, TestSetPermissionsNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestSetPermissionsNodeDoesNotExist);
	
/* 	/\*startPutTransfer (These tests may be slow )*\/ */
  	SUITE_ADD_TEST(suite, TestPutTransferCancelled);
  	SUITE_ADD_TEST(suite, TestPutTransferSuccessful);
 	SUITE_ADD_TEST(suite, TestEndTransferChecksChecksum);
  	SUITE_ADD_TEST(suite, TestPutTransferRemoteDirDoesNotExist);
/* 	 /\* test with 2 diff transfers, one good, one bad, monitor works for both.  */
/* 	 *  - testing transfer id. *\/ */
  	SUITE_ADD_TEST(suite, TestPutTransferTwoDiffTransfersOneCancelled);
  	SUITE_ADD_TEST(suite, TestPutTransferTwoDiffTransfers);
  	SUITE_ADD_TEST(suite, TestPutTransferNoWritePermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestPutTransferNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestPutTransferNodeDoesNotExist);
/* 	/\*mkdir*\/ */
  	SUITE_ADD_TEST(suite, TestMkDirNoRemoteDir);
  	SUITE_ADD_TEST(suite, TestMkDirNoWritePermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestMkDirNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestMkDirNodeDoesNotExist);
  	SUITE_ADD_TEST(suite, TestMkDirDirAlreadyExists);
  	SUITE_ADD_TEST(suite, TestMkDirSuccessful);
/* 	/\*mkdirtree*\/ */
  	SUITE_ADD_TEST(suite, TestMkDirTreeNoWritePermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestMkDirTreeNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestMkDirTreeNodeDoesNotExist);
  	SUITE_ADD_TEST(suite, TestMkDirTreeDirAlreadyExists);
  	SUITE_ADD_TEST(suite, TestMkDirTreeJustOneNewDir);
  	SUITE_ADD_TEST(suite, TestMkDirTreeSuccessful);
/* 	/\*startGetTransfer (These tests may be slow )*\/ */
  	SUITE_ADD_TEST(suite, TestGetTransferCancelled);
  	SUITE_ADD_TEST(suite, TestGetTransferSuccessful);
 	SUITE_ADD_TEST(suite, TestGetTransferRemoteFileDoesNotExist);
/* 	/\* test with 2 diff transfers, one good, one bad, monitor works for both.  */
/* 	  - testing transfer id. *\/ */
   	SUITE_ADD_TEST(suite, TestGetTransferTwoDiffTransfersOneCancelled);
   	SUITE_ADD_TEST(suite, TestGetTransferTwoDiffTransfers);
   	SUITE_ADD_TEST(suite, TestGetTransferNoWritePermissionsOnLocalDir);
  	SUITE_ADD_TEST(suite, TestGetTransferNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestGetTransferNodeDoesNotExist);
/* 	/\*copyToNEW*\/ */
  	SUITE_ADD_TEST(suite, TestCopyToInboxSuccessful);
  	SUITE_ADD_TEST(suite, TestCopyToInboxNoInbox);
/* 	/\* test with 2 diff transfers *\/ */
  	SUITE_ADD_TEST(suite, TestCopyToInboxTwoDiffTransfersOneCancelled);
  	SUITE_ADD_TEST(suite, TestCopyToInboxTwoDiffTransfers);
  	SUITE_ADD_TEST(suite, TestCopyToInboxNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestCopyToInboxNodeDoesNotExist);
/* 	/\*getModificationTime*\/ */
  	SUITE_ADD_TEST(suite, TestGetModificationTimeSuccessful);
  	SUITE_ADD_TEST(suite, TestGetModificationTimeNoRemoteFile);
  	SUITE_ADD_TEST(suite, TestGetModificationTimeNoReadPermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestGetModificationTimeFileIsDir);
  	SUITE_ADD_TEST(suite, TestGetModificationTimeNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestGetModificationTimeNodeDoesNotExist);
/* 	/\*mv*\/ */
  	SUITE_ADD_TEST(suite, TestMoveSuccessful);
 	SUITE_ADD_TEST(suite, TestMoveNoRemoteFile);
 	SUITE_ADD_TEST(suite, TestMoveNoReadPermissionsOnRemoteDir);
 	SUITE_ADD_TEST(suite, TestMoveNoWritePermissionsOnRemoteDir);
 	SUITE_ADD_TEST(suite, TestMoveFileIsDir);
 	SUITE_ADD_TEST(suite, TestMoveNoGlobusOnRemoteNode);
 	SUITE_ADD_TEST(suite, TestMoveNodeDoesNotExist);
 	SUITE_ADD_TEST(suite, TestMoveNoDirToPutFileTo);
/* 	/\*rm*\/ */
 	SUITE_ADD_TEST(suite, TestPutTransferSuccessful);
 	SUITE_ADD_TEST(suite, TestRemoveSuccessful); // Must run TestPutTransferSuccessful first.
 	SUITE_ADD_TEST(suite, TestRemoveNoRemoteFile);
 	SUITE_ADD_TEST(suite, TestRemoveNoWritePermissionsOnRemoteFile);
 	SUITE_ADD_TEST(suite, TestRemoveFileIsDir);
	SUITE_ADD_TEST(suite, TestRemoveNoGlobusOnRemoteNode);
 	SUITE_ADD_TEST(suite, TestRemoveNodeDoesNotExist);
/* 	/\*rmdir*\/ */
  	SUITE_ADD_TEST(suite, TestRemoveDirSuccessful);
  	SUITE_ADD_TEST(suite, TestRemoveDirNoRemoteDir);
  	SUITE_ADD_TEST(suite, TestRemoveDirNoWritePermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestRemoveDirDirIsFile);
  	SUITE_ADD_TEST(suite, TestRemoveDirNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestRemoveDirNodeDoesNotExist);
  	SUITE_ADD_TEST(suite, TestRemoveDirDirNotEmpty);
/* 	/\*rmr*\/ */
   	SUITE_ADD_TEST(suite, TestRmrNoWritePermissionsOnRemoteDir);
  	SUITE_ADD_TEST(suite, TestRmrNoGlobusOnRemoteNode);
  	SUITE_ADD_TEST(suite, TestRmrNodeDoesNotExist);
 	SUITE_ADD_TEST(suite, TestRmrJustOneDir);
  	SUITE_ADD_TEST(suite, TestRmrSeveralDirs);
 	SUITE_ADD_TEST(suite, TestRmrSeveralDirsWithFiles);
	SUITE_ADD_TEST(suite, TestRmrFileInsteadOfDir);
/* 	/\*copyFromInbox*\/ */
 	SUITE_ADD_TEST(suite, TestCopyFromInboxSuccessful);
 	SUITE_ADD_TEST(suite, TestCopyFromInboxNoInbox);
 	SUITE_ADD_TEST(suite, TestCopyFromInboxNoWritePermissions);
 	SUITE_ADD_TEST(suite, TestCopyFromInboxNoFileInInbox);
 	SUITE_ADD_TEST(suite, TestCopyFromInboxNoGlobusOnRemoteNode);
 	SUITE_ADD_TEST(suite, TestCopyFromInboxNodeDoesNotExist);
	/* scanNode */
 	SUITE_ADD_TEST(suite, TestScanNodeSuccessful);
 	SUITE_ADD_TEST(suite, TestScanNodeSuccessfulDoNotShowAll);
 	SUITE_ADD_TEST(suite, TestScanNodeNoGlobusOnRemoteNode);
 	SUITE_ADD_TEST(suite, TestScanNodeNodeDoesNotExist);
 	SUITE_ADD_TEST(suite, TestScanNodeNoDataDir);
	SUITE_ADD_TEST(suite, TestFreeStringArraySuccessful);
	/* ping */
 	SUITE_ADD_TEST(suite, TestPingSuccessful);
 	SUITE_ADD_TEST(suite, TestPingNoGlobusOnRemoteNode);
 	SUITE_ADD_TEST(suite, TestPingNodeDoesNotExist);
 	SUITE_ADD_TEST(suite, TestPingNoDataDir);

	return suite;
}

extern char *configFileName;

char *getProperty(char *prop)
{
  char *result = getFirstConfigValue("setest", prop);
  if (!result) {
    fprintf(stderr, "Missing property %s in config file\n", prop);
    exit(1);
  }
  return result;
}

void setup(void) {
  char *valid_se;
  char *valid_se_type;
  char *no_inbox_se;
  char *no_inbox_se_path;
  char *local_file_path;
  char *username;
  char *groupname;
  char *valid_group;
  char *invalid_group;
  FILE *f;
  char *fn;
	
  /* load config file */
  if (!loadConfigFile(configFileName, "setest")) {
    fprintf(stderr, "Error reading SE test config from %s\n", configFileName);
    exit(1);
  }
  valid_se = getProperty("valid_se");
  valid_se_path = getProperty("valid_se_path");
  good_se_type_ = getProperty("valid_se_type");
  no_inbox_se = getProperty("no_inbox_se");
  no_inbox_se_path = getProperty("no_inbox_se_path");
  local_file_path = getProperty("local_file_path");
  username = getProperty("username");
  groupname = getProperty("groupname");
  valid_group = getProperty("valid_group");
  invalid_group = getProperty("invalid_group");

  /* create the strings we need */
  good_filepath_ = makestring(valid_se_path, "/file1.txt");
  large_good_filepath_ = makestring(valid_se_path, "/largeFile");
  good_filepath_for_mv = makestring(valid_se_path, "/fileForMoving.txt");

  node_list_dir_ = local_file_path;

  non_existant_filepath_ = makestring(valid_se_path, "/nonExistentFile.txt");
  no_read_access_path_ = makestring(valid_se_path, "/noReadAccessDir/noReadAccessFile.txt");
  no_write_access_path_ = makestring(valid_se_path, "/noWriteAccessDir");
  no_write_access_empty_dir_ = makestring(valid_se_path, "/noWriteAccessDir/dirInsideNoWriteAccess");
  no_write_access_filepath_ = makestring(valid_se_path, "/noWriteAccessDir/uploadedFile");
  no_write_access_file_ = makestring(valid_se_path, "/noWriteAccessDir/file1.txt");
  time_dir_ = makestring(valid_se_path, "/timeDir");
  path_to_dir_ = valid_se_path;
  path_to_made_dir = makestring(valid_se_path, "/madeDir");
  path_to_file_in_new_dir = makestring(valid_se_path, "/NEW/myDir-DIR-anotherDir-DIR-file1.txt");
  path_to_other_file_in_new_dir = makestring(valid_se_path, "/NEW/myDir-DIR-anotherDir-DIR-largeFile.txt");
  dir_tree = makestring(valid_se_path, "/notHere/NotADir/Idontexist/newDir");
  file_in_dir_tree = makestring(valid_se_path, "/notHere/NotADir/file1.txt");
  middle_of_dir_tree = makestring(valid_se_path, "/notHere");

  fileOwner = username;
  fileGroup = groupname;
  otherGroup = valid_group;
  otherGroupThatUserIsNotMemberOf = invalid_group;

  fileForUpload = makestring(local_file_path, "/testData/file1.txt");
  largeFileForUpload = makestring(local_file_path, "/testData/largeFile");
  pathToStoreFile = makestring(valid_se_path, "/uploadedFile");
  pathToStoreFileLocked = makestring(valid_se_path, "/uploadedFile-LOCKED");
  pathToStoreOtherFile = makestring(valid_se_path, "/otherUploadedFile");
  pathToStoreFileNoDir = makestring(valid_se_path, "/nonExistentDir/uploadedFile");
  pathToNoDir = makestring(valid_se_path, "/nonExistentDir");
  pathToDownloadFileTo_ = makestring(local_file_path, "/testData/temp/downloadedFile");
  pathToDownloadFileToLocked_ = makestring(local_file_path, "/testData/temp/downloadedFile-LOCKED");
  pathToDownloadOtherFileTo_ = makestring(local_file_path, "/testData/temp/otherDownloadedFile");
  pathToDownloadOtherFileToLocked_ = makestring(local_file_path, "/testData/temp/otherDownloadedFile-LOCKED");
  no_write_access_pathToDownloadFileTo_ = makestring(local_file_path, "/testData/noWritePermissions/downloadedFile");
  
  good_se_ = valid_se;
  no_inbox_se_ = no_inbox_se;

  /* read in expected file and directory mod times */
  fn = makestring(local_file_path, "/filemodtime");
  f = fopen(fn, "r");
  if (!f) {
    fprintf(stderr, "Cannot open expected file modification time from %s\n",
	    fn);
    exit(1);
  }
  fgets(expectedFileTime, 100, f);
  fclose(f);
  fn = makestring(local_file_path, "/dirmodtime");
  f = fopen(fn, "r");
  if (!f) {
    fprintf(stderr, "Cannot open expected dir modification time from %s\n",
	    fn);
    exit(1);
  }
  fgets(expectedDirTime, 100, f);
  fclose(f);


  activateGlobusModules();
  startupReplicationSystem();
  getNodeInfoFromLocalFile(node_list_dir_);
  puts("setup finished");
}
