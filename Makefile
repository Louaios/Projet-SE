CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
LDFLAGS = -lrt -lpthread

TARGETS = mysh myls myps

MYSH_OBJS = mysh.o parser.o executor.o builtins.o wildcards.o redirections.o jobs.o variables.o signals.o utils.o
MYLS_OBJS = myls.o
MYPS_OBJS = myps.o

all: $(TARGETS)

mysh: $(MYSH_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

myls: $(MYLS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

myps: $(MYPS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
