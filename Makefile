SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c 
.ONESHELL:
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables

ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This Make does not support .RECIPEPREFIX. Please use GNU Make 4.0 or later)
endif
.RECIPEPREFIX = >

CC=gcc
CPPFLAGS=
TARGET_ARCH=
CFLAGS=-Wall -g
DEPS = macros.h
ODIR = obj
SDIR = common
#OBJ = skrum_errno.o skrum_protocol_api.o skrum_protocol_defs.o skrum_protocol_pack.o skrum_protocol_socket.o pack.o logs.o

SOURCES     := $(shell find ./$(SDIR) -type f -name "*.c")
OBJECTS     := $(SOURCES:.c=.o)

all: skrumd

skrumd: skrumd.c $(OBJECTS) 
> $(CC) -o $@ $^ $(CFLAGS)

obj/%.o: common/%.c $(DEPS)
> $(CC) -c -o $@ $< $(CFLAGS)

.PHONY=clean

clean:
> rm common/*.o
