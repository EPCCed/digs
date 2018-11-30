#
# Makefile for building SE test suite
#

GLOBUS_LOCATION=/opt/vdt/globus
GLOBUS_FLAVOR=gcc32dbg

VOMS=yes
VOMS_LOCATION=/opt/vdt/glite
VOMS_FLAVOR=gcc32dbg

SRM=yes
GSOAP_LOCATION=/usr/local

TEST_OBJS=runAllTests.o globusSETest.o CuTest.o CuTestTest.o
DIGS_OBJS=../../obj/gridftp.o ../../obj/gridftp-common.o ../../obj/node.o ../../obj/misc.o ../../obj/config.o ../../obj/md5.o ../../obj/replica.o ../../obj/job.o ../../obj/hashtable.o

GLOBUS_LIB_LINKS = -lglobus_gram_client_$(GLOBUS_FLAVOR)pthr -lglobus_rls_client_$(GLOBUS_FLAVOR)pthr -lglobus_gass_copy_$(GLOBUS_FLAVOR)pthr -lglobus_gram_protocol_$(GLOBUS_FLAVOR)pthr -lglobus_gass_transfer_$(GLOBUS_FLAVOR)pthr -lglobus_ftp_client_$(GLOBUS_FLAVOR)pthr -lglobus_ftp_control_$(GLOBUS_FLAVOR)pthr -lltdl_$(GLOBUS_FLAVOR)pthr -lglobus_io_$(GLOBUS_FLAVOR)pthr -lglobus_common_$(GLOBUS_FLAVOR)pthr -lglobus_gss_assist_$(GLOBUS_FLAVOR)pthr -lglobus_gssapi_gsi_$(GLOBUS_FLAVOR)pthr -lssl_$(GLOBUS_FLAVOR)pthr -lcrypto_$(GLOBUS_FLAVOR)pthr -lglobus_io_$(GLOBUS_FLAVOR)pthr -lglobus_gass_server_ez_$(GLOBUS_FLAVOR)pthr

CFLAGS=-I../../src -I../src -I$(GLOBUS_LOCATION)/include/$(GLOBUS_FLAVOR)pthr -std=c99
LDFLAGS=-L$(GLOBUS_LOCATION)/lib $(GLOBUS_LIB_LINKS)

ifeq ($(SRM),yes)
DIGS_OBJS += ../../obj/srm.o ../../obj/srm-transfer-gridftp.o ../../obj/soapC.o ../../obj/soapClientLib.o
CFLAGS += -DWITH_SRM -I$(GSOAP_LOCATION)/include
LDFLAGS += -L$(GSOAP_LOCATION)/lib -lgsigsoap
endif

ifeq ($(VOMS),yes)
CFLAGS += -DWITH_VOMS -I$(VOMS_LOCATION)/include/glite/security/voms
LDFLAGS += -L$(VOMS_LOCATION)/lib -lvomsc_$(VOMS_FLAVOR) -lvomsapi_$(VOMS_FLAVOR)
endif

all: runAllTests setup-se-test

setup-se-test: setup-se-test.o $(DIGS_OBJS)
	gcc -o setup-se-test setup-se-test.o $(DIGS_OBJS) $(LDFLAGS)

setup-se-test.o: setup-se-test.c
	gcc -c -o setup-se-test.o setup-se-test.c $(CFLAGS)

runAllTests: $(TEST_OBJS) $(DIGS_OBJS)
	gcc -o runAllTests $(TEST_OBJS) $(DIGS_OBJS) $(LDFLAGS)

runAllTests.o: runAllTests.c	
	gcc -c -o runAllTests.o runAllTests.c $(CFLAGS)

globusSETest.o: globusSETest.c
	gcc -c -o globusSETest.o globusSETest.c $(CFLAGS)

CuTest.o: CuTest.c
	gcc -c -o CuTest.o CuTest.c $(CFLAGS)

CuTestTest.o: CuTestTest.c
	gcc -c -o CuTestTest.o CuTestTest.c $(CFLAGS)
