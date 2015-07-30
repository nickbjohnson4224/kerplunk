OBJECTS := build/main.o build/go.o build/record.o build/gtree.o build/sgf.o build/mcts.o
OBJECTS += build/features/octant.o build/features/neighbor.o

CFLAGS := -std=c11
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -O3 -fomit-frame-pointer

LIBS := -lm -lsodium

.PHONY: clean

all: kerplunk

kerplunk: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OBJECTS): | build build/features

build:
	mkdir -p build

build/features:
	mkdir -p build/features

build/go.o: src/go.c src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/record.o: src/record.c src/record.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/gtree.o: src/gtree.c src/gtree.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/sgf.o: src/sgf.c src/sgf.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/mcts.o: src/mcts.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/features/octant.o: src/features/octant.c src/features/octant.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/features/neighbor.o: src/features/neighbor.c src/features/neighbor.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/main.o: src/main.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJECTS)
	rmdir build/features
	rmdir build
