BIN=main
CC=gcc
CFLAGS=-I./ -I./lvgl -O3 -g3
LVGL_DIR=lvgl
LIBS+=-lstdc++ -lm

include lvgl/lvgl.mk
include lvgl/lv_drivers/lv_drivers.mk

OBJDIR=./obj
CSRCS+=$(wildcard *.c)

COBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(CSRCS))

SRCS=$(CSRCS)
OBJS=$(COBJS)

all: default

$(OBJDIR)/%.o: %.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

default: $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LDFLAGS) $(LIBS)

nothing:
	$(info OBJS ="$(OBJS)")
	$(info SRCS ="$(SRCS)")
	$(info DONE)

clean:
	rm -f $(OBJS) $(BIN)
