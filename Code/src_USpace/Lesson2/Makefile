CC = gcc

WORK_ROOT = $(PWD)
include Makefile.feat
include Makefile.module

SVR_SRC_FILE += $(PWD)/tcpserver.c 
SVR_SRC_FILE +=	$(COMMON_SRC_FILE)

CLT_SRC_FILE += $(PWD)/tcpclient.c 
CLT_SRC_FILE += $(COMMON_SRC_FILE)

CFLAGS 	+= 	-I $(WORK_ROOT)/../include/ \
			-I $(WORK_ROOT)/
CFLAGS  +=	$(COMMON_CFLAGS)
CFLAGS 	+= 	-Wall -Wextra -g -pthread

LDFLAGS =

TARGET = tcpclient tcpserver
all: $(TARGET)

tcpclient:
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLT_SRC_FILE) -o $@

tcpserver:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SVR_SRC_FILE) -o $@

clean:
	rm -f *.o $(TARGET)
.PHONY: all
