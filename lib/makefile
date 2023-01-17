CC=gcc
CFLAGS= -Wall -pthread -lpthread -lrt

# source files directory
SRCDIR=./src/
# header files directory
INCLDIR=./include/
# object files directory
OBJDIR=./obj/
# source files names
SOURCES=$(wildcard $(SRCDIR)*.c)
# header files names
HEADERS=$(wildcard $(INCLDIR)*.h)
# object files names
OBJECTS=$(patsubst $(SRCDIR)%.c,$(OBJDIR)%.o,$(SOURCES))

# make static library
libxalloc.a: $(OBJECTS)
	ar -rcsv libxalloc.a $(OBJDIR)*.o

# make an object file in ./obj for every source file in ./src
$(OBJDIR)%.o: $(SRCDIR)%.c $(HEADERS) | ./obj
	$(CC) $(CFLAGS) -c -o $@ $<

# make obj directory
./obj:
	mkdir obj

# clean tasks
.PHONY: clean

# remove object files (+ obj directory) and static library
clean:
	rm -rf obj
	rm -f libxalloc.a
