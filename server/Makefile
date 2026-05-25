CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror
.PHONY: all clean default

default: all
all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c

clean:
	rm -f aesdsocket *.o
