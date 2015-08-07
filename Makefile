OBJECTS := build/main.o build/go.o build/record.o build/gtree.o build/sgf.o build/mcts.o
OBJECTS += build/features/octant.o build/features/neighbor.o
OBJECTS += build/cmd/cat.o build/cmd/import_games.o build/cmd/kerplunk.o build/cmd/lsqlite3.o

CFLAGS := -std=c99 -pedantic
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -O3 -fomit-frame-pointer
CFLAGS += -rdynamic
CFLAGS += -I/usr/include/luajit-2.0/

LIBS := -lm -lluajit-5.1 -lsodium -lsqlite3

.PHONY: clean

all: kerplunk

kerplunk: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OBJECTS): | build build/features build/cmd

build:
	mkdir -p build

build/features:
	mkdir -p build/features

build/cmd:
	mkdir -p build/cmd

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

build/cmd/cat.o: src/cmd/cat.lua
	luajit -b $< $@

build/cmd/import_games.o: src/cmd/import_games.lua
	luajit -b $< $@

build/cmd/kerplunk.o: src/cmd/kerplunk.lua
	luajit -b $< $@

build/cmd/lsqlite3.o: src/cmd/lsqlite3.lua
	luajit -b $< $@

build/main.o: src/main.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJECTS)
	rmdir build/features
	rmdir build/cmd
	rmdir build
