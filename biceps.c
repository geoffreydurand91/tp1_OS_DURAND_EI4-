#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// variables globales pour les mots de la commande
char **Mots = NULL;
int NMots = 0;

// fonction pour copier dynamiquement une chaine
char *copyString(char *s) {
    if (s == NULL) return NULL;
    int longueur = strlen(s);
    char *copie = malloc(longueur + 1);
    if (copie != NULL) {
        strcpy(copie, s);
    }
    return copie;
}

// fonction pour analyser la commande et compter les mots
int analyseCom(char *b) {
    char *token;
    char *delim = " \t\n";
    int capacite = 10;
    
    // liberation de l'ancien tableau si existant
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }
    
    // allocation initiale du tableau de mots
    Mots = malloc(capacite * sizeof(char *));
    NMots = 0;
    char *chaine_a_analyser = b;
    
    // decoupage de la chaine
    while ((token = strsep(&chaine_a_analyser, delim)) != NULL) {
        if (*token != '\0') {
            // reallocation si le tableau est plein
            if (NMots >= capacite) {
                capacite *= 2;
                Mots = realloc(Mots, capacite * sizeof(char *));
            }
            Mots[NMots] = copyString(token);
            NMots++;
        }
    }
    
    // ajout d'un pointeur nul a la fin (utile pour execvp plus tard)
    if (NMots >= capacite) {
        Mots = realloc(Mots, (NMots + 1) * sizeof(char *));
    }
    Mots[NMots] = NULL;
    
    return NMots;
}

int main(void) {
    char nom_machine[256];
    char *nom_utilisateur;
    char prompt[512];
    char *ligne_saisie;
    char caractere_fin;

    // recuperation du nom de la machine
    if (gethostname(nom_machine, sizeof(nom_machine)) != 0) {
        strcpy(nom_machine, "machine_inconnue");
    }

    // recuperation de la variable d'environnement user
    nom_utilisateur = getenv("USER");
    if (nom_utilisateur == NULL) {
        nom_utilisateur = "inconnu";
    }

    // verification des droits pour le caractere du prompt
    if (geteuid() == 0) {
        caractere_fin = '#';
    } else {
        caractere_fin = '$';
    }

    // formatage dynamique de la chaine du prompt
    snprintf(prompt, sizeof(prompt), "%s@%s%c ", nom_utilisateur, nom_machine, caractere_fin);

    while (1) {
        ligne_saisie = readline(prompt);
        
        // gestion du ctrl+d (eof)
        if (ligne_saisie == NULL) {
            printf("\n");
            break;
        }

        // traitement de la saisie si non vide
        if (strlen(ligne_saisie) > 0) {
            add_history(ligne_saisie);
            analyseCom(ligne_saisie);
            
            // affichage du nom de la commande et de ses parametres
            if (NMots > 0) {
                printf("commande : %s\n", Mots[0]);
                for (int i = 1; i < NMots; i++) {
                    printf("parametre %d : %s\n", i, Mots[i]);
                }
            }
        }

        // liberation de la memoire allouee par readline
        free(ligne_saisie);
    }
    
    // liberation finale du tableau global avant de quitter
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) {
            free(Mots[i]);
        }
        free(Mots);
    }

    return 0;
}