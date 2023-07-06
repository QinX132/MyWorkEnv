CC 		:= 	gcc
CFLAGS 	:= 	-Wall -fPIC
TARGET	:= myMsg

SRC_FILES := tcpMsg.c

CFLAGS	+= 	-I../../include/

OBJECTS := $(SRC_FILES:.c=.o)

all: lib$(TARGET).a

lib$(TARGET).a: $(OBJECTS)
	 ar rcs $@ $(OBJECTS)

so: $(OBJECTS)
	$(CC) -shared -o lib$(TARGET).so $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) lib$(TARGET).so lib$(TARGET).a
