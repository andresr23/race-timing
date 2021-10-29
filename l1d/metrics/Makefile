#---- Final Binary
BINARY  := metrics.out

#---- Include
INCLUDE	:= -I../../

#---- Libraries
LDFLAGS	:= -L../../libcache/ -lcache -pthread

#---- Flags
CFLAGS	:= $(INCLUDE)

#---- Compiler
CC      := gcc

#-------------------------------------------------------------------------------

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

LIBDIRS = ../../libcache/

BUILDDIRS = $(LIBDIRS:%=build-%)
CLEANDIRS = $(LIBDIRS:%=clean-%)

.PHONY: all clean $(BUILDDIRS) $(CLEANDIRS)

.SILENT:
all: $(BINARY)

$(BINARY): $(BUILDDIRS) $(OBJECTS)
	echo "============================== [${@}]"
	echo "  LD" $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(BINARY)

%.o: %.c
	echo "  CC" $<
	$(CC) $^ $(CFLAGS) $(MICROFLAG) $(SPECFLAG) -c

clean: $(CLEANDIRS)
	@rm -f *.o
	@rm -f *.out

$(BUILDDIRS):
	echo "============================== [${@:build-%=%}]"
	$(MAKE) -C $(@:build-%=%) MICROFLAG=$(MICROFLAG) SPECFLAG=$(SPECFLAG)

$(CLEANDIRS):
	echo "============================== [${@:build-%=%}]"
	$(MAKE) clean -C $(@:clean-%=%)

#---- Microarchitecture Specific, default is Skylake.
.PHONY: skylake zen zen-spec

zen: MICROFLAG = -DZEN
zen: all

zen-spec: SPECFLAG = -DLFENCE_ALLOWS_SPECULATION
zen-spec: zen

skylake: all
