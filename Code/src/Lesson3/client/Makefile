CC = gcc

WORK_ROOT = $(PWD)
MODULE_ROOT = $(PWD)/../modules
include Makefile.feat
include Makefile.module

SRC_FILE += $(WORK_ROOT)/tcpclient.c \
			$(WORK_ROOT)/../modules/myModuleCommon.c

OBJ_FILE := $(SRC_FILE:.c=.o) 

CFLAGS 	+= 	-I$(WORK_ROOT)/../include/ \
			-I$(WORK_ROOT)/../../third_party/libevent/include/ 

CFLAGS 	+= 	-Wall -Wextra -g -DBUILD_RULE_CLIENT
CFLAGS  +=  $(FEATURE_CFLAGS)

LDFLAGS +=	$(FEATURE_LDFLAGS)
LDFLAGS +=  -L$(WORK_ROOT)/../../third_party/libevent/lib/ -levent
LDFLAGS +=  -pthread

TARGET = tcpclient
all: $(TARGET)

tcpclient: $(OBJ_FILE)
	$(CC) $(OBJ_FILE) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILE) $(TARGET)

.PHONY: all
