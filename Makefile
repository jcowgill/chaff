# Chaff basic makefile

# All source files are relative to src/
#  Supported types:
#	.c - C Source, compiled with GCC
#	.s - Assembly Source, compiled with NASM

SOURCES = $(wildcard src/*.c)
SOURCES += $(wildcard src/*.s)

BINFILE = chaff.elf

#Set compiler stuff
LINK = ld
CC = gcc
ASM = nasm

#	For some gay reason, using stabs seems to crash gdb at random moments
#	(at least on my compiler)
CFLAGS = -c -std=gnu99 -fgnu89-inline -gdwarf-2 -Wall -Wextra -Werror -Isrc/include -DDEBUG -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector
ASMFLAGS = -Xgnu -f elf -F dwarf

#Calculate objects (changes to .obj and uses obj directory)
OBJS = $(subst src/,obj/,$(addsuffix .obj,$(basename $(SOURCES))))

#Set source file path
VPATH = src

#AUTO-DEPENDANCIES
-include $(OBJS:.obj=.dep)

#TARGETS

all: $(BINFILE)

mkdirs:
	cmd /c mkdir bin
	cmd /c mkdir obj

install:
	cmd /c copy bin\\$(BINFILE) O:\\

clean:
	cmd /c del /q obj
	cmd /c del bin\\$(BINFILE)

$(BINFILE): $(OBJS)
	$(LINK) -o bin/$@ -T src/linker.ld $(OBJS)

obj/%.obj : %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	cmd /c $(CC) -MM $(CFLAGS) $< \> obj/$*.d

obj/%.obj : %.s
	$(ASM) $(ASMFLAGS) -o $@ $<
