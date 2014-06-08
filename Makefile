CC=gcc
AR=ar
CFLAGS=-c -Wall -D__COMMON_TOOLX_DEBUG__ -g
LDFLAGS= 
LIBS=
SOURCES=common_toolx.c simple_hashx.c messageQx.c
INCLUDES=common_toolx.h simple_hashx.h messageQx.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=test
SLIB=libcommontoolx.a

lib:$(SLIB)

$(SLIB): $(OBJECTS)
	$(AR) rcs $(SLIB) $(OBJECTS)

%.o: %.c ${INCLUDES}
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE) $(SLIB)
