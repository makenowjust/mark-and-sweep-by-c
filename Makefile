CC      = gcc -Wall
CFLAGS +=  \
	-std=c99 \
	-DDEBUG

SRC =              \
  mark_and_sweep.c \
  test.c

OBJ = $(SRC:.c=.o)

all: $(OBJ) test.exe

test.exe: $(SRC)
	$(CC) -o test.exe $(OBJ)

clean:
	rm -vf *.o test.exe

mark_and_sweep.o: mark_and_sweep.h
test.o:           mark_and_sweep.h

.c.o:
	$(CC) $(CFLAGS) -c $<

