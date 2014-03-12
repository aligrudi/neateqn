CC = cc
CFLAGS = -Wall -O2
LDFLAGS =

all: eqn
%.o: %.c eqn.h
	$(CC) -c $(CFLAGS) $<
eqn: eqn.o tok.o in.o def.o box.o reg.o sbuf.o
	$(CC) -o $@ $^ $(LDFLAGS)
clean:
	rm -f *.o eqn
