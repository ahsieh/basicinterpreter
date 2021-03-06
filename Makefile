#####################################
# Makefile - BASIC interpreter
#####################################

## Project Name
PROJECT = basicinterp

## Compiler Definitions
CC = gcc
CFLAGS=-std=c99 -Wall -O

## Root Directories
ROOT_DIR = .
BUILD_DIR = ./bin
OBJECT_DIR = ./obj
PROJ_SRC_DIR = ./src
PROJ_INC_DIR = ./inc

## Where to find source files
VPATH = $(ROOT_DIR)/$(PROJ_SRC_DIR)

## Where to find header files
IPATH = $(ROOT_DIR)/$(PROJ_INC_DIR)
INCPATH = $(patsubst %,-I%, $(IPATH))

#************************************
## Source files
SRCS  = main.c
SRCS += basic.c

## Dependencies
DEPS = basic.h

## Object files
OBJS = $(patsubst %.c,%.o,$(SRCS))

#************************************
## Build Messages
BEGIN_MSG = "Beginning build..."
END_MSG = "Build complete!"

#************************************
## Make rules
PROJECT_OUT = ${BUILD_DIR}/${PROJECT}
all: mkbuilddir begin ${PROJECT_OUT} end

## Rule to clean the build projects
clean:
	@echo "rm -rf ${BUILD_DIR} ${wildcard *~}"
	@rm -rf ${BUILD_DIR} ${wildcard *~}

## Rule to create the target directory
.PHONY: mkbuilddir
mkbuilddir:
	@mkdir -p ${BUILD_DIR}

## Rule to generate executable code
${PROJECT_OUT}: ${SRCS}
	@echo "out $^ $@"
	${CC} ${CFLAGS} ${INCPATH} $^ -o $@

## Eye candy rules
begin:
	@echo ${BEGIN_MSG}
end:
	@echo ${END_MSG}

## Debug
debug:
	@echo BUILD_DIR = ${BUILD_DIR}
	@echo
	@echo SRCS = ${SRCS}
	@echo
	@echo OBJS = ${OBJS}
	@echo
	@echo DEPS = ${DEPS}
	@echo
	@echo VPATH = ${VPATH}
	@echo
	@echo INCPATH = ${INCPATH}
	@echo


