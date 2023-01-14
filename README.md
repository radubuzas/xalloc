# xalloc

This is a memory allocator.

## Roadmap

1. Create daemon skeleton and check that it runs separately from the parent process.
2. Add logging functionality. Check that it can log to syslog and maybe copy logs to a separate file with the help of a bash script.
3. Add shared memory initialization for _shm\_request_ and _shm\_response_, and create functions for reading/writing to these objects.
4. Create automation for compiling the library, possibly through makefiles/bash scripts. 
5. Create a separate process and test communication with the daemon through the shared memory files.

## How to run the program

All the compilation is managed by two makefiles, one in the root of the project, and another in `./lib`. While the former checks for any changes made to `main.c` and the static library object, `./lib/libxalloc.a`, the latter watches for all source and header files in `./lib`. To compile `main.c` and run the memory allocator run the following command in the root of the project:
```
$ make
$ ./xalloc
```

To recompile the library after some changes made to its files type:
```
$ make lib
```

## Setup system logging

In order to debug the daemon's actions we try to log as many of the process's actions using syslog. This logging utility will write the logs in a file stored at `/var/log/syslog`, where every other process is logging. Therefore, we want to write our logs into another file to make debugging easier, although this is optional and does not affect the memory allocator's functioning.

In order to redirect daemon logs to another log file follow these steps:

1. Open syslog configuration file for editing, it can be either one of these: `/etc/syslog.conf` or `/etc/rsyslog.conf`. Check which one is present and open it with root permissions (I have rsyslog file):
```
$ sudo nano /etc/rsyslog.conf
```

2. At the end of this file add the following line:
```
$ :programname,isequal,"xallo" /var/log/xalloc.log
```
This syntax is called property-based filters in RSyslog configuration terminology. By doing this you specify that you want every system log whose `programname` property is equal to `xalloc` to be also logged to `/var/log/xalloc.log`. The `programname` property isn't necessarily the name of the executable running the daemon, but the value passed as `ident` parameter to the `openlog` call. If `NULL` is passed then `programname` will equal to the name of the executable. 

3. Restart the logging server:
```
$ service rsyslog restart
```
I restart the `rsyslog` service because I changed `rsyslog.conf`. If you modify `syslog.conf` you should probably restart the `syslog` service.
Now the every log made by the allocator daemon can be checked at `/var/log/xalloc.log`!
