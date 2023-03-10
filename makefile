CC=gcc
CFLAGS= -Wall -lrt -pthread -lpthread

# library directory
LIBDIR=./lib/
# library name
LIB=xalloc
# executable name
EXE=$(LIB)

# make executable
xalloc: main.c $(LIBDIR)lib$(LIB).a
	$(CC) -o $(EXE) main.c -L$(LIBDIR) -l$(LIB) $(CFLAGS)

# make xalloc static library
$(LIBDIR)lib$(LIB).a:
	make lib;

.PHONY: clean cleanlib cleanall lib all tests

tests:
	gcc -o test a.c -L./lib/ -lxalloc $(CFLAGS)

all:
	make lib
	make
	make tests

# make library
lib:
	cd $(LIBDIR); make;

# delete executable
clean:
	rm -f $(EXE)

# execute clean task in library
cleanlib:
	cd $(LIBDIR); make clean;

# execute both clean tasks
cleanall: clean cleanlib
