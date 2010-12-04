# Chaff basic makefile

# All source files are relative to src/
#  Supported types:
#	.c - C Source, compiled with GCC
#	.s - Assembly Source, compiled with NASM

VPATH = src
DIRS = src src/memmgr

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

install:
	cmd /c copy bin\\$(BINFILE) O:\\

clean:
	-cmd /c rmdir /s /q obj
	-cmd /c del bin\\$(BINFILE)

$(BINFILE): $(OBJS)
	-cmd /c mkdir bin
	$(LINK) $(LDFLAGS) -o bin/$@ -T src/linker.ld $(OBJS)

obj/%.obj : %.c
	-cmd /c mkdir $(subst /,\\,$(dir $@))
	$(CC) $(CFLAGS) -o $@ -c $<
	cmd /c $(CC) -MM $(CFLAGS) $< \> obj/$*.d

obj/%.obj : %.s
	-cmd /c mkdir $(subst /,\\,$(dir $@))
	$(ASM) $(ASMFLAGS) -o $@ $<
