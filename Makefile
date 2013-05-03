# coding: utf-8
#
# Copyright (C) 2013, Niklas Rosenstein
#
# Template makefile for compiling Cinema 4D plugins from the command-line
# using the [Cinema 4D makefile collection][1] by Niklas Rosenstein. This
# makefile can be copied into a plugins' directory and modified to fit
# your needs.
#
# [1]: https://github.com/NiklasRosenstein/make-c4d

# Include the Cinema 4D makefile collection relative to the plugin
# directory. Must be adjusted when built from another location.
include ../api/c4d.mak

# The base-directory for source-files.
BASEDIR_SRC = src

# Basename of the target library.
TARGET_NAME = containerobject

# Collect a list of all source-files from the source-files directory.
SOURCES = $(wildcard $(BASEDIR_SRC)/*.cpp)

# Generate the list of object files from the source-file. The object
# files will be put into an obj/{platform}[-dbg] directory.
OBJECTS = $(call C4D_GEN_OBJECTNAMES,$(C4D_OBJECTS_DIR)/,$(BASEDIR_SRC)/,$(SOURCES))

# A list of directories that may not exist and need to be created
# before compilation can be done.
DIRS = $(sort $(dir $(OBJECTS)))

# A list of additional include directories. Will be prefixed later.
INCLUDES = ./ res/description/ include/

# Generate the target output-name.
TARGET = $(call C4D_GEN_DLLNAME, $(TARGET_NAME))

# Description: Builds the target.
plugin: $(TARGET)

# Description: Link all object-files into a dynamic library.
$(TARGET): $(DIRS) $(OBJECTS)
	$(call C4D_LINK_DLL,$@) $(OBJECTS)

# Description: Compile *.cpp files into object-files.
$(C4D_OBJECTS_DIR)/%.$(C4D_SUFFIX_OBJ): $(BASEDIR_SRC)/%.cpp
	$(call C4D_COMPILE,$@) $< $(call C4D_GEN_INCLUDES, $(INCLUDES))

# Description: Generate the required directories, suppressing any errors,
#              like directories that already exist.
$(DIRS):
	mkdir -p $(DIRS)

