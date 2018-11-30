###########################################################################
#
# Makefile for building QCDgrid software
#
# If you have problems, make sure the GLOBUS_LOCATION, GLOBUS_FLAVOR and CC
# variables are set correctly for your configuration
#
# Targets:
#    storagenode builds the client tools and the storage node infrastructure.
#    server      builds the control and backup threads, and storagenode.
#    jobsub      builds the job submission servers.
#    client      builds just the client tools for accessing the grid.
#
# The default build is client.
#
###########################################################################


###########################################################################
#
# Change these to suit your system.
#
###########################################################################

GLOBUS_LOCATION=/usr/local/globus-5.0.4
GLOBUS_REPLICA_LOCATION=/usr/local/globus-5.0.4
GLOBUS_FLAVOR=gcc64dbg
GLOBUS_VERSION=5
CC=gcc

OMERO=no

VOMS=yes
VOMS_LIB_LOCATION=/home/qcdgrid/glite/glite/lib64
VOMS_INCLUDE_LOCATION=/home/qcdgrid/glite/glite/include/glite/security/voms
VOMS_FLAVOR=gcc64dbg

SRM=yes
GSOAP_LOCATION=/usr/local

###########################################################################
#
# Library and include locations. Derived from above settings.
#
###########################################################################

GLOBUS_INCLUDES = -I$(GLOBUS_LOCATION)/include/$(GLOBUS_FLAVOR)pthr -I$(GLOBUS_REPLICA_LOCATION)/include/$(GLOBUS_FLAVOR)pthr
GLOBUS_LDFLAGS = -L$(GLOBUS_LOCATION)/lib -L$(GLOBUS_REPLICA_LOCATION)/lib -L$(GLOBUS_LOCATION)/lib/lib -L$(GLOBUS_REPLICA_LOCATION)/lib/lib
GLOBUS_LIBS = -ldl
GLOBUS_CFLAGS = -D_REENTRANT -D_FILE_OFFSET_BITS=64 -g

ifeq ($(GLOBUS_VERSION),5)
GLOBUS_LIB_LINKS = -lglobus_gram_client_$(GLOBUS_FLAVOR)pthr -lglobus_rls_client_$(GLOBUS_FLAVOR)pthr -lglobus_gass_copy_$(GLOBUS_FLAVOR)pthr -lglobus_gram_protocol_$(GLOBUS_FLAVOR)pthr -lglobus_gass_transfer_$(GLOBUS_FLAVOR)pthr -lglobus_ftp_client_$(GLOBUS_FLAVOR)pthr -lglobus_ftp_control_$(GLOBUS_FLAVOR)pthr -lltdl_$(GLOBUS_FLAVOR)pthr -lglobus_io_$(GLOBUS_FLAVOR)pthr -lglobus_common_$(GLOBUS_FLAVOR)pthr -lglobus_gss_assist_$(GLOBUS_FLAVOR)pthr -lglobus_gssapi_gsi_$(GLOBUS_FLAVOR)pthr -lglobus_openssl_$(GLOBUS_FLAVOR)pthr -lglobus_io_$(GLOBUS_FLAVOR)pthr -lglobus_gass_server_ez_$(GLOBUS_FLAVOR)pthr -lglobus_gsi_callback_$(GLOBUS_FLAVOR)pthr -lglobus_gsi_cert_utils_$(GLOBUS_FLAVOR)pthr -lglobus_gsi_credential_$(GLOBUS_FLAVOR) -lglobus_gsi_proxy_core_$(GLOBUS_FLAVOR)pthr -lglobus_gsi_sysconfig_$(GLOBUS_FLAVOR)pthr -lglobus_xio_$(GLOBUS_FLAVOR)pthr
else
GLOBUS_LIB_LINKS = -lglobus_gram_client_$(GLOBUS_FLAVOR)pthr -lglobus_rls_client_$(GLOBUS_FLAVOR)pthr -lglobus_gass_copy_$(GLOBUS_FLAVOR)pthr -lglobus_gram_protocol_$(GLOBUS_FLAVOR)pthr -lglobus_gass_transfer_$(GLOBUS_FLAVOR)pthr -lglobus_ftp_client_$(GLOBUS_FLAVOR)pthr -lglobus_ftp_control_$(GLOBUS_FLAVOR)pthr -lltdl_$(GLOBUS_FLAVOR)pthr -lglobus_io_$(GLOBUS_FLAVOR)pthr -lglobus_common_$(GLOBUS_FLAVOR)pthr -lglobus_gss_assist_$(GLOBUS_FLAVOR)pthr -lglobus_gssapi_gsi_$(GLOBUS_FLAVOR)pthr -lssl_$(GLOBUS_FLAVOR)pthr -lcrypto_$(GLOBUS_FLAVOR)pthr -lglobus_io_$(GLOBUS_FLAVOR)pthr -lglobus_gass_server_ez_$(GLOBUS_FLAVOR)pthr
endif


###########################################################################
#
# Definitions.
#
###########################################################################

ifeq ($(OMERO),yes)

OMERO_DIST=/home/digs/trunk/dist
ICE_HOME=/home/digs/Ice

COMPILE_OPTIONS = -fPIC -O3 $(GLOBUS_INCLUDES) $(GLOBUS_CFLAGS) -I./src -I./js/src -I./StorageElementInterface/src -I$(OMERO_DIST)/include -I$(ICE_HOME)/include -DOMERO -Wall
LINK_OPTIONS = $(GLOBUS_LDFLAGS) $(GLOBUS_LIBS) $(GLOBUS_LIB_LINKS) -L. -L$(OMERO_DIST)/lib -L$(ICE_HOME)/lib -lIce -lIceUtil -lGlacier2 -lOMERO_client -lOMERO_common -lstdc++

QCDGRID_OBJS = obj/client.o obj/misc.o obj/node.o obj/job.o obj/gridftp.o obj/gridftp-common.o obj/replica.o obj/config.o obj/md5.o obj/diskspace.o obj/hashtable.o obj/omero.o obj/CommentAnnotation.o obj/CommentAnnotationI.o
BACKGROUND_OBJS = $(QCDGRID_OBJS) obj/verify.o obj/background-delete.o obj/background-new.o obj/background-permissions.o obj/background-msg.o obj/background-modify.o obj/repqueue.o
CXX=g++

else

COMPILE_OPTIONS = -fPIC -O3 $(GLOBUS_INCLUDES) $(GLOBUS_CFLAGS) -I./src -I./js/src -I./StorageElementInterface/src  -Wall -ansi -pedantic -std=c99
LINK_OPTIONS = $(GLOBUS_LDFLAGS) $(GLOBUS_LIBS) $(GLOBUS_LIB_LINKS) -L.

QCDGRID_OBJS = obj/client.o obj/misc.o obj/node.o obj/job.o obj/gridftp.o obj/gridftp-common.o obj/replica.o obj/config.o obj/md5.o obj/diskspace.o obj/hashtable.o
BACKGROUND_OBJS = $(QCDGRID_OBJS) obj/verify.o obj/background-delete.o obj/background-new.o obj/background-permissions.o obj/background-msg.o obj/background-modify.o obj/repqueue.o

endif

ifeq ($(SRM),yes)
COMPILE_OPTIONS += -DWITH_SRM -DWITH_OPENSSL -I$(GSOAP_LOCATION)/include
LINK_OPTIONS += -L$(GSOAP_LOCATION)/lib -lgsigsoap
QCDGRID_OBJS += obj/srm.o obj/soapC.o obj/soapClientLib.o obj/srm-transfer-gridftp.o
endif

ifeq ($(VOMS),yes)
COMPILE_OPTIONS += -DWITH_VOMS -I$(VOMS_INCLUDE_LOCATION)
LINK_OPTIONS += -L$(VOMS_LIB_LOCATION) -lvomsc_$(VOMS_FLAVOR) -lvomsapi_$(VOMS_FLAVOR)
endif

###########################################################################
#
# Build all servers and clients.
#
###########################################################################

default: client


all: init libqcdgridclient.so client admin server internal jobsub


###########################################################################
#
# Initialise all builds.
#
###########################################################################

init:
# Tests if "obj" directory exists. If not, then it creates it.
	@if [ ! -d "${PWD}/obj" ] ; then \
	mkdir obj; \
	fi

###########################################################################
#
# Cleanup.
#
###########################################################################


clean:
	rm -rf obj

cleanall: clean
	rm -f digs-get background digs-i-like-this-file digs-add-node digs-remove-node digs-disable-node digs-enable-node digs-list digs-delete digs-verify-rc digs-delete-rc digs-rebuild-rc digs-retire-node digs-unretire-node qcdgrid-checksum digs-chmod digs-make-private digs-make-public digs-ping digs-check-lfn digs-lock digs-unlock digs-replica-count digs-modify digs-job-submit qcdgrid-job-wrapper qcdgrid-job-controller qcdgrid-job-getdir qcdgrid-job-test libqcdgridclient.so digs-omero-test

###########################################################################
#
# Client builds just those that are needed for client access.
#
###########################################################################

ifeq ($(OMERO),yes)

client: init libqcdgridclient.so get-file-from-qcdgrid i-like-this-file qcdgrid-delete qcdgrid-job-submit digs-list qcdgrid-ping digs-make-private digs-make-public digs-check-lfn digs-lock digs-unlock digs-replica-count digs-modify digs-omero-test

else

client: init libqcdgridclient.so get-file-from-qcdgrid i-like-this-file qcdgrid-delete qcdgrid-job-submit digs-list qcdgrid-ping digs-make-private digs-make-public digs-check-lfn digs-lock digs-unlock digs-replica-count digs-modify

endif

###########################################################################
#
# Builds the internal node commands for the data grid.
#
###########################################################################

internal: init libqcdgridclient.so qcdgrid-checksum digs-chmod digs-check-lfn


###########################################################################
#
# If you want to set up a storage node.
#
###########################################################################

storagenode: internal


###########################################################################
#
# Builds the administrative tools for the data grid.
# 
##########################################################################

admin: init libqcdgridclient.so add-qcdgrid-node delete-qcdgrid-rc disable-qcdgrid-node enable-qcdgrid-node rebuild-qcdgrid-rc remove-qcdgrid-node retire-qcdgrid-node unretire-qcdgrid-node verify-qcdgrid-rc digs-check-lfn


###########################################################################
#
# Builds the control thread and backup.
#
###########################################################################

server: init background admin storagenode


###########################################################################
#
# Jobsub builds the job submission system.
#
###########################################################################

jobsub: init libqcdgridclient.so qcdgrid-job-controller qcdgrid-job-getdir qcdgrid-job-test qcdgrid-job-wrapper


###########################################################################
#
# Targets for the Job submission system.
#
###########################################################################

qcdgrid-job-wrapper: js/src/qcdgrid-job-wrapper.c obj/jobdesc.o ; $(CC) -o qcdgrid-job-wrapper js/src/qcdgrid-job-wrapper.c obj/jobdesc.o $(COMPILE_OPTIONS)
qcdgrid-job-controller: js/src/qcdgrid-job-controller.c obj/jobdesc.o obj/batch.o obj/batch-none.o obj/batch-pbs.o $(QCDGRID_OBJS) ; $(CC) -o qcdgrid-job-controller js/src/qcdgrid-job-controller.c obj/jobdesc.o obj/batch.o obj/batch-none.o obj/batch-pbs.o $(COMPILE_OPTIONS) -lpthread -L. -lqcdgridclient $(LINK_OPTIONS)

qcdgrid-job-submit: js/src/qcdgrid-job-submit.c obj/jobio.o $(QCDGRID_OBJS) ; $(CC) -o digs-job-submit js/src/qcdgrid-job-submit.c obj/jobio.o $(COMPILE_OPTIONS) -L. -lqcdgridclient $(LINK_OPTIONS)

qcdgrid-job-getdir: js/src/qcdgrid-job-getdir.c ; $(CC) -o qcdgrid-job-getdir js/src/qcdgrid-job-getdir.c $(COMPILE_OPTIONS)

###########################################################################
#
# Targets for the data grid internals and servers.
#
###########################################################################

qcdgrid-ping: src/qcdgrid-ping.c $(QCDGRID_OBJS) ; $(CC) -o digs-ping src/qcdgrid-ping.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

qcdgrid-job-test: src/qcdgrid-job-test.c $(QCDGRID_OBJS) ; $(CC) -o digs-job-test src/qcdgrid-job-test.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

retire-qcdgrid-node: src/retire-qcdgrid-node.c libqcdgridclient.so ; $(CC) -o digs-retire-node src/retire-qcdgrid-node.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

unretire-qcdgrid-node: src/unretire-qcdgrid-node.c libqcdgridclient.so ; $(CC) -o digs-unretire-node src/unretire-qcdgrid-node.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

delete-qcdgrid-rc: src/delete-qcdgrid-rc.c $(QCDGRID_OBJS) ; $(CC) -o digs-delete-rc src/delete-qcdgrid-rc.c $(QCDGRID_OBJS) $(COMPILE_OPTIONS) $(LINK_OPTIONS)

rebuild-qcdgrid-rc: src/rebuild-qcdgrid-rc.c $(QCDGRID_OBJS) ; $(CC) -o digs-rebuild-rc src/rebuild-qcdgrid-rc.c $(QCDGRID_OBJS) $(COMPILE_OPTIONS) $(LINK_OPTIONS)

verify-qcdgrid-rc: src/verify-qcdgrid-rc.c $(QCDGRID_OBJS) obj/verify.o ; $(CC) -o digs-verify-rc src/verify-qcdgrid-rc.c $(QCDGRID_OBJS) obj/verify.o $(COMPILE_OPTIONS) $(LINK_OPTIONS)

qcdgrid-delete: src/qcdgrid-delete.c libqcdgridclient.so ; $(CC) -o digs-delete src/qcdgrid-delete.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-list: src/digs-list.c libqcdgridclient.so ; $(CC) -o digs-list src/digs-list.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

add-qcdgrid-node: src/add-qcdgrid-node.c libqcdgridclient.so ; $(CC) -o digs-add-node src/add-qcdgrid-node.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

remove-qcdgrid-node: src/remove-qcdgrid-node.c libqcdgridclient.so ; $(CC) -o digs-remove-node src/remove-qcdgrid-node.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

disable-qcdgrid-node: src/disable-qcdgrid-node.c libqcdgridclient.so ; $(CC) -o digs-disable-node src/disable-qcdgrid-node.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

enable-qcdgrid-node: src/enable-qcdgrid-node.c libqcdgridclient.so ; $(CC) -o digs-enable-node src/enable-qcdgrid-node.c $(COMPILE_OPTIONS) -lqcdgridclient -L. $(LINK_OPTIONS)

qcdgrid-checksum: src/qcdgrid-checksum.c obj/md5.o ; $(CC) -o qcdgrid-checksum src/qcdgrid-checksum.c obj/md5.o $(COMPILE_OPTIONS)

digs-chmod: src/digs-chmod.c ; $(CC) -o digs-chmod src/digs-chmod.c $(COMPILE_OPTIONS)

background: src/background.c $(BACKGROUND_OBJS) ; $(CC) -o background src/background.c $(BACKGROUND_OBJS) $(COMPILE_OPTIONS) $(LINK_OPTIONS)

get-file-from-qcdgrid: src/get-file-from-qcdgrid.c libqcdgridclient.so ; $(CC) -o digs-get src/get-file-from-qcdgrid.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

i-like-this-file: src/i-like-this-file.c libqcdgridclient.so ; $(CC) -o digs-i-like-this-file src/i-like-this-file.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-make-private: src/digs-make-private.c libqcdgridclient.so ; $(CC) -o digs-make-private src/digs-make-private.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-make-public: src/digs-make-public.c libqcdgridclient.so ; $(CC) -o digs-make-public src/digs-make-public.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-check-lfn: src/digs-check-lfn.c libqcdgridclient.so ; $(CC) -o digs-check-lfn src/digs-check-lfn.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-lock: src/digs-lock.c libqcdgridclient.so ; $(CC) -o digs-lock src/digs-lock.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-unlock: src/digs-unlock.c libqcdgridclient.so ; $(CC) -o digs-unlock src/digs-unlock.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-replica-count: src/digs-replica-count.c libqcdgridclient.so ; $(CC) -o digs-replica-count src/digs-replica-count.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

digs-modify: src/digs-modify.c libqcdgridclient.so ; $(CC) -o digs-modify src/digs-modify.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)

libqcdgridclient.so : $(QCDGRID_OBJS) ; $(CC) -shared -o libqcdgridclient.so $(QCDGRID_OBJS) $(LINK_OPTIONS)

testSE: init  obj/CuTest.o obj/globusSETest.o $(QCDGRID_OBJS) StorageElementInterface/test/runAllTests.c obj/CuTest.o
	$(CC) $(CFLAGS) $(LINK_OPTIONS) $(COMPILE_OPTIONS) -o StorageElementInterface/runAllTests StorageElementInterface/test/runAllTests.c StorageElementInterface/test/globusSETest.c obj/CuTest.o $(QCDGRID_OBJS)

ifeq ($(OMERO),yes)
digs-omero-test: StorageElementInterface/test/digs-omero-test.c libqcdgridclient.so
	$(CC) -o digs-omero-test StorageElementInterface/test/digs-omero-test.c -lqcdgridclient $(COMPILE_OPTIONS) $(LINK_OPTIONS)
endif

###########################################################################
#
# Targets for the object files
#
###########################################################################

ifeq ($(OMERO),yes)
obj/omero.o : StorageElementInterface/src/omero.cpp ; $(CXX) -c -o obj/omero.o StorageElementInterface/src/omero.cpp $(COMPILE_OPTIONS)
obj/CommentAnnotation.o : StorageElementInterface/src/CommentAnnotation.cpp ; $(CXX) -c -o obj/CommentAnnotation.o StorageElementInterface/src/CommentAnnotation.cpp $(COMPILE_OPTIONS)
obj/CommentAnnotationI.o : StorageElementInterface/src/CommentAnnotationI.cpp ; $(CXX) -c -o obj/CommentAnnotationI.o StorageElementInterface/src/CommentAnnotationI.cpp $(COMPILE_OPTIONS)
endif

ifeq ($(SRM),yes)
obj/srm.o : StorageElementInterface/src/srm.c
	$(CC) -c -o obj/srm.o StorageElementInterface/src/srm.c $(COMPILE_OPTIONS)
obj/srm-transfer-gridftp.o : StorageElementInterface/src/srm-transfer-gridftp.c
	$(CC) -c -o obj/srm-transfer-gridftp.o StorageElementInterface/src/srm-transfer-gridftp.c $(COMPILE_OPTIONS)
obj/soapC.o : StorageElementInterface/src/soapC.c
	$(CC) -c -o obj/soapC.o StorageElementInterface/src/soapC.c $(COMPILE_OPTIONS)
obj/soapClientLib.o : StorageElementInterface/src/soapClientLib.c
	$(CC) -c -o obj/soapClientLib.o StorageElementInterface/src/soapClientLib.c $(COMPILE_OPTIONS)
endif

obj/gridftp.o : StorageElementInterface/src/gridftp.c ; $(CC) -c -o obj/gridftp.o StorageElementInterface/src/gridftp.c $(COMPILE_OPTIONS)
obj/gridftp-common.o : StorageElementInterface/src/gridftp-common.c ; $(CC) -c -o obj/gridftp-common.o StorageElementInterface/src/gridftp-common.c $(COMPILE_OPTIONS)
obj/node.o : src/node.c ; $(CC) -c -o obj/node.o src/node.c $(COMPILE_OPTIONS)
obj/replica.o : src/replica.c ; $(CC) -c -o obj/replica.o src/replica.c $(COMPILE_OPTIONS)
obj/job.o : src/job.c ; $(CC) -c -o obj/job.o src/job.c $(COMPILE_OPTIONS)
obj/misc.o : src/misc.c ; $(CC) -c -o obj/misc.o src/misc.c $(COMPILE_OPTIONS)
obj/config.o : src/config.c ; $(CC) -c -o obj/config.o src/config.c $(COMPILE_OPTIONS)
obj/verify.o : src/verify.c ; $(CC) -c -o obj/verify.o src/verify.c $(COMPILE_OPTIONS)
obj/md5.o : src/md5.c ; $(CC) -c -o obj/md5.o src/md5.c $(COMPILE_OPTIONS)
obj/client.o : src/client.c ; $(CC) -c -o obj/client.o src/client.c $(COMPILE_OPTIONS)
obj/hashtable.o : src/hashtable.c ; $(CC) -c -o obj/hashtable.o src/hashtable.c $(COMPILE_OPTIONS)

obj/background-delete.o : src/background-delete.c ; $(CC) -c -o obj/background-delete.o src/background-delete.c $(COMPILE_OPTIONS)
obj/background-new.o : src/background-new.c ; $(CC) -c -o obj/background-new.o src/background-new.c $(COMPILE_OPTIONS)
obj/background-permissions.o : src/background-permissions.c ; $(CC) -c -o obj/background-permissions.o src/background-permissions.c $(COMPILE_OPTIONS)
obj/background-msg.o : src/background-msg.c ; $(CC) -c -o obj/background-msg.o src/background-msg.c $(COMPILE_OPTIONS)
obj/background-modify.o : src/background-modify.c ; $(CC) -c -o obj/background-modify.o src/background-modify.c $(COMPILE_OPTIONS)
obj/repqueue.o : src/repqueue.c ; $(CC) -c -o obj/repqueue.o src/repqueue.c $(COMPILE_OPTIONS)
obj/diskspace.o: src/diskspace.c ; $(CC) -c -o obj/diskspace.o src/diskspace.c $(COMPILE_OPTIONS)
obj/jobdesc.o: js/src/jobdesc.c ; $(CC) -c -o obj/jobdesc.o js/src/jobdesc.c $(COMPILE_OPTIONS)
obj/batch.o: js/src/batch.c ; $(CC) -c -o obj/batch.o js/src/batch.c $(COMPILE_OPTIONS)
obj/batch-none.o: js/src/batch-none.c ; $(CC) -c -o obj/batch-none.o js/src/batch-none.c $(COMPILE_OPTIONS)
obj/batch-pbs.o: js/src/batch-pbs.c ; $(CC) -c -o obj/batch-pbs.o js/src/batch-pbs.c $(COMPILE_OPTIONS)
obj/jobio.o: js/src/jobio.c ; $(CC) -c -o obj/jobio.o js/src/jobio.c $(COMPILE_OPTIONS)

obj/CuTest.o : StorageElementInterface/test/CuTest.c ; $(CC) $(CFLAGS) -c -o obj/CuTest.o  StorageElementInterface/test/CuTest.c  $(COMPILE_OPTIONS)
obj/globusSETest.o : StorageElementInterface/test/globusSETest.c ; $(CC) $(CFLAGS) -c -o obj/globusSETest.o  StorageElementInterface/test/globusSETest.c  $(COMPILE_OPTIONS)
