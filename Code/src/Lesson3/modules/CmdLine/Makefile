CC 		:= 	gcc
CFLAGS 	:= 	-Wall -fPIC
TARGET	:= myCmdLine

SRC_FILES := myCmdLine.c

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
