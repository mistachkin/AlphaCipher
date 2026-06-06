###############################################################################
#
# Makefile --
#
# Copyright (c) 2011-2026 by Joe Mistachkin.  All rights reserved.
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# RCS: @(#) $Id: $
#
###############################################################################
#
# NOTE: This file currently requires GNU make to operate properly.  On some
#       platforms, this may be invoked using the 'gmake' command rather than
#       the 'make' command.
#
#       All build outputs (object files, shared libraries, executables) land
#       in the $(OBJ_DIR) directory.  The source tree itself is not written
#       to.
#
#------------------------------------------------------------------------------

HOST_OS_CMD = uname -s
HOST_OS = $(shell $(HOST_OS_CMD) | sed s/-[0-9]\\.[0-9]//)

#------------------------------------------------------------------------------

CC = gcc
RM = rm
LN = ln

#------------------------------------------------------------------------------

SRC_DIR = .
SRC_OS_DIR = $(SRC_DIR)/os

OBJ_DIR = ./obj

#------------------------------------------------------------------------------

CFLAGS.Linux = -std=gnu99

CFLAGS = $(CFLAGS.$(HOST_OS))
CFLAGS += -Wall -fPIC

#------------------------------------------------------------------------------
#
# NOTE: The "-o" target paths point into $(OBJ_DIR) so that no build output
#       is written to the source directory.
#
#------------------------------------------------------------------------------

LDFLAGS.MINGW32_NT = -o $(OBJ_DIR)/ACKernel4Sdk.dll

LDFLAGS.Linux = -o $(OBJ_DIR)/libackernel4sdk.so

LDFLAGS.OpenBSD = -Wl,-soname,libackernel4sdk.so.1 \
    -o $(OBJ_DIR)/libackernel4sdk.so.1.0.1

LDFLAGS = -shared
LDFLAGS += $(LDFLAGS.$(HOST_OS))

#------------------------------------------------------------------------------
#
# NOTE: Test-executable link flags.  Object files must precede -l options so
#       that GNU ld's default --as-needed behavior does not discard the
#       library before any of its symbols have been requested.  On platforms
#       that support it, an rpath of '$ORIGIN' is embedded so the executable
#       finds the freshly built shared library in its own directory without
#       needing LD_LIBRARY_PATH.
#
#------------------------------------------------------------------------------

TEST_LDFLAGS.MINGW32_NT = -L$(OBJ_DIR) -lackernel4sdk

TEST_LDFLAGS.Linux = -L$(OBJ_DIR) -Wl,-rpath,'$$ORIGIN' -lackernel4sdk

TEST_LDFLAGS.OpenBSD = -L$(OBJ_DIR) -Wl,-rpath,'$$ORIGIN' -lackernel4sdk

TEST_LDFLAGS = $(TEST_LDFLAGS.$(HOST_OS))

#------------------------------------------------------------------------------

COMMON_DEFINES = -DACK_BUILD -DSQLITE_STATIC_LIB

#------------------------------------------------------------------------------

FEATURE_DEFINES = -DFEATURE_IMPORT -DFEATURE_RESET_CACHED_KEYS

#------------------------------------------------------------------------------

HAVE_DEFINES.MINGW32_NT = -DHAVE_INT64 -D_INT32_DEFINED -D_INT64_DEFINED \
    -D_UINT32_DEFINED -DHAVE_DIRENT_H -DHAVE_ATOLL

HAVE_DEFINES.Linux = -DHAVE_INT32_T -DHAVE_INT64_T -DHAVE_UINT32_T \
    -DHAVE_UINT64_T -DHAVE_DIRENT_H -DHAVE_ATOLL

HAVE_DEFINES.OpenBSD = -DHAVE_INT32_T -DHAVE_INT64_T -DHAVE_UINT32_T \
    -DHAVE_UINT64_T -DHAVE_DIRENT_H -DHAVE_ATOLL -DHAVE_GETTIMEOFDAY

HAVE_DEFINES = $(HAVE_DEFINES.$(HOST_OS))

#------------------------------------------------------------------------------

OS_DEFINES.MINGW32_NT = -DACK_OS_ANSI -DACK_OS_GCC -DACK_OS_WIN32 \
    -DACK_OS_SQLITE3

OS_DEFINES.Linux = -DACK_OS_ANSI -DACK_OS_GCC -DACK_OS_POSIX -DACK_OS_SQLITE3

OS_DEFINES.OpenBSD = -DACK_OS_ANSI -DACK_OS_GCC -DACK_OS_POSIX -DACK_OS_SQLITE3

OS_DEFINES = $(OS_DEFINES.$(HOST_OS))

#------------------------------------------------------------------------------

SQLITE_DEFINES = -DSQLITE_THREADSAFE=1 -DSQLITE_SECURE_DELETE=1 \
    -DSQLITE_ENABLE_COLUMN_METADATA=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 \
    -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 -DSQLITE_ENABLE_STAT3=1 \
    -DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1

SQLITE_CFLAGS =

#------------------------------------------------------------------------------

SRC_DEFINES = $(COMMON_DEFINES) $(OS_DEFINES) $(HAVE_DEFINES) $(FEATURE_DEFINES)

SRC_INCLUDES = -I$(SRC_DIR)/../include -I$(SRC_DIR)/../include/os \
    -I$(SRC_DIR)/../../../Common/Shared/include \
    -I$(SRC_DIR)/../../../Common/Externals/sqlite3

#------------------------------------------------------------------------------

LIBRARIES = -lpthread

OBJ = \
    $(OBJ_DIR)/sqlite3.o \
    $(OBJ_DIR)/navajo.o \
    $(OBJ_DIR)/navajoCache.o \
    $(OBJ_DIR)/navajoGen.o \
    $(OBJ_DIR)/navajoInt.o \
    $(OBJ_DIR)/navajoSql.o \
    $(OBJ_DIR)/navajoUtil.o \
    $(OBJ_DIR)/os.o \
    $(OBJ_DIR)/os_ansi.o \
    $(OBJ_DIR)/os_gcc.o \
    $(OBJ_DIR)/os_msvc.o \
    $(OBJ_DIR)/os_null.o \
    $(OBJ_DIR)/os_posix.o \
    $(OBJ_DIR)/os_sqlite3.o \
    $(OBJ_DIR)/os_win32.o \
    $(OBJ_DIR)/os_wince.o

#------------------------------------------------------------------------------

.PHONY: all clean test navajoSdk navajoSdkTest \
    navajoSdkTest.MINGW32_NT navajoSdkTest.Linux navajoSdkTest.OpenBSD

all: navajoSdk navajoSdkTest

navajoSdk: $(OBJ_DIR) $(OBJ)
	$(CC) $(LDFLAGS) $(LIBRARIES) $(OBJ)

$(OBJ_DIR):
	-mkdir $(OBJ_DIR)

#------------------------------------------------------------------------------

navajoSdkTest.MINGW32_NT:
	@echo "Creating symbolic links to shared library... skipped"

navajoSdkTest.Linux:
	@echo "Creating symbolic links to shared library... skipped"

navajoSdkTest.OpenBSD: navajoSdk
	@echo "Creating symbolic links to shared library..."
	$(RM) -f $(OBJ_DIR)/libackernel4sdk.so $(OBJ_DIR)/libackernel4sdk.so.1
	$(LN) -s libackernel4sdk.so.1.0.1 $(OBJ_DIR)/libackernel4sdk.so
	$(LN) -s libackernel4sdk.so.1.0.1 $(OBJ_DIR)/libackernel4sdk.so.1

navajoSdkTest: navajoSdk navajoSdkTest.$(HOST_OS) $(OBJ_DIR)/sdkTest.o
	$(CC) -o $(OBJ_DIR)/sdkTest $(OBJ_DIR)/sdkTest.o $(TEST_LDFLAGS) \
	    $(LIBRARIES)

#------------------------------------------------------------------------------

clean:
	$(RM) -rf $(OBJ_DIR)
	$(RM) -f *.a *.core *.o *~ \#* *.E
	$(RM) -f ACKernel4Sdk* libackernel4sdk* sdkTest*
	$(RM) -f core errs TAGS a.exe a.out errors lib.exp

#------------------------------------------------------------------------------
#
# NOTE: The test target stages a fresh copy of the keyset fixture under
#       $(OBJ_DIR)/test_keys so that the test can mutate offset state
#       without dirtying the source tree.  ACK_TEST_DATABASE points
#       sdkTest at the staged copy in lieu of the legacy hard-coded
#       X:\layout.pef / /keys/layout.pef path.
#
#------------------------------------------------------------------------------

TEST_KEY_SRC_DIR = $(SRC_DIR)/../keys
TEST_KEY_DIR     = $(OBJ_DIR)/test_keys

test: all
	$(RM) -rf $(TEST_KEY_DIR)
	mkdir -p $(TEST_KEY_DIR)
	cp $(TEST_KEY_SRC_DIR)/* $(TEST_KEY_DIR)/
	ACK_TEST_DATABASE=$(TEST_KEY_DIR)/layout.pef $(OBJ_DIR)/sdkTest

#------------------------------------------------------------------------------

$(OBJ_DIR)/sqlite3.o: $(SRC_DIR)/../../../Common/Externals/sqlite3/sqlite3.c
	$(CC) -c $(CFLAGS) $(SQLITE_CFLAGS) $(SQLITE_DEFINES) -o $@ $<

$(OBJ_DIR)/navajo.o: $(SRC_DIR)/navajo.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/navajoCache.o: $(SRC_DIR)/navajoCache.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/navajoGen.o: $(SRC_DIR)/navajoGen.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/navajoInt.o: $(SRC_DIR)/navajoInt.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/navajoSql.o: $(SRC_DIR)/navajoSql.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/navajoUtil.o: $(SRC_DIR)/navajoUtil.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os.o: $(SRC_OS_DIR)/os.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_ansi.o: $(SRC_OS_DIR)/os_ansi.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_gcc.o: $(SRC_OS_DIR)/os_gcc.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_msvc.o: $(SRC_OS_DIR)/os_msvc.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_null.o: $(SRC_OS_DIR)/os_null.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_posix.o: $(SRC_OS_DIR)/os_posix.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_sqlite3.o: $(SRC_OS_DIR)/os_sqlite3.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_win32.o: $(SRC_OS_DIR)/os_win32.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/os_wince.o: $(SRC_OS_DIR)/os_wince.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

$(OBJ_DIR)/sdkTest.o: $(SRC_DIR)/../test/sdkTest.c
	$(CC) -c $(CFLAGS) $(SRC_INCLUDES) $(SRC_DEFINES) -o $@ $<

# DO NOT DELETE THIS LINE -- make depend depends on it.
