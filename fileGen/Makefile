PROG = fileGen
CC = g++
CPPFLAGS = -I /usr/include -L /usr/lib
OBJS = main.o
LDFLAGS = -lmysqlcppconn
all: build run
run:
	./$(PROG) 

build: $(PROG)

$(PROG): $(OBJS)
	g++ -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f *stackdump $(PROG) *.o .depend

