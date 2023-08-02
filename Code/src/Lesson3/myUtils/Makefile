CC 		:= 	gcc
TARGET	:= myUtils

UTILS_ROOT = .
THIRD_PARTY_ROOT = $(UTILS_ROOT)/../../third_party

SRC_FILES +=	$(UTILS_ROOT)/myCommonUtil.c $(UTILS_ROOT)/myModuleCommon.c \
				$(UTILS_ROOT)/CmdLine/myCmdLine.c \
				$(UTILS_ROOT)/Health/myModuleHealth.c \
				$(UTILS_ROOT)/Log/myLogIO.c \
				$(UTILS_ROOT)/Mem/myMem.c \
				$(UTILS_ROOT)/Msg/myMsg.c \
				$(UTILS_ROOT)/TPool/myThreadPool.c

CFLAGS	+= 	-I$(UTILS_ROOT)/../include/ \
            -I$(THIRD_PARTY_ROOT)/libevent/include/

CFLAGS	+= -Wall -fPIC -Wextra -g

OBJECTS := $(SRC_FILES:.c=.o)

all: lib$(TARGET).a

lib$(TARGET).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	make test

so: $(OBJECTS)
	$(CC) -shared -o lib$(TARGET).so $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) lib$(TARGET).so lib$(TARGET).a 
	pushd UnitTest && make clean && popd

test:
	pushd UnitTest && make -B && ./UnitTest && popd
