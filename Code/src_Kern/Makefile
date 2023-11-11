KDIR                ?= /lib/modules/$(shell uname -r)/build
ARCH                ?= $(shell uname -m)

TARGET              ?=  myKernMod

WORK_RELA_ROOT		= .
WORK_ABSL_ROOT		= $(PWD)

#get stress src files
SRC_FILES   +=	myKernModMain.c
ccflags-y	+=	-I$(WORK_ABSL_ROOT)/include

OBJ_FILES 		= 	$(addsuffix .o, $(basename $(SRC_FILES)))

$(TARGET)-y 	+= 	$(OBJ_FILES)
obj-m  			+=  $(TARGET).o

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
	$(shell rm ${OBJ_FILES})
