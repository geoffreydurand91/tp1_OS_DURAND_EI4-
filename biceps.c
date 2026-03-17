#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define NBMAXC 10 // limite des commandes internes

typedef struct {
    char *nom;
    int (*fonction)(int N, char *P[]);
} CommandeInterne;

static char **Mots = NULL; 
static int NMots = 0;      
CommandeInterne TabComInt[NBMAXC]; 
int NbComInt = 0; 

// copie de chaine dynamique
char *copyString(char *s) {
    if (s == NULL) return NULL;
    char *copy = malloc(strlen(s) + 1);
    if (copy != NULL) strcpy(copy, s);
    return copy;
}

// decoupage de la ligne
int analyseCom(char *b) {
    char *delim = " \t\n";
    char *token;
    int capacite_max = 20;

    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }

    Mots = malloc(capacite_max * sizeof(char *));
    NMots = 0;

    while ((token = strsep(&b, delim)) != NULL) {
        if (*token != '\0') {
            Mots[NMots] = copyString(token);
            NMots++;
        }
    }
    
    Mots[NMots] = NULL; 
    return NMots;
}

// commande exit
int Sortie(int N, char *P[]) {
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }
    printf("fermeture du programme...\n");
    exit(0);
    return 0; 
}

// commande cd
int change_dir(int n, char *p[]) {
    // si un chemin est fourni on y va, sinon on va dans home
    if (n > 1) {
        if (chdir(p[1]) != 0) perror("erreur cd");
    } else {
        chdir(getenv("HOME"));
    }
    return 0;
}

// commande pwd
int print_wd(int n, char *p[]) {
    char cwd[1024];
    // on recupere et affiche le chemin courant
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("erreur pwd");
    }
    return 0;
}

// commande vers
int version(int n, char *p[]) {
    printf("biceps version 1.0\n");
    return 0;
}

// enregistrement dans le tableau
void ajouteCom(char *nom, int (*fonc)(int, char **)) {
    if (NbComInt >= NBMAXC) {
        fprintf(stderr, "le tableau des commandes est plein\n");
        exit(1);
    }
    TabComInt[NbComInt].nom = nom;
    TabComInt[NbComInt].fonction = fonc;
    NbComInt++;
}

// on ajoute nos nouvelles commandes ici
void majComInt(void) {
    ajouteCom("exit", Sortie);
    ajouteCom("cd", change_dir);
    ajouteCom("pwd", print_wd);
    ajouteCom("vers", version);
}

// execution si interne
int execComInt(int N, char **P) {
    if (N == 0 || P[0] == NULL) return 0;
    
    for (int i = 0; i < NbComInt; i++) {
        if (strcmp(P[0], TabComInt[i].nom) == 0) {
            TabComInt[i].fonction(N, P);
            return 1; 
        }
    }
    return 0; 
}

// execution si externe
int execComExt(char **P) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("erreur fork");
        return -1;
    } else if (pid == 0) {
#ifdef TRACE
        printf("[trace] processus enfant va executer : %s\n", P[0]);
#endif
        execvp(P[0], P);
        printf("erreur d'execution : commande introuvable\n");
        exit(1); 
    } else {
        waitpid(pid, NULL, 0);
#ifdef TRACE
        printf("[trace] processus enfant termine\n");
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

    majComInt();

    if (gethostname(nom_machine, sizeof(nom_machine)) != 0) strcpy(nom_machine, "inconnu");
    nom_utilisateur = getenv("USER");
    if (nom_utilisateur == NULL) nom_utilisateur = "inconnu";
    caractere_fin = (geteuid() == 0) ? '#' : '$';

    while (1) {
        // mise a jour dynamique du prompt pour voir l'effet du cd
        char cwd[1024];
        char chemin_affiche[1024] = "";
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(chemin_affiche, sizeof(chemin_affiche), " [%s]", cwd);
        }
        snprintf(prompt, sizeof(prompt), "%s@%s%s%c ", nom_utilisateur, nom_machine, chemin_affiche, caractere_fin);

        ligne_saisie = readline(prompt);
        
        if (ligne_saisie == NULL) {
            printf("\n");
            break;
        }

        if (strlen(ligne_saisie) > 0) {
            add_history(ligne_saisie);
            analyseCom(ligne_saisie);
            
            if (NMots > 0) {
                if (execComInt(NMots, Mots) == 0) {
                    execComExt(Mots);
                }
            }
        }
        free(ligne_saisie);
    }
    
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }
    return 0;
}