lib-so			+= libcompel
obj-y			+= compel.o
obj-y			+= string.o
obj-y			+= util.o

cflags-y		+= -fPIC -Wa,--noexecstack -fno-stack-protector
cflags-so		+= -rdynamic

.SECONDARY:

ifneq ($(MAKECMDGOALS),clean)
incdeps := y
endif
