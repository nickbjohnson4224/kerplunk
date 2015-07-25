BUILDDIR := build
SRCDIR := src

SOURCES := $(SRCDIR)/main.c $(SRCDIR)/go.c $(SRCDIR)/sgf.c $(SRCDIR)/mcts.c
HEADERS := $(SRCDIR)/go.h $(SRCDIR)/sgf.h $(SRCDIR)/mcts.h
OBJECTS := $(BUILDDIR)/main.o $(BUILDDIR)/go.o $(BUILDDIR)/sgf.o $(BUILDDIR)/mcts.o

CFLAGS := -std=c99 -pedantic
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Werror
CFLAGS += -O3 -fomit-frame-pointer -DNDEBUG
CFLAGS += -fopenmp

LIBS := -lm -lsodium

.PHONY: clean

all: kerplunk

kerplunk: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OBJECTS): | $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/go.o: src/go.c src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/mcts.o: src/mcts.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/sgf.o: src/sgf.c src/sgf.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/main.o: src/main.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJECTS)
	rmdir $(BUILDDIR)
