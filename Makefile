# Top level Makefile rules
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

# Targets:
#  all 		= Build everything (using clang)
#  all-clang= Build everything with clang
#  all-gcc  = Build everything with gcc
#  clean 	= Clean everything
#  
#  kernel	= Build kernel
#

# Compilers
LD			= ld
CC			= clang
ASM			= nasm

# Global flags (apply to kernel and user mode)
ifeq ($(CC),clang)
CF_ALL		= -ccc-host-triple i386-pc-unknown-gnu -g -m32 -Wall -Wextra -DDEBUG -std=gnu99
else
CF_ALL		= -gdwarf-2 -m32 -Wall -Wextra -DDEBUG -std=gnu99
endif

LF_ALL		= -x -m elf_i386
AF_ALL		= -Xgnu -f elf32 -F dwarf

# Compiling commands
ASMCOMP		= $(ASM) $(AF_ALL) $(AF_TGT) -o $@ $<
COMP		= $(CC) $(CF_ALL) $(CF_TGT) -MD -o $@ -c $<
LINK		= $(LD) $(LF_ALL) $(LF_TGT) -o $@ $^

# Detect if using windows
ifneq (,$(findstring NT,$(shell uname)))
WINDOWS := 1
CYGWIN := nodosfilewarning
export CYGWIN
else
WINDOWS := 0
endif

# Main target
.PHONY:	all all-gcc all-clang
all:	all-clang

all-clang: CC=clang
all-clang: targets
all-gcc: CC=gcc
all-gcc: targets

# Things to build
dir		:= kernel
include $(dir)/Rules.mk

# General rules
obj/%.o : %.c
	@mkdir -p $(dir $@)
	$(COMP)
ifeq ($(WINDOWS),1)
	sed -i 's/:\//\\:\//g' $(addsuffix .d,$(basename $@))
endif

obj/%.o : %.s
	@mkdir -p $(dir $@)
	$(ASMCOMP)

bin/% : %.o
	@mkdir -p $(dir $@)
	$(LINK)
	
# Top-level make rules
#  Add top-level tagrets to TGT_BIN
.PHONY: targets
targets:	$(TGT_BIN)

.PHONY:	clean
clean:
		rm -f $(TGT_BIN)
		rm -rf obj/
