BUILDDIR := build
SRCDIR := src

SOURCES := $(SRCDIR)/main.c $(SRCDIR)/go.c $(SRCDIR)/record.c $(SRCDIR)/gtree.c $(SRCDIR)/feat.c $(SRCDIR)/sgf.c $(SRCDIR)/mcts.c
HEADERS := $(SRCDIR)/go.h $(SRCDIR)/record.h $(SRCDIR)/gtree.h $(SRCDIR)/sgf.h $(SRCDIR)/feat.h $(SRCDIR)/mcts.h
OBJECTS := $(BUILDDIR)/main.o $(BUILDDIR)/go.o $(BUILDDIR)/record.o $(BUILDDIR)/gtree.o $(BUILDDIR)/feat.o $(BUILDDIR)/sgf.o $(BUILDDIR)/mcts.o

CFLAGS := -std=c11
CFLAGS += -Wall -Wextra -Werror
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

$(BUILDDIR)/record.o: src/record.c src/record.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/gtree.o: src/gtree.c src/gtree.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/feat.o: src/feat.c src/feat.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/sgf.o: src/sgf.c src/sgf.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/mcts.o: src/mcts.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/main.o: src/main.c src/mcts.h src/go.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJECTS)
	rmdir $(BUILDDIR)
