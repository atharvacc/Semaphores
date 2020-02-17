# Target library
lib := libuthread.a

objs := queue.o thread.o sem.o
objs1:= sem.o

CC := gcc
CFLAGS := -Wall -Wextra

all: $(lib)


libuthread.a: $(objs)
	ar rcs libuthread.a $(objs)

sem.o: sem.c sem.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(objs1)