CC = gcc
CFLAGS = -pthread
ENDING_CFLAGS = -lrt -lm
BINDIR = ../bin/
ODIR = ../obj/

all: task

task: task.o rt-lib.o
	$(CC) $(CFLAGS) -o $(BINDIR)task $(ODIR)task.o $(ODIR)rt-lib.o $(ENDING_CFLAGS)

task.o: task.c lib/rt-lib.h
	$(CC) -c task.c -o $(ODIR)task.o

rt-lib.o: rt-lib.c lib/rt-lib.h
	$(CC) -c rt-lib.c -o $(ODIR)rt-lib.o

clean:
	rm $(ODIR)*.o $(BINDIR)task $(BINDIR)*.txt
