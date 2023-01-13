# xalloc

This is a memory allocator.

## Roadmap

1. Create daemon skeleton and check that it runs separately from the parent process.
2. Add logging functionality. Check that it can log to syslog and maybe copy logs to a separate file with the help of a bash script.
3. Add shared memory initialization for _shm\_request_ and _shm\_response_, and create functions for reading/writing to these objects.
4. Create automation for compiling the library, possibly through makefiles/bash scripts. 
5. Create a separate process and test communication with the daemon through the shared memory files.
