PROG = fill 

CC = g++
CXXFLAGS = -Wall -Wextra -std=c++17
CPPFLAGS = #-I /usr/include -L /usr/lib
SRC = $(wildcard *.cpp) \
      $(wildcard ../lib/*.cpp)
OBJ = $(SRC:.cpp=.o)
LDFLAGS = -lmysqlcppconn

all: build run
run:
	./$(PROG) 

build: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $(OBJ) $(CPPFLAGS) $(LDFLAGS)

clean:
	rm -f *stackdump $(PROG) $(OBJ)
