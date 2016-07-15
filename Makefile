CC=gcc
AR=ar
MAKE=make
CFLAGS=-c -Wall
LDFLAGS= 
LIBS=
SOURCES=common_toolx.c simple_hashx.c messageQx.c static_linked_listx.c
INCLUDES=common_toolx.h simple_hashx.h messageQx.h static_linked_listx.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=test
SLIB=libcommontoolx.a

default: debug

lib:$(SLIB)

debug: CFLAGS+=-D__COMMON_TOOLX_DEBUG__ -g
debug: lib

release: CFLAGS+=-O3
release: lib

$(SLIB): $(OBJECTS)
	$(AR) rcs $(SLIB) $(OBJECTS)

%.o: %.c ${INCLUDES}
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE) $(SLIB)

.PHONY: lib clean debug release default
