COMP=../compel
COMP_DEV=../devel/
COMP_LIB=../src/lib/
COMP_PLUGIN=$(COMP_DEV)/lib/compel/
COMP_INC=$(COMP_DEV)/include/
CC=gcc

all: piggie parasite.co

test: all
	./piggie ./parasite.co > piggie.out

.PHONY: all

parasite.co: parasite.o
	$(COMP) pack $< -l fds -l shmem -o $@ -L $(COMP_PLUGIN)

parasite.o: $(OBJ)/parasite.c
	$(CC) -c -o $@ $< $(shell $(COMP) cflags) -I$(COMP_INC)

piggie: $(OBJ)/piggie.c
	$(CC) -o $@ $< -I$(COMP_INC) -lcompel -L$(COMP_LIB)

clean:
	rm -f parasite.o
	rm -f parasite.co
	rm -f piggie