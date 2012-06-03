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
#  all 		= Build everything
#  clean 	= Clean everything
#  
#  kernel	= Build kernel
#

# Detect if using windows
ifneq (,$(findstring NT,$(shell uname)))
WINDOWS := 1
CYGWIN := nodosfilewarning
export CYGWIN
else
WINDOWS := 0
endif

# Compilers
LD			= ld
CC			= clang
ASM			= nasm

# Global flags (apply to kernel and user mode)
ifeq ($(CC),clang)
CF_ALL		= -ccc-host-triple i386-pc-unknown-gnu -g -m32 -Wall -Wextra -DDEBUG -std=gnu99 -fno-common
else
CF_ALL		= -gdwarf-2 -m32 -Wall -Wextra -DDEBUG -std=gnu99 -fno-common
endif

LF_ALL		= -x -m elf_i386
AF_ALL		= -Xgnu -f elf32 -F dwarf

# Compiling commands
ASMCOMP		= $(ASM) $(AF_ALL) $(AF_TGT) -o $@ $<
LINK		= $(LD) $(LF_ALL) $(LF_TGT) -o $@ $(filter-out %.ld, $^)
COMP		= $(CC) $(CF_ALL) $(CF_TGT) -MMD -o $@ -c $<

# Main target
.PHONY:	all
all:	targets

# Things to build
dir		:= kernel
include $(dir)/Rules.mk

# General rules
obj/%.o : %.c
	@mkdir -p $(dir $@)
	$(COMP)
ifeq ($(WINDOWS),1)
# - This changes \ to / unless it is at the end of the line
	@sed -i 's#\\\\\\(.\\)#/\\1#g' $(addsuffix .d,$(basename $@))
endif

obj/%.o : %.s
	@mkdir -p $(dir $@)
	$(ASMCOMP)
	
# Top-level make rules
#  Add top-level tagrets to TGT_BIN
.PHONY: targets
targets:	$(TGT_BIN)

.PHONY:	clean
clean:
		rm -f $(TGT_BIN)
		rm -rf obj/
