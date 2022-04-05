CC = gcc
LDFLAGS = -lrt
RM = /bin/rm -f

.SUFFIXES : .c .o

INC = 
LIBS = 
CFLAGS = -D_REENTRANT  -pthread -g -w -Wall

EXESRCS = final_user.c 

LIBOBJS = ${LIBSRC:.c=.o}
OBJECTS = ${EXESRCS:.c=.o}

TARGETS = final_user

$@.o : $@.c
	$(CC) $(CFLAGS) -c -o $<

all: targ1 targ2

targ1: ${LIBOBJS} ${OBJECTS}
	${CC} -o ${TARGETS} ${OBJECTS} ${LDFLAGS} ${LIBOBJS} ${CFLAGS} 

clean :
	${RM} ${OBJECTS} ${OBJECTS2} ${TARGETS} ${LIBOBJS} core tags
