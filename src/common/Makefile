obj-y			+= log.o

cflags-y		+= -fPIC -Wa,--noexecstack -fno-stack-protector

.SECONDARY:

ifneq ($(MAKECMDGOALS),clean)
incdeps := y
endif
