# Kernel build rules
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

# Push Directory
sp 				:= $(sp).x
dirstack_$(sp)	:= $(d)
d				:= $(dir)

# Local Rules
#  - Directories which are searched for source files (*.c and *.s)
DIRS_$(d) := $(d)/src $(d)/src/mm $(d)/src/process $(d)/src/io

SOURCES_$(d) := $(foreach DIR, $(DIRS_$(d)), $(wildcard $(DIR)/*.c)) \
				$(foreach DIR, $(DIRS_$(d)), $(wildcard $(DIR)/*.s))

#  - Primary target
TGTS_$(d) := bin/chaff.elf

#  - Extra build options
$(TGTS_$(d)):	CF_TGT := -I$(d)/include -fno-builtin -ffreestanding -fno-stack-protector
$(TGTS_$(d)):	LF_TGT := -T $(d)/linker.ld

#  - Common rules
OBJS_$(d) 	:= $(addprefix obj/,$(addsuffix .o,$(basename $(SOURCES_$(d)))))
DEPS_$(d) 	:= $(OBJS_$(d):.o=.d)

TGT_BIN		:= $(TGT_BIN) $(TGTS_$(d))

$(TGTS_$(d)):	$(OBJS_$(d)) $(d)/linker.ld
				$(LINK)

# Pop Directory
-include	$(DEPS_$(d))
d			:= $(dirstack_$(sp))
sp			:= $(basename $(sp))
