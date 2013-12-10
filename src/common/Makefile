obj-y			+= log.o
obj-y			+= string.o
obj-y			+= util.o

cflags-y		+= -fPIC -Wa,--noexecstack -fno-stack-protector

.SECONDARY:

ifneq ($(MAKECMDGOALS),clean)
incdeps := y
endif
