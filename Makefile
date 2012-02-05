# Chaff basic makefile
#
#  Copyright 2012 James Cowgill
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

# All source files are relative to src/
#  Supported types:
#	.c - C Source, compiled with GCC
#	.s - Assembly Source, compiled with NASM

VPATH = src
DIRS = src src/memmgr src/process src/io

SOURCES = $(foreach DIR, $(DIRS), $(wildcard $(DIR)/*.c))
SOURCES += $(foreach DIR, $(DIRS), $(wildcard $(DIR)/*.s))

BINFILE = chaff.elf

#Set compiler stuff
LINK = ld
CC = gcc
ASM = nasm

CFLAGS = -c -gdwarf-2 -m32 -Wall -Wextra -Isrc/include -DDEBUG -std=gnu99 -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-stack-protector -ffreestanding
ASMFLAGS = -Xgnu -f elf32 -F dwarf
LDFLAGS = -x -m elf_i386

#Calculate objects (changes to .obj and uses obj directory)
OBJS = $(subst src/,obj/,$(addsuffix .obj,$(basename $(SOURCES))))

#TARGETS

all: $(BINFILE)

install:
	-mount bin/chaffdir
	cp bin/chaff.elf bin/chaffdir/chaff.elf
	sync

clean:
	rm -rf obj
	rm -f bin/$(BINFILE)

$(BINFILE): $(OBJS)
	mkdir -p bin
	$(LINK) $(LDFLAGS) -o bin/$@ -T src/linker.ld $(OBJS)

obj/%.obj : %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) -MM -MT $@ $(CFLAGS) $< > obj/$*.dep

obj/%.obj : %.s
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -o $@ $<

#AUTO-DEPENDANCIES
-include $(OBJS:.obj=.dep)
