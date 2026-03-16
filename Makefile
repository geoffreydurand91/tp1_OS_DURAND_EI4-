CC = gcc
CFLAGS = -Wall
LDFLAGS = -lreadline

biceps: biceps.c
	$(CC) $(CFLAGS) -o biceps biceps.c $(LDFLAGS)

clean:
	rm -f biceps