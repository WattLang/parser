# Structure of the project: 
#
# /
#     build/  
#         shared/
#             libproject-name.so
#             src/
#                 object files...
#         static/
#             libproject-name.a
#             src/
#                 object files...
#         executable/
#             project-name
#             src/
#                 object files
#     src/
#         sources files...
#     inc/
#         header files...

# Note: inc and src can be the same folder

#####
##### FOLDER SETTINGS
#####

SRC_FOLDER := src
INC_FOLDER := include

BUILD_FOLDER := build
BUILD_EXE_FOLDER := $(BUILD_FOLDER)/executable

#####
##### GENERAL SETTINGS
#####

PROJECT_NAME := parser
CXX := g++

# Targets
TARGET_EXE := $(BUILD_EXE_FOLDER)/$(PROJECT_NAME)

# Target to build when `make` or `make all` is typed
TARGET_ALL := $(TARGET_EXE)

#####
##### FILES SETTINGS
#####

# Extension
EXT_SRC_FILE = .cpp
EXT_INC_FILE = .hpp

# Get header from source file
# This function is only used to checks if the header has bee modified and the object file must be rebuild
# So if the header doesn't exist it's ok

# The first argument is the source file relative to $(SRC_FOLDER)
# The header must be relative to $(INC_FOLDER)

# EX: $(1:%$(EXT_SRC_FILE)=%$(EXT_INC_FILE)) 
# will take the file "folder/sub_folder_file.cpp"
# and transform it into "folder/sub_folder_file.hpp"
header-of = $(1:%$(EXT_SRC_FILE)=%$(EXT_INC_FILE))

# Relative to $(SRC_FOLDER)
SRC_EXCLUDE_FILE := 
# All files that are not use for libraries, don't add src/
SRC_MAINS := test.cpp main.cpp
# The main file to use (must be in $(SRC_MAINS))
SRC_MAIN := main.cpp

#####
##### FLAGS
#####

FLAGS := -std=c++17 -g3 -Wall -Wextra -Wno-pmf-conversions -O2

# Include path
# Must be use with -I
INC_FLAG := -I $(INC_FOLDER) -I lib/

#####
##### LIBRARY
#####

# Path to libaries if not in $PATH, for example (relative to the project folder): lib/
# Must be use with -L
LIBS_PATH := -L lib/

# For example: -lsfml-graphics
LIBS := 

# Library that require to be build
LIB_TO_BUILD := 

# Create rules to build the libraries


###############################################
#                   PRIVATE                   #
###############################################

#####
##### OTHER
#####

_RESET := \033[0m
_BOLD := \033[1m

_COLOR_GREEN := \033[32m
_COLOR_MAGENTA := \033[35m
_COLOR_CYAN := \033[36m
_COLOR_WHITE := \033[37m

MAKEFLAGS += --no-print-directory

#####
##### FUNCTIONS
#####

_void =
_space = $(_void) $(_void)
_comma = ,

# join <between> <list>
_join = $(subst $(_space),$(1),$(2))

# _header <message>
_header = echo -e "$(_COLOR_CYAN)$(_BOLD)>>> $(1)$(_RESET)"
# _sub-header <message>
_sub-header = echo -e "$(_COLOR_GREEN)>>> $(1)$(_RESET)"
# _build-msg <target> <from>
_build-msg = echo -e "$(_COLOR_WHITE):: Building $(_BOLD)$(1)$(_RESET)$(_COLOR_WHITE) from $(_BOLD)$(2)$(_RESET)"
# _special <message>
_special = echo -e "$(_COLOR_MAGENTA)$(_BOLD)\# $(1)$(_RESET)"

# _remove-folder <folder>
define _remove-folder
	rm -rf $(1)
endef

#####
##### SOURCES
#####

_SRC_MAINS := $(addprefix $(SRC_FOLDER)/,$(SRC_MAINS))
# All sources files not main
_ALL_SRC_FILES := $(shell find $(SRC_FOLDER) -name '*$(EXT_SRC_FILE)')
_SRC_FILES := $(filter-out $(_SRC_MAINS),$(_ALL_SRC_FILES))

#####
##### DIRECTORIES
#####

# All sources file directories
_SRC_DIR := $(sort $(dir $(_ALL_SRC_FILES)))

_EXE_DIR := $(addprefix $(BUILD_EXE_FOLDER)/,$(_SRC_DIR))

_BUILD_DIR := $(_EXE_DIR) $(BUILD_EXE_FOLDER) 

#####
##### OBJECT FILES
##### 

_OBJ_MAIN := $(SRC_MAIN:%$(EXT_SRC_FILE)=$(BUILD_EXE_FOLDER)/$(SRC_FOLDER)/%.o)
_OBJ_SRC_EXE := $(_OBJ_MAIN) $(_SRC_FILES:%$(EXT_SRC_FILE)=$(BUILD_EXE_FOLDER)/%.o) 

_LIB_PATH_LD := $(call _join,:,$(strip $(filter-out -L,$(LIBS_PATH))))
export LD_LIBRARY_PATH += $(_LIB_PATH_LD)

#####
##### RULES
#####

.PHONY: all executable test
.PHONY: clean
.PHONY: re re-test
.PHONY: re-run run run-test re-run-test

.DEFAULT_GOAL := all

all:
ifneq ($(findstring $(TARGET_EXE),$(TARGET_ALL)),)
	@$(MAKE) executable
endif

executable: 
	@$(call _header,BUILDING EXECUTABLE...)
	@$(MAKE) $(TARGET_EXE)

test:
	@$(MAKE) PROJECT_NAME=parser_test SRC_MAIN=test.cpp

clean:
	@$(call _header,REMOVING $(BUILD_FOLDER))
	@$(call _remove-folder,$(BUILD_FOLDER))

where-executable:
	@echo $(TARGET_EXE)

re:
	@$(MAKE) clean
	@$(MAKE)

re-test:
	@$(MAKE) re PROJECT_NAME=parser_test SRC_MAIN=test.cpp

run:
	@$(MAKE) executable
	@echo
	@$(call _special,EXECUTING $(TARGET_EXE)...)
	@$(TARGET_EXE) $(args); ERR=$$?; $(call _special,PROGRAM HALT WITH CODE $$ERR); exit $$ERR;

run-test:
	@$(MAKE) run PROJECT_NAME=parser_test SRC_MAIN=test.cpp

re-run:
	@$(MAKE) re
	@$(MAKE) run

re-run-test:
	@$(MAKE) re-run PROJECT_NAME=parser_test SRC_MAIN=test.cpp

valgrind:
	@$(MAKE) executable
	@echo
	@$(call _special,EXECUTING $(TARGET_EXE) WITH VALGRIND...)
	@valgrind $(TARGET_EXE) $(args); ERR=$$?; $(call _special,PROGRAM HALT WITH CODE $$ERR); exit $$ERR;

re-valgrind:
	@$(MAKE) re-executable
	@$(MAKE) valgrind

$(_BUILD_DIR):
	@mkdir -p $(_BUILD_DIR)

###


$(TARGET_EXE): $(_BUILD_DIR) $(LIB_TO_BUILD) $(_OBJ_SRC_EXE)
	@$(call _sub-header,Linking...)
	@$(CXX) $(INC_FLAG) $(FLAGS) $(_OBJ_SRC_EXE) -o "$@" $(LIBS_PATH) $(LIBS)
	@$(call _header,Executable done ($(TARGET_EXE)))

$(BUILD_EXE_FOLDER)/$(SRC_FOLDER)/%.o: $(SRC_FOLDER)/%$(EXT_SRC_FILE) $(INC_FOLDER)/$(call header-of,%$(EXT_SRC_FILE))
	@$(call _build-msg,$(notdir $@),$(call _join,$(_comma)$(_space),$(strip $(notdir $< $(wildcard $(word 2,$^))))))
	@$(CXX) -c $(INC_FLAG) $(FLAGS) -o "$@" "$<"


# Just to avoid file without headers
%$(EXT_INC_FILE):
	
