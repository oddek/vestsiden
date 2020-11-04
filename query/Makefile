PROG = query
CC = g++
CPPFLAGS = -I /usr/include -L /usr/lib
OBJS = query.o
LDFLAGS = -lmysqlcppconn
all: build run
run:
	./$(PROG) 

build: $(PROG)

$(PROG): $(OBJS)
	g++ -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f *stackdump $(PROG) *.o .depend

