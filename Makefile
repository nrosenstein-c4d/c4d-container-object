# Copyright (C) 2012-2013, Niklas Rosenstein
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# ----------------------------------------------------------------------------
#
# Template makefile for compiling Cinema 4D plugins from the command-line
# using the [Cinema 4D makefile collection][1] by Niklas Rosenstein. This
# makefile can be copied into a plugins' directory and modified to fit
# your needs.
#
# Targets:
# - plugin: Builds the plugin
# - clear: Removes all built object-files
#
# For rebuilding the plugin completely, use `make clear plugin`.
#
#
# [1]: https://github.com/NiklasRosenstein/c4d-make

# Description: Default make command. Prints usage.
# default:
# 	@echo "Usage:"
# 	@echo "make clear        Clear all object files"
# 	@echo "make dirs         Create all required directories for building the plugin"
# 	@echo "make plugin       Build the plugin."
# 	@echo ""
# 	@echo "Environment Variables:"
# 	@echo "DEBUG             Define to create a debug-build."
# 	@echo "C4D_ARCHITECTURE  x86 by default, set to x64 for 64-bit build."

ifndef SUBDIR
  SUBDIR := ..
endif

# The path to the Cinema 4D resource folder must be adjusted when
# building from another path than the plugins folder.
C4D_RESOURCE_PATH = $(SUBDIR)/../resource

# Include the Cinema 4D makefile collection relative to the plugin
# directory. Must be adjusted when built from another location.
include $(SUBDIR)/api/c4d.mak

# The base-directory for source-files.
BASEDIR_SRC = src
BASEDIR_INCLUDE = include

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
INCLUDES = res/description/ $(BASEDIR_INCLUDE)/

# Determine the Python Framework's base.
# --------------------------------------

ifeq ($(C4D_PLATFORM),mac)
  PYX = osx
else ifeq ($(C4D_PLATFORM), win)
  ifeq ($(C4D_ARCHITECTURE),x86)
    PYX = win32
  else ifeq ($(C4D_ARCHITECTURE),x64)
    PYX = win64
  else
    $(error Unsupported Architecture $(C4D_ARCHITECTURE))
  endif
else
  $(error Unsupported platform)
endif
PYFW = $(C4D_RESOURCE_PATH)/modules/python/res/Python.$(PYX).framework

# And now add the Python include directory.
#INCLUDES += $(PYFW)/include
#LIBS += $(PYFW)/libs/python26.lib

# Generate the target output-name.
TARGET = $(call C4D_GEN_DLLNAME, $(TARGET_NAME))

# Description: Shortcut target name for building the plugin.
plugin: $(TARGET)

# Description: Link all object-files into a dynamic library.
$(TARGET): dirs $(OBJECTS)
	$(call C4D_LINK_DLL,$@) $(LIBS) $(OBJECTS)

# Description: Compile *.cpp files into object-files, even when the
#              header file changed.
$(C4D_OBJECTS_DIR)/%.$(C4D_SUFFIX_OBJ): $(BASEDIR_SRC)/%.cpp $(BASEDIR_INCLUDE)/%.h
	 $(call C4D_COMPILE,$@) $(call C4D_GEN_DEFINES,$(DEFINES)) $(call C4D_GEN_INCLUDES, $(INCLUDES)) $<

# Description: Compile *.cpp files into object-files.
$(C4D_OBJECTS_DIR)/%.$(C4D_SUFFIX_OBJ): $(BASEDIR_SRC)/%.cpp
	$(call C4D_COMPILE,$@) $(call C4D_GEN_DEFINES,$(DEFINES)) $(call C4D_GEN_INCLUDES, $(INCLUDES)) $<



