CC = gcc

WORK_ROOT = $(PWD)

SVR_SRC_FILE += $(PWD)/tcpserver.c
CLT_SRC_FILE += $(PWD)/tcpclient.c

CFLAGS 	+= 	-I $(WORK_ROOT)/../include/ \
			-I $(WORK_ROOT)/
CFLAGS 	+= 	-Wall -Wextra -g -pthread

LDFLAGS =

TARGET = tcpclient tcpserver
all: $(TARGET)

tcpclient:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SVR_SRC_FILE) -o $@

tcpserver:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SVR_SRC_FILE) -o $@

clean:
	rm -f *.o $(TARGET)
