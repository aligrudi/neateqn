CC = cc
CFLAGS = -Wall -O2
LDFLAGS =
OBJS = eqn.o tok.o src.o def.o box.o reg.o sbuf.o

all: eqn
%.o: %.c eqn.h
	$(CC) -c $(CFLAGS) $<
eqn: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
clean:
	rm -f *.o eqn
