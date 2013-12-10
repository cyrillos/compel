lib-so			+= libcompel
obj-y			+= ptrace.o
obj-y			+= compel.o
obj-y			+= string.o
obj-y			+= util.o
obj-y			+= pstree.o
obj-y			+= proc.o

cflags-y		+= -fPIC -Wa,--noexecstack -fno-stack-protector
cflags-so		+= -rdynamic

.SECONDARY:

ifneq ($(MAKECMDGOALS),clean)
incdeps := y
endif
