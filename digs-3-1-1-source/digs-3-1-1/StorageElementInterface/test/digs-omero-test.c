#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcdgrid-client.h"
#include "node.h"
#include "misc.h"
#include "omero.h"

int main(int argc, char *argv[])
{
  digs_error_code_t result;
  char errbuf[MAX_ERROR_MESSAGE_LENGTH];
  char **list;
  int listLength;
  int i;
  long long len;
  char *cksum;
  int handle;
  char *group;
  char *owner;
  char *perms;

  if (!qcdgridInit(1)) {
    fprintf(stderr, "Cannot initialise grid\n");
    return 1;
  }
  atexit(qcdgridShutdown);

  /*
   * Test scanning node
   */
  result = digs_scanNode_omero(errbuf, "idcn1.icmb.ed.ac.uk", &list, &listLength, 0);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error scanning node: %s\n", errbuf);
    return 1;
  }

  for (i = 0; i < listLength; i++) {
    printf("%s\n", list[i]);
  }

  /*
   * Test getting file length
   */
  result = digs_getLength_omero(errbuf, "/home/digs/omerotest/nph-zms.jpg", "idcn1.icmb.ed.ac.uk", &len);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error getting file length: %s\n", errbuf);
    return 1;
  }
  printf("Length: %qd\n", len);

  /*
   * Test getting file checksum
   */
  result = digs_getChecksum_omero(errbuf, "/home/digs/omerotest/nph-zms.jpg", "idcn1.icmb.ed.ac.uk", &cksum,
				  DIGS_MD5_CHECKSUM);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error getting file checksum: %s\n", errbuf);
    return 1;
  }
  printf("Checksum: %s\n", cksum);

  /*
   * Test getting file group
   */
  result = digs_getGroup_omero(errbuf, "/home/digs/omerotest/nph-zms.jpg", "idcn1.icmb.ed.ac.uk", &group);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error getting file group: %s\n", errbuf);
    return 1;
  }
  printf("Group: %s\n", group);

  /*
   * Test getting file owner
   */
  result = digs_getOwner_omero(errbuf, "/home/digs/omerotest/nph-zms.jpg", "idcn1.icmb.ed.ac.uk", &owner);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error getting file owner: %s\n", errbuf);
    return 1;
  }
  printf("Owner: %s\n", owner);

  /*
   * Test getting file permissions
   */
  result = digs_getPermissions_omero(errbuf, "/home/digs/omerotest/nph-zms.jpg", "idcn1.icmb.ed.ac.uk", &perms);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error getting file permissions: %s\n", errbuf);
    return 1;
  }
  printf("Permissions: %s\n", perms);

  /*
   * Test retrieving file
   */
  result = digs_startGetTransfer_omero(errbuf, "idcn1.icmb.ed.ac.uk", "/home/digs/omerotest/nph-zms.jpg", "/home/digs/downloaded.jpg", &handle);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error getting file: %s\n", errbuf);
    return 1;
  }

  /*
   * Test putting file
   */
  result = digs_startPutTransfer_omero(errbuf, "/home/digs/downloaded.jpg", "idcn1.icmb.ed.ac.uk", "/home/digs/omerotest/nph-zms2.jpg", &handle);
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error putting file: %s\n", errbuf);
    return 1;
  }

  /*
   * Test setting a file group
   */
  result = digs_setGroup_omero(errbuf, "/home/digs/omerotest/nph-zms2.jpg", "idcn1.icmb.ed.ac.uk", "jamesy1");
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error setting group: %s\n", errbuf);
    return 1;
  }

  /*
   * Test setting file permissions
   */
  result = digs_setPermissions_omero(errbuf, "/home/digs/omerotest/nph-zms2.jpg", "idcn1.icmb.ed.ac.uk", "0644");
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error setting permissions: %s\n", errbuf);
    return 1;
  }  

  /*
   * Test deleting file
   */
  result = digs_rm_omero(errbuf, "idcn1.icmb.ed.ac.uk", "/home/digs/omerotest/nph-zms2.jpg");
  if (result != DIGS_SUCCESS) {
    fprintf(stderr, "Error deleting file: %s\n", errbuf);
    return 1;
  }

  return 0;
}
