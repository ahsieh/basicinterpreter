#####################################
# Makefile - BASIC interpreter
#####################################

## Project Name
PROJECT = basicinterp

## Compiler Definitions
CC = gcc
CFLAGS=-std=c99 -pedantic -Wall

## Root Directories
BUILD_DIR = ./bin
OBJECT_DIR = ./obj
PROJ_SRC_DIR = ./src

## Where to find source files
VPATH = $(PROJ_SRC_DIR)

## Where to find header files


#************************************
## Source files
SRCS = basic.c

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
	${CC} ${CFLAGS} $^ -o $@

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
	@echo VPATH = ${VPATH}
	@echo


