CC=gcc
AR=ar
CFLAGS=-c -Wall -D__COMMON_TOOLX_DEBUG__ -I../ -g
LDFLAGS=-L../
LIBS=-lcommontoolx -lrt -lpthread
SOURCES=test.c msgqx_sender.c msgqx_receiver.c sllst_tester.c
INCLUDES=../common_toolx.h ../messageQx.h ../simple_hashx.h msgqqx_test.h
OBJECTS=$(SOURCES:.c=.o)
TEST1=test
TEST2=msgqx_sender
TEST3=msgqx_receiver
TEST4=sllst

all: $(TEST1) $(TEST2) $(TEST3) $(TEST4)

$(TEST1): test.o
	$(CC) $(LDFLAGS) test.o -o $@ $(LIBS)

$(TEST2): msgqx_sender.o
	$(CC) $(LDFLAGS) msgqx_sender.o -o $@ $(LIBS)

$(TEST3): msgqx_receiver.o
	$(CC) $(LDFLAGS) msgqx_receiver.o -o $@ $(LIBS)

$(TEST4): sllst_tester.o
	$(CC) $(LDFLAGS) sllst_tester.o -o $@ $(LIBS)

%.o: %.c ${INCLUDES}
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(TEST1) $(TEST2) $(TEST3) $(TEST4)
