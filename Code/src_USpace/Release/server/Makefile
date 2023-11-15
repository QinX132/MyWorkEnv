CC = gcc

WORK_ROOT		= .
UTILS_ROOT		= $(WORK_ROOT)/../myUtils
THIRT_PART_ROOT	= $(WORK_ROOT)/../../third_party

SRC_FILE	+=	$(WORK_ROOT)/QinXServerMain.c  \
				$(WORK_ROOT)/ServerProc.c

OBJ_FILE	:=	$(SRC_FILE:.c=.o)

CFLAGS	+=	-I$(WORK_ROOT)/../include/ \
			-I$(WORK_ROOT)/include/ \
			-I$(THIRT_PART_ROOT)/libevent/include/
 
CFLAGS	+= 	-Wall -Wextra -g

LDFLAGS +=	-L$(UTILS_ROOT) -lmyUtils
LDFLAGS +=	-L$(THIRT_PART_ROOT)/libevent/lib/ -levent
LDFLAGS +=  -pthread

TARGET	=	QinXServer
all: $(TARGET)

$(TARGET): $(OBJ_FILE)
	$(CC) $(OBJ_FILE) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILE) $(TARGET)

.PHONY: all
