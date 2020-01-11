
BIN  := nush
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS := -g
LDLIBS :=

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

%.o : %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c -o $@ $<
