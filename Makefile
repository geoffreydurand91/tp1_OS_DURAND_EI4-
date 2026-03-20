CC = gcc
CFLAGS = -Wall
LDFLAGS = -lreadline

# on assemble les deux fichiers objets pour creer l'executable
biceps: biceps.o gescom.o
	$(CC) $(CFLAGS) -o biceps biceps.o gescom.o $(LDFLAGS)

# compilation du programme principal
biceps.o: biceps.c gescom.h
	$(CC) $(CFLAGS) -c biceps.c

# compilation de la bibliotheque de gestion de commandes
gescom.o: gescom.c gescom.h
	$(CC) $(CFLAGS) -c gescom.c

# menage des fichiers binaires
clean:
	rm -f biceps *.o