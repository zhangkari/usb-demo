
PROJ := usb-demo
CC := gcc
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))
CFLAGS += -lusb-1.0

$(PROJ) : $(OBJS)
	$(CC) $< $(CFLAGS) -o $@

.PHONY:clean
clean:
	@rm -rf $(PROJ) $(OBJS)
