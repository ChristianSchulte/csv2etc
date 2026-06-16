#
# Copyright (c) 2026 Christian Schulte <cs@schulte.it>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

.SUFFIXES: .c .o

CC=cc

# clang-format - A tool to format code
#   https://clang.llvm.org/docs/ClangFormat.html
FORMAT=clang-format

INCLUDES=-I/usr/include

DEBUG=
DEBUG+=-g
#DEBUG+=-O0
#DEBUG+=-DDEBUG

CONFIG=

PROFILE=
#PROFILE+=-pg

LTO=
LTO+=-flto=auto

STANDARD=-std=c11

WARNINGS=-Wall
WARNINGS+=-Werror
WARNINGS+=-Wpedantic
WARNINGS+=-Wstrict-prototypes
WARNINGS+=-Wmissing-prototypes
WARNINGS+=-Wmissing-declarations
WARNINGS+=-Wpointer-arith
WARNINGS+=-Wsign-compare
WARNINGS+=-Wformat-signedness
WARNINGS+=-Wformat-truncation
WARNINGS+=-Wuninitialized
WARNINGS+=-Wshadow
#WARNINGS+=-Wcast-qual

CFLAGS=$(INCLUDES) $(DEBUG) $(PROFILE) $(LTO) $(CONFIG) $(WARNINGS)
CFLAGS+=-pedantic
CFLAGS+=-O3
#CFLAGS+=-fsanitize=address
#CFLAGS+=-fno-omit-frame-pointer

LDFLAGS=$(DEBUG) $(PROFILE) $(LTO)
#LDFLAGS+=-fsanitize=address
#LDFLAGS+=-fno-omit-frame-pointer

HEADERS=csv2etc.h
HEADERS+=dbip.h
HEADERS+=heap.h
HEADERS+=host.h
HEADERS+=map.h
HEADERS+=proc.h
HEADERS+=string.h

OBJS=csv2etc.o
OBJS+=cmd-dbip.o
OBJS+=heap.o
OBJS+=map.o
OBJS+=proc.o
OBJS+=string.o

FORMATSRC=csv2etc.c
FORMATSRC+=cmd-dbip.c
FORMATSRC+=heap.c
FORMATSRC+=map.c
FORMATSRC+=proc.c
FORMATSRC+=string.c

CLEAN=$(OBJS)

all: csv2etc 

clean:
	rm -f $(CLEAN)

format:
	$(FORMAT) -i $(FORMATSRC) $(HEADERS)

csv2etc: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(STANDARD) $(CFLAGS) -c -o $@ $<

$(OBJS): $(HEADERS)
