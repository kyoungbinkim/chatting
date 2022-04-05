#ifndef _INCUDESH_
#define _INCUDESH_
#include <sys/types.h>// open(), creat(), lseek()
#include <sys/stat.h>	// open(), creat()
#include <fcntl.h>	// open(), creat()
#include <unistd.h>	// read(), write(), lseek(), unlink() , fcntl()
#include <sys/ioctl.h>	// ioctl()
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <linux/fs.h>
#include <sched.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>	// struct passwd
#include <dirent.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <malloc.h>
#include <alloca.h>
#include <signal.h>
#include <sys/times.h>
#include <assert.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif