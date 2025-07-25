CC = gcc
CFLAGS = -Wall -Werror -O2
OBJDIR = build
SRCDIR = src

OBJS = $(OBJDIR)/main.o $(OBJDIR)/lz77.o $(OBJDIR)/tree.o $(OBJDIR)/bitio.o

all: lz77

lz77: $(OBJS)
	$(CC) -o $@ $(OBJS) -lm

$(OBJDIR)/main.o: $(SRCDIR)/main.c $(SRCDIR)/bitio.h $(SRCDIR)/lz77.h
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lz77.o: $(SRCDIR)/lz77.c $(SRCDIR)/bitio.h $(SRCDIR)/tree.h
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/tree.o: $(SRCDIR)/tree.c $(SRCDIR)/tree.h
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/bitio.o: $(SRCDIR)/bitio.c $(SRCDIR)/bitio.h
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(OBJDIR) lz77
