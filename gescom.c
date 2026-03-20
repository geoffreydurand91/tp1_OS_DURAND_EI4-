#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "gescom.h"

// on definit la limite de commandes internes que le tableau peut stocker
#define NBMAXC 10

// structure associant une chaine de caracteres a un pointeur de fonction
typedef struct {
    char *nom;
    int (*fonction)(int N, char *P[]);
} CommandeInterne;

// variables globales a ce fichier pour stocker l'etat de la commande en cours
static char **Mots = NULL;
static int NMots = 0;
CommandeInterne TabComInt[NBMAXC];
int NbComInt = 0;

// fonction dediee a la liberation propre de la memoire dynamique allouee
void libererMots(void) {
    if (Mots != NULL) {
        // on parcourt chaque mot pour liberer sa memoire specifique
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        // on libere ensuite le tableau de pointeurs lui-meme
        free(Mots);
        // on reinitialise les compteurs pour eviter les pointeurs fous (dangling pointers)
        Mots = NULL;
        NMots = 0;
    }
}

// analyse et decoupage d'une chaine de caracteres en tableau d'arguments
int analyseCom(char *b) {
    char *delim = " \t\n"; // delimiteurs classiques du shell
    char *token;
    int capacite_max = 20; // allocation arbitraire pour des raisons de token efficiency

    // on nettoie d'abord les residus d'une eventuelle commande precedente
    libererMots();

    // allocation dynamique d'un nouveau tableau de pointeurs
    Mots = malloc(capacite_max * sizeof(char *));
    NMots = 0;

    // boucle de decoupage: strsep remplace les delimiteurs par des caracteres nuls
    while ((token = strsep(&b, delim)) != NULL) {
        // on ignore les chaines vides (cas de plusieurs espaces consecutifs)
        if (*token != '\0') {
            // strdup alloue la memoire et copie la chaine en une seule operation standard
            Mots[NMots] = strdup(token);
            NMots++;
        }
    }
    
    // la norme posix pour execvp exige que le tableau de pointeurs se termine par null
    Mots[NMots] = NULL; 
    return NMots;
}

// enregistrement d'une nouvelle fonction dans le dictionnaire des commandes internes
void ajouteCom(char *nom, int (*fonc)(int, char **)) {
    // verification de la capacite du tableau avant insertion
    if (NbComInt >= NBMAXC) {
        fprintf(stderr, "le tableau des commandes est plein\n");
        exit(1);
    }
    // on duplique le nom pour s'assurer qu'il persiste en memoire
    TabComInt[NbComInt].nom = strdup(nom);
    // on lie le pointeur de la fonction c correspondante
    TabComInt[NbComInt].fonction = fonc;
    // on avance l'index pour la prochaine insertion
    NbComInt++;
}

// recherche sequentielle et execution d'une commande interne
int execComInt(int N, char **P) {
    // protection contre les appels vides
    if (N == 0 || P[0] == NULL) return 0;
    
    // parcours du tableau des commandes connues
    for (int i = 0; i < NbComInt; i++) {
        // si le premier argument correspond a un nom connu
        if (strcmp(P[0], TabComInt[i].nom) == 0) {
            // on execute la fonction pointee en lui passant les arguments
            TabComInt[i].fonction(N, P);
            // on signale le succes de l'execution interne
            return 1;
        }
    }
    // on signale que la commande n'a pas ete trouvee
    return 0;
}

// creation d'un processus enfant pour deleguer l'execution a l'os
int execComExt(char **P) {
    // la fonction fork duplique le processus courant
    pid_t pid = fork();

    // gestion du cas ou le clonage echoue (manque de ressources)
    if (pid < 0) {
        perror("erreur fork");
        return -1;
    } else if (pid == 0) {
        // nous sommes dans le processus fils (pid == 0)
#ifdef TRACE
        printf("[trace] processus enfant va executer : %s\n", P[0]);
#endif
        // execvp remplace l'image memoire du fils par le programme demande
        execvp(P[0], P);
        // ce code n'est atteint que si execvp a echoue (commande introuvable)
        printf("erreur d'execution : commande introuvable\n");
        // on termine le fils en erreur pour eviter d'avoir deux shells en parallele
        exit(1);
    } else {
        // nous sommes dans le processus pere (notre shell interactif)
        // on met le pere en attente bloquante jusqu'a la fin du fils
        waitpid(pid, NULL, 0);
#ifdef TRACE
        printf("[trace] processus enfant termine\n");
#endif
    }
    return 0;
}

// gestion de l'enchainement des commandes separees par des points-virgules
void traiterSequence(char *ligne) {
    char *commande_seule;
    char *delimiteur_sequence = ";";

    // decoupage preliminaire de la ligne brute par blocs de sous-commandes
    while ((commande_seule = strsep(&ligne, delimiteur_sequence)) != NULL) {
        // verification que la sous-commande n'est pas vide
        if (strlen(commande_seule) > 0) {
            // on envoie la sous-commande a l'analyseur de mots
            analyseCom(commande_seule);
            // si l'analyse a retourne au moins un mot valide
            if (NMots > 0) {
                // on essaie d'abord la resolution interne, sinon resolution systeme
                if (execComInt(NMots, Mots) == 0) {
                    execComExt(Mots);
                }
            }
        }
    }
}