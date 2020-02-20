# Target library
lib := libuthread.a

objs := queue.o thread.o sem.o tps.o
objs1:= sem.o tps.o libuthread.a

CC := gcc
CFLAGS := -Wall -Wextra

all: $(lib)


libuthread.a: $(objs)
	ar rcs libuthread.a $(objs)

sem.o: sem.c sem.h
	$(CC) $(CFLAGS) -c -o $@ $<
tps.o: tps.c tps.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(objs1)
