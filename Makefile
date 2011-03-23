# Chaff basic makefile

# All source files are relative to src/
#  Supported types:
#	.c - C Source, compiled with GCC
#	.s - Assembly Source, compiled with NASM

VPATH = src
DIRS = src src/memmgr src/process

SOURCES = $(foreach DIR, $(DIRS), $(wildcard $(DIR)/*.c))
SOURCES += $(foreach DIR, $(DIRS), $(wildcard $(DIR)/*.s))

BINFILE = chaff.elf

#Set compiler stuff
LINK = ld
CC = gcc
ASM = nasm

CFLAGS = -c -gdwarf-2 -Wall -Wextra -Isrc/include -DDEBUG -std=gnu99 -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-stack-protector
ASMFLAGS = -Xgnu -f elf -F dwarf
LDFLAGS = -x

#Calculate objects (changes to .obj and uses obj directory)
OBJS = $(subst src/,obj/,$(addsuffix .obj,$(basename $(SOURCES))))

#AUTO-DEPENDANCIES
-include $(OBJS:.obj=.dep)

#TARGETS

all: $(BINFILE)

clean:
	rm -rf obj
	rm bin/$(BINFILE)

$(BINFILE): $(OBJS)
	mkdir -p bin
	$(LINK) $(LDFLAGS) -o bin/$@ -T src/linker.ld $(OBJS)

obj/%.obj : %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) -MM $(CFLAGS) $< > obj/$*.d

obj/%.obj : %.s
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -o $@ $<
