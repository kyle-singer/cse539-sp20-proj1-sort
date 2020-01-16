CLASS_DIR=/project/linuxlab/class/cse539
CC=$(CLASS_DIR)/tapir/build/bin/clang
CXX=$(CLASS_DIR)/tapir/build/bin/clang++

# vanilla cilk plus runtime location
CILK_DIR=$(CLASS_DIR)/cilkplus-rts
CILK_LIBS=$(CILK_DIR)/libs

CFLAGS = -ggdb -O3 -fcilkplus
LIBS = -L$(CILK_LIBS) -Wl,-rpath -Wl,$(CILK_LIBS) -lcilkrts -lpthread -lrt -lm
INST_LIBS = -L$(CILK_INST_LIBS) -Wl,-rpath -Wl,$(CILK_INST_LIBS) -lcilkrts -lpthread -lrt -lm

PROGS = sort

all:: $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< -I$(CILK_DIR)/include

sort: pthread_sort.o cilk_sort.o main.o ktiming.o
	$(CC) -o $@ $^ $(LIBS) 

clean:: 
	-rm -f $(PROGS) *.o
