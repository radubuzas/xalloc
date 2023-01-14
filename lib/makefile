CC=gcc
CFLAGS= -Wall

# source files directory
SRCDIR=./src/
# object files directory
OBJDIR=./obj/
# source files names
SOURCES=$(wildcard $(SRCDIR)*.c)
# object files names
OBJECTS=$(patsubst $(SRCDIR)%.c,$(OBJDIR)%.o,$(SOURCES))

# make static library
libxalloc.a: $(OBJECTS)
	ar -rcsv libxalloc.a $(OBJDIR)*.o

# make an object file in ./obj for every source file in ./src
$(OBJDIR)%.o: $(SRCDIR)%.c | ./obj
	$(CC) $(CFLAGS) -c -o $@ $<

# make obj directory
./obj:
	mkdir obj

# clean tasks
.PHONY: clean

# remove object files and static library
clean:
	rm -f obj/*.o
	rm -f libxalloc.a