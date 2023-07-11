CC = gcc

WORK_ROOT = $(PWD)
MODULE_ROOT = ../modules
include Makefile.feat
include Makefile.module

SRC_FILE += $(WORK_ROOT)/tcpserver.c 
OBJ_FILE := $(SRC_FILE:.c=.o)

CFLAGS 	+= 	-I$(WORK_ROOT)/../include/ \
			-I$(WORK_ROOT)/../../third_party/libevent/include/
 
CFLAGS 	+= 	-Wall -Wextra -g
CFLAGS  +=  $(FEATURE_CFLAGS)

LDFLAGS +=	$(FEATURE_LDFLAGS)
LDFLAGS +=  -L$(WORK_ROOT)/../../third_party/libevent/lib/ -levent
LDFLAGS +=  -pthread

TARGET = tcpserver
all: $(TARGET)

tcpserver: $(OBJ_FILE)
	$(CC) $(OBJ_FILE) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILE) $(TARGET)

.PHONY: all
