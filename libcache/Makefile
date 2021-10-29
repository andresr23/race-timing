#---- Library Output
LIB_OUTPUT := libcache

#---- Include
INCLUDE    :=

#---- Archiver
AR         := ar
AROPTIONS  := -rcs

#---- Compiler
CC         := gcc
CFLAGS     := $(INCLUDE)

#-------------------------------------------------------------------------------

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=%.o)

.PHONY: clean

.SILENT:
$(LIB_OUTPUT).a: $(OBJECTS)
	echo "  AR" $^
	$(AR) $(AROPTIONS) $@ $^

%.o: %.c
	echo "  CC" $<
	$(CC) $(CFLAGS) $(MICROFLAG) $(SPECFLAG) -c $^

clean:
	@rm -f *.o
	@rm -f *.a
	@rm -f *.h.gch
