CC = gcc
CFLAGS = -g -Wall -Wextra -D_REENTRANT -DCOLOR \
				 -D__BSD_VISIBLE -DREADLINE -Isupport -I.
LDFLAGS = -lpthread -lreadline

SRCS = node.c dbg.c lnxparse.c

all: node 

node : $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f node 
