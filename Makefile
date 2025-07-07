CC      = gcc
CFLAGS  = -Wall -pthread -std=c99 \
          -D_POSIX_C_SOURCE=200809L \
          -D_XOPEN_SOURCE=700 \
          -D_DEFAULT_SOURCE

TARGET  = trabalho

SRCS    = main.c produtor.c consumidor.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c buffer.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
