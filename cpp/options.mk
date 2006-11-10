#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

# Use APR by default till Posix is complete.
USE_APR := 1

# Local options.
-include options-local.mk

## Release vs. debug build.
ifdef RELEASE
BUILD := release
CXXFLAGS := $(CXXFLAGS) -O3 -DNDEBUG
else
BUILD := debug
CXXFLAGS := $(CXXFLAGS) -ggdb3
endif

## Platform specific options
ifdef USE_APR
PLATFORM := apr
IGNORE   := posix
CXXFLAGS := $(CXXFLAGS) -DUSE_APR -I/usr/local/apr/include
LDFLAGS  := $(LDFLAGS) -L/usr/local/apr/lib -lapr-1 
else
PLATFORM := posix
IGNORE   := apr
LDFLAGS  := $(LDFLAGS) -lpthread -lrt
endif

## Build directories.

BUILD :=$(PLATFORM)-$(BUILD)
GENDIR:=build/gen
BINDIR:=build/$(BUILD)/bin
LIBDIR:=build/$(BUILD)/lib
OBJDIR:=build/$(BUILD)/obj
TESTDIR:=build/$(BUILD)/test

BUILDDIRS := $(BINDIR) $(LIBDIR) $(OBJDIR) $(TESTDIR) $(GENDIR)
SRCDIRS := src $(GENDIR)

## External dependencies:

# Add location for headers and libraries of any external dependencies here
EXTRA_INCLUDES :=
EXTRA_LIBDIRS  :=

## Compile flags

# Warnings: Enable as many as possible, keep the code clean. Please
# do not disable warnings or remove -Werror without discussing on
# qpid-dev list.
# 
# The following warnings deliberately omitted, they warn on valid code.
# -Wunreachable-code -Wpadded -Winline
# -Wshadow - warns about boost headers.
# 
WARN := -Werror -pedantic -Wall -Wextra -Wno-shadow -Wpointer-arith -Wcast-qual -Wcast-align -Wno-long-long -Wvolatile-register-var -Winvalid-pch -Wno-system-headers

INCLUDES := $(SRCDIRS:%=-I%) $(EXTRA_INCLUDES)
LDFLAGS  := $(LDFLAGS) -L$(LIBDIR) 
CXXFLAGS := $(CXXFLAGS) $(WARN) -MMD -fpic $(INCLUDES) 

## Macros for linking, must be late evaluated

# Collect object files from a collection of src subdirs
# $(call OBJ_FROM,srcdir,subdir)
OBJECTS_1 = $(patsubst $1/$2/%.cpp,$(OBJDIR)/$2/%.o,$(wildcard $1/$2/*.cpp))
OBJECTS = $(foreach src,$(SRCDIRS),$(foreach sub,$1,$(call OBJECTS_1,$(src),$(sub))))

# $(call LIBFILE,name,version)
LIBFILE =$(CURDIR)/$(LIBDIR)/libqpid_$1.so.$2

LIB_COMMAND = 	mkdir -p $(dir $@) && $(CXX) -shared -o $@ $(LDFLAGS) $(CXXFLAGS) $^

