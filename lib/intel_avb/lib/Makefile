# Intel AVB HAL Library Makefile
#
# Copyright (c) 2025, Intel Corporation
# All rights reserved.

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -fPIC
INCLUDES = -I. -I../../../lib/common
LDFLAGS = -shared

# Source files
SOURCES = intel.c intel_common.c intel_i210.c intel_i219.c intel_i225.c
OBJECTS = $(SOURCES:.c=.o)

# Target library
TARGET = libintel_avb.so
STATIC_TARGET = libintel_avb.a

# Header files
HEADERS = intel.h intel_private.h

# Build rules
all: $(TARGET) $(STATIC_TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(STATIC_TARGET): $(OBJECTS)
	ar rcs $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Install rules
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include

install: $(TARGET) $(STATIC_TARGET)
	mkdir -p $(LIBDIR)
	mkdir -p $(INCDIR)
	cp $(TARGET) $(LIBDIR)/
	cp $(STATIC_TARGET) $(LIBDIR)/
	cp intel.h $(INCDIR)/

# Clean rules
clean:
	rm -f $(OBJECTS) $(TARGET) $(STATIC_TARGET)

# Development rules
test: $(STATIC_TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) -o test_intel test_intel.c -L. -lintel_avb

debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Windows specific rules
windows: CC = x86_64-w64-mingw32-gcc
windows: TARGET = intel_avb.dll
windows: LDFLAGS = -shared -Wl,--out-implib,libintel_avb.dll.a
windows: $(TARGET)

# Dependencies
intel.o: intel.c intel.h intel_private.h
intel_common.o: intel_common.c intel.h intel_private.h
intel_i210.o: intel_i210.c intel.h intel_private.h
intel_i219.o: intel_i219.c intel.h intel_private.h
intel_i225.o: intel_i225.c intel.h intel_private.h

.PHONY: all clean install test debug windows
