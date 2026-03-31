CC = gcc
CFLAGS = -Wall -ansi -pedantic 
TARGET = assembler
SRCS = assembler.c pre_processor.c first_pass.c second_pass.c symbol_table.c output_generator.c utils.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) *.am *.ob *.ent *.ext