#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

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
        
        // gestion du ctrl+d (eof) pour eviter une boucle infinie si readline echoue 
        if (ligne_saisie == NULL) {
            printf("\n");
            break;
        }

        // affichage de la commande et ajout a l'historique si non vide 
        if (strlen(ligne_saisie) > 0) {
            add_history(ligne_saisie);
            printf("%s\n", ligne_saisie);
        }

        // liberation de la memoire allouee par readline 
        free(ligne_saisie);
    }

    return 0;
}