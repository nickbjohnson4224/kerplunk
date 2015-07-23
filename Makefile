SOURCES := src/main.c src/go.c src/mcts.c
HEADERS := src/go.h src/mcts.h
OBJECTS := build/main.o build/go.o build/mcts.o

CFLAGS := -std=c99 -pedantic
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Werror
CFLAGS += -O3 -fomit-frame-pointer -DNDEBUG
CFLAGS += -fopenmp

LIBS := -lsodium

.PHONY: clean

all: kerplunk

kerplunk: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

build/go.o: src/go.c src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/mcts.o: src/mcts.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/main.o: src/main.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJECTS)
