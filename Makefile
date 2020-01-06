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

SOURCES     := $(wildcard $(SDIR)/*.c)
OBJECTS     := $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SOURCES))

all: skrumd skrumd_client

skrumd: skrumd.c $(OBJECTS) 
> $(CC) $(CFLAGS) -o $@ $^ 

skrumd_client: skrumd_client.c $(OBJECTS) 
> $(CC) $(CFLAGS) -o $@ $^ 

$(OBJECTS): $(ODIR)/%.o: $(SDIR)/%.c 
> $(CC) $(CFLAGS) -c -o $@ $< 

.PHONY=clean

clean:
> rm $(ODIR)/*.o
