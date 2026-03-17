#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define NBMAXC 10 // limite du nombre de commandes internes a 10

// structure pour lier un nom de commande a une fonction c
typedef struct {
    char *nom;
    int (*fonction)(int N, char *P[]);
} CommandeInterne;

// variables globales pour le decoupage des mots de la commande
static char **Mots = NULL; 
static int NMots = 0;      
CommandeInterne TabComInt[NBMAXC]; // tableau stockant nos commandes internes
int NbComInt = 0; // index pour savoir combien de commandes sont enregistrees

// alloue dynamiquement de la memoire pour copier une chaine de caracteres
char *copyString(char *s) {
    if (s == NULL) return NULL;
    char *copy = malloc(strlen(s) + 1);
    // on verifie que le malloc a reussi avant de copier
    if (copy != NULL) {
        strcpy(copy, s);
    }
    return copy;
}

// separe la ligne de commande en plusieurs mots distincts
int analyseCom(char *b) {
    char *delim = " \t\n";
    char *token;
    int capacite_max = 20;

    // nettoyage de l'ancienne analyse pour eviter les fuites memoire
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }

    // allocation du nouveau tableau pouvant contenir jusqu'a 20 mots
    Mots = malloc(capacite_max * sizeof(char *));
    NMots = 0;

    // decoupage de la chaine tant qu'il y a des separateurs
    while ((token = strsep(&b, delim)) != NULL) {
        if (*token != '\0') {
            Mots[NMots] = copyString(token);
            NMots++;
        }
    }
    
    // ajout du pointeur null a la fin du tableau, obligatoire pour execvp
    Mots[NMots] = NULL; 
    return NMots;
}

// fonction appelee lors de la saisie de la commande exit
int Sortie(int N, char *P[]) {
    // on libere la memoire des mots avant de tuer le processus
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }
    printf("fermeture du programme...\n");
    exit(0);
    return 0; 
}

// ajoute une nouvelle commande dans notre dictionnaire interne
void ajouteCom(char *nom, int (*fonc)(int, char **)) {
    // securite pour ne pas depasser la taille du tableau
    if (NbComInt >= NBMAXC) {
        fprintf(stderr, "le tableau des commandes est plein\n");
        exit(1);
    }
    // assignation du nom et de la fonction dans la structure
    TabComInt[NbComInt].nom = nom;
    TabComInt[NbComInt].fonction = fonc;
    NbComInt++;
}

// initialise la liste des commandes internes au lancement
void majComInt(void) {
    ajouteCom("exit", Sortie);
}

// verifie si le premier mot correspond a une commande interne et l'execute
int execComInt(int N, char **P) {
    if (N == 0 || P[0] == NULL) return 0;
    
    // on boucle sur toutes les commandes internes enregistrees
    for (int i = 0; i < NbComInt; i++) {
        // si le nom correspond, on lance la fonction associee
        if (strcmp(P[0], TabComInt[i].nom) == 0) {
            TabComInt[i].fonction(N, P);
            return 1; // indique que la commande a ete traitee
        }
    }
    return 0; // indique que ce n'est pas une commande interne
}

// cree un processus enfant pour executer une commande systeme externe
int execComExt(char **P) {
    pid_t pid;
    int status;

    // creation du processus fils
    pid = fork();

    if (pid < 0) {
        // si le fork echoue, on affiche une erreur
        perror("erreur lors de la creation du processus fils");
        return -1;
    } else if (pid == 0) {
        // on est dans le processus fils, on affiche une trace si demande a la compilation
#ifdef TRACE
        printf("[trace] execution de la commande externe : %s\n", P[0]);
#endif
        // on remplace le code du fils par le programme externe appele
        execvp(P[0], P);
        
        // si execvp rend la main, c'est que la commande n'a pas ete trouvee ou a echoue
        printf("commande externe non trouvee ou erreur d'execution : %s\n", P[0]);
        // on quitte le fils en erreur pour ne pas dupliquer le shell
        exit(1); 
    } else {
        // on est dans le processus pere, on attend la fin de l'execution du fils
        waitpid(pid, &status, 0);
        
        // affichage d'une trace de fin si demande a la compilation
#ifdef TRACE
        if (WIFEXITED(status)) {
            printf("[trace] le processus fils s'est termine avec le code : %d\n", WEXITSTATUS(status));
        }
#endif
    }
    return 0;
}

int main(void) {
    char nom_machine[256];
    char *nom_utilisateur;
    char prompt[512];
    char *ligne_saisie;
    char caractere_fin;

    // chargement des commandes internes
    majComInt();

    // recuperation du nom de l'ordinateur
    if (gethostname(nom_machine, sizeof(nom_machine)) != 0) {
        strcpy(nom_machine, "inconnu");
    }

    // recuperation du nom de la session utilisateur
    nom_utilisateur = getenv("USER");
    if (nom_utilisateur == NULL) {
        nom_utilisateur = "inconnu";
    }

    // definition du caractere de prompt selon les droits d'administration
    if (geteuid() == 0) {
        caractere_fin = '#';
    } else {
        caractere_fin = '$';
    }

    // construction de la chaine de caracteres affichee a l'ecran
    snprintf(prompt, sizeof(prompt), "%s@%s%c ", nom_utilisateur, nom_machine, caractere_fin);

    // boucle infinie pour demander des commandes a l'utilisateur
    while (1) {
        ligne_saisie = readline(prompt);
        
        // gestion de la fin de fichier via ctrl+d
        if (ligne_saisie == NULL) {
            printf("\n");
            break;
        }

        // si la ligne n'est pas vide, on la traite
        if (strlen(ligne_saisie) > 0) {
            // on ajoute la commande a l'historique pour utiliser les fleches du clavier
            add_history(ligne_saisie);
            // on decoupe la commande
            analyseCom(ligne_saisie);
            
            if (NMots > 0) {
                // on tente d'abord de lancer une commande interne
                if (execComInt(NMots, Mots) == 0) {
                    // si elle n'est pas interne, on tente de l'executer comme commande externe
                    execComExt(Mots);
                }
            }
        }
        // on libere la memoire allouee par la fonction readline
        free(ligne_saisie);
    }
    
    // nettoyage final si on sort de la boucle
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }

    return 0;
}