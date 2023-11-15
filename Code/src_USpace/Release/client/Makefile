CC = gcc

WORK_ROOT			=	.
UTILS_ROOT			= 	$(WORK_ROOT)/../myUtils
THIRD_PARTY_ROOT	= 	$(WORK_ROOT)/../../third_party

SRC_FILE	+=	$(WORK_ROOT)/QinXClientMain.c \
				$(WORK_ROOT)/ClientProc.c

OBJ_FILE	:= $(SRC_FILE:.c=.o) 

CFLAGS	+=	-I$(WORK_ROOT)/../include/ \
			-I$(WORK_ROOT)/include/ \
			-I$(THIRD_PARTY_ROOT)/libevent/include/ 

CFLAGS	+=	-Wall -Wextra -g

LDFLAGS +=	-L$(UTILS_ROOT) -lmyUtils
LDFLAGS +=	-L$(THIRD_PARTY_ROOT)/libevent/lib/ -levent
LDFLAGS +=	-pthread

TARGET	=	QinXClient

all: $(TARGET)

$(TARGET): $(OBJ_FILE)
	$(CC) $(OBJ_FILE) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILE) $(TARGET)

.PHONY: all
