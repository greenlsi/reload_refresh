LIB_PATH = $(shell pwd)
CC=gcc
CFLAGS=-Werror

all: libT.so example_code sender

example_code: LDLIBS= -lm -L$(LIB_PATH) -lT
example_code: common.o cache_utils.o example_code.o

cache_utils.o: common.o

libT.so: T.o
	gcc -shared -o $(LIB_PATH)/$@ $^

T.o: T.c
	gcc -c -Wall -Werror -fpic $^

common.o: LDLIBS= -lm

sender.o: sender.c
	gcc -c -o $@ $^

sender: sender.o libT.so
	gcc -o $@ sender.o -L$(LIB_PATH) -lT

clean:
	$(RM) *.o *~ *.so
	$(RM) 
