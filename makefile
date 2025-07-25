
# Compiler & flags
CC = gcc
CFLAGS = -Wall -Werror -O2 -g

# Directories
SRCDIR = src
OBJDIR = build

# Source files
SOURCES = main.c lz77.c tree.c bitio.c aes.c

# Object files
OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(notdir $(SOURCES)))

# Output binary
TARGET = lz77

# Default rule
all: $(TARGET)

# Linking rule
$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) -lm

# Generic rule for building object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(TARGET)

