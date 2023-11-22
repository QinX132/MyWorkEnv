CC ?= gcc
CXX ?= g++

UNIT_TEST_ROOT = .
UTILS_ROOT = ..
THIRD_PARTY_ROOT = ../../../third_party

SRC_FILE += UnitTestMain.c \
			UnitTestTPool.c \
			UnitTestMsg.c \
			UnitTestMem.c \
			UnitTestHealth.c \
			UnitTestTimer.c \
			UnitTestCommonUtil.c

OBJ_FILE := $(SRC_FILE:.c=.o)

CFLAGS 	+= 	-I$(UNIT_TEST_ROOT) \
			-I$(UTILS_ROOT)/../include \
			-I$(THIRD_PARTY_ROOT)/libevent/include \
			-I$(THIRD_PARTY_ROOT)/googletest/include
 
CFLAGS 	+= 	-Wall -Wextra -g

LDFLAGS +=	-L$(UTILS_ROOT) -lmyUtils
LDFLAGS +=	-L$(THIRD_PARTY_ROOT)/googletest/lib -lgtest \
			-L$(THIRD_PARTY_ROOT)/libevent/lib -levent \
			-lpthread

TARGET = UnitTest
all: $(TARGET)

$(TARGET): $(OBJ_FILE)
	$(CXX) $(OBJ_FILE) -o $@ $(LDFLAGS)

%.o: %.c
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILE) $(TARGET)

.PHONY: all
