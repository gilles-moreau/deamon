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
INCLUDE=-I.
DEPS = macros.h
BUILDDIR := build
SDIR := src

# common variables
SCOMMONDIR := $(SDIR)/common
OCOMMONDIR := $(BUILDDIR)/common
SCOMMONFILES     := $(wildcard $(SCOMMONDIR)/*.c)
OCOMMONFILES     := $(patsubst $(SCOMMONDIR)/%.c,$(OCOMMONDIR)/%.o,$(SCOMMONFILES))

# skrumd variables
SSKRUMDDIR := $(SDIR)/skrumd
OSKRUMDDIR := $(BUILDDIR)/skrumd
SSKRUMDFILES     := $(SSKRUMDDIR)/skrumd_req.c
OSKRUMDFILES     := $(patsubst $(SSKRUMDDIR)/%.c,$(OSKRUMDDIR)/%.o,$(SSKRUMDFILES))

# skrumctld variables
SSKRUMCTLDDIR := $(SDIR)/skrumctld
OSKRUMCTLDDIR := $(BUILDDIR)/skrumctld
SSKRUMCTLDFILES     := $(SSKRUMCTLDDIR)/
OSKRUMCTLDFILES     := $(patsubst $(SSKRUMCTLDDIR)/%.c,$(OSKRUMCTLDDIR)/%.o,$(SSKRUMCTLDFILES))

# flags
CFLAGS      += $(shell pkg-config --cflags hwloc)
LDLIBS      += $(shell pkg-config --libs hwloc)

all: skrumd skrumctld

# build skrumd
skrumd: $(SSKRUMDDIR)/skrumd.c $(OSKRUMDFILES) $(OCOMMONFILES) 
> $(CC) $(CFLAGS) $(LDLIBS) $(INCLUDE) -o $@ $^ 

$(OSKRUMDFILES): $(SSKRUMDFILES) | $(OSKRUMDDIR)
> $(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $^

$(OSKRUMDDIR):
> mkdir -p $@

# build skrumctld
skrumctld: $(SSKRUMCTLDDIR)/skrumctld.c $(OCOMMONFILES) 
> $(CC) $(CFLAGS) $(INCLUDE) $(LDLIBS) -o $@ $^ 


# build common objects
$(OCOMMONFILES): $(OCOMMONDIR)/%.o: $(SCOMMONDIR)/%.c | $(OCOMMONDIR)
> $(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $< 

$(OCOMMONDIR):
> mkdir -p $@

.PHONY=clean

clean:
> rm -rf $(BUILDDIR)/
