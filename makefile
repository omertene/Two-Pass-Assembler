CC = gcc
CFLAGS = -Wall -ansi -pedantic -Iinclude
TARGET = assembler

# Path to source files
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule for compiling .c files into .o files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o tests/*.am tests/*.ob tests/*.ent tests/*.ext