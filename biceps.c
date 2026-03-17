#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define NBMAXC 10 // limite du nombre de commandes internes a 10

// on cree un moule pour associer un mot cle a un bout de code c
typedef struct {
    char *nom;
    int (*fonction)(int N, char *P[]);
} CommandeInterne;

// variables globales pour garder la trace des mots decoupable
static char **Mots = NULL; 
static int NMots = 0;      
CommandeInterne TabComInt[NBMAXC]; // notre dictionnaire de commandes
int NbComInt = 0; // le compteur pour savoir ou on en est dans le dico

// copie une chaine proprement dans la memoire
char *copyString(char *s) {
    if (s == NULL) return NULL;
    char *copy = malloc(strlen(s) + 1);
    if (copy != NULL) {
        strcpy(copy, s);
    }
    return copy;
}

// decoupe la phrase tapee par l'utilisateur en mots separes
int analyseCom(char *b) {
    char *delim = " \t\n";
    char *token;
    int capacite_max = 20;

    // on vide l'ancienne commande si elle existe
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }

    // on prepare le nouveau tableau de mots
    Mots = malloc(capacite_max * sizeof(char *));
    NMots = 0;

    // on lit chaque morceau et on le range
    while ((token = strsep(&b, delim)) != NULL) {
        if (*token != '\0') {
            Mots[NMots] = copyString(token);
            NMots++;
        }
    }
    
    // on met un vide a la fin pour eviter les bugs plus tard
    Mots[NMots] = NULL; 
    return NMots;
}

// ce qui se passe quand on tape exit
int Sortie(int N, char *P[]) {
    // on nettoie la memoire avant de partir
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

// fonction pour apprendre une nouvelle commande a notre programme
void ajouteCom(char *nom, int (*fonc)(int, char **)) {
    // on s'assure qu'on a encore de la place dans le tableau
    if (NbComInt >= NBMAXC) {
        fprintf(stderr, "le tableau des commandes est plein\n");
        exit(1);
    }
    // on enregistre le nom et la fonction correspondante
    TabComInt[NbComInt].nom = nom;
    TabComInt[NbComInt].fonction = fonc;
    NbComInt++;
}

// c'est ici qu'on charge les commandes connues au lancement
void majComInt(void) {
    ajouteCom("exit", Sortie);
}

// pour verifier ce qu'on a enregistre
void listeComInt(void) {
    printf("--- liste des commandes internes ---\n");
    for (int i = 0; i < NbComInt; i++) {
        printf("- %s\n", TabComInt[i].nom);
    }
    printf("------------------------------------\n");
}

// verifie si ce qu'on a tape est connu, et lance le code si c'est le cas
int execComInt(int N, char **P) {
    if (N == 0 || P[0] == NULL) return 0;
    
    // on compare le premier mot avec notre dictionnaire
    for (int i = 0; i < NbComInt; i++) {
        if (strcmp(P[0], TabComInt[i].nom) == 0) {
            // on a trouve, donc on lance la fonction
            TabComInt[i].fonction(N, P);
            return 1; // on dit au programme principal que c'est bon
        }
    }
    return 0; // on a rien trouve de correspondant
}

int main(void) {
    char nom_machine[256];
    char *nom_utilisateur;
    char prompt[512];
    char *ligne_saisie;
    char caractere_fin;

    // on met a jour le dictionnaire des commandes des le debut
    majComInt();

    // preparation de l'affichage du terminal
    if (gethostname(nom_machine, sizeof(nom_machine)) != 0) {
        strcpy(nom_machine, "inconnu");
    }

    nom_utilisateur = getenv("USER");
    if (nom_utilisateur == NULL) {
        nom_utilisateur = "inconnu";
    }

    if (geteuid() == 0) {
        caractere_fin = '#';
    } else {
        caractere_fin = '$';
    }

    snprintf(prompt, sizeof(prompt), "%s@%s%c ", nom_utilisateur, nom_machine, caractere_fin);

    // le coeur du programme qui tourne en boucle
    while (1) {
        ligne_saisie = readline(prompt);
        
        // on quitte si on fait ctrl+d
        if (ligne_saisie == NULL) {
            printf("\n");
            break;
        }

        if (strlen(ligne_saisie) > 0) {
            add_history(ligne_saisie);
            analyseCom(ligne_saisie);
            
            if (NMots > 0) {
                // on teste si le premier mot tape est une de nos commandes
                if (execComInt(NMots, Mots) == 0) {
                    // si la fonction a renvoye 0, on previent que ca n'existe pas
                    printf("la commande '%s' n'est pas reconnue pour le moment.\n", Mots[0]);
                }
            }
        }
        free(ligne_saisie);
    }
    
    // nettoyage de fin
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }

    return 0;
}