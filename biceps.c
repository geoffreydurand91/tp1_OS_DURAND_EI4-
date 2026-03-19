#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define NBMAXC 10 // definit la taille maximum du tableau des commandes internes

// definition du type pour stocker une commande interne et sa fonction associee
typedef struct {
    char *nom; // le nom de la commande tapee par l'utilisateur
    int (*fonction)(int N, char *P[]); // le pointeur vers la fonction c correspondante
} CommandeInterne;

// declaration des variables globales utiles dans tout le fichier
static char **Mots = NULL; // tableau dynamique qui contiendra les mots d'une commande
static int NMots = 0; // compteur du nombre de mots trouves
CommandeInterne TabComInt[NBMAXC]; // le tableau qui liste nos commandes internes
int NbComInt = 0; // le compteur de commandes internes actuellement enregistrees

// fonction qui alloue de la memoire pour dupliquer une chaine de caracteres
char *copyString(char *s) {
    if (s == NULL) return NULL; // on evite de copier du vide
    char *copy = malloc(strlen(s) + 1); // on demande la taille de la chaine plus le caractere de fin
    if (copy != NULL) strcpy(copy, s); // on effectue la copie si la memoire a bien ete donnee
    return copy;
}

// fonction qui decoupe une commande simple en un tableau de mots
int analyseCom(char *b) {
    char *delim = " \t\n"; // les caracteres qui separent les mots (espace, tabulation, saut de ligne)
    char *token; // le mot en cours d'extraction
    int capacite_max = 20; // on limite a 20 mots pour economiser la memoire

    // on nettoie le tableau precedent pour eviter de saturer la memoire
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }

    // on cree un nouveau tableau vide pour les mots a venir
    Mots = malloc(capacite_max * sizeof(char *));
    NMots = 0; // on remet le compteur de mots a zero

    // on decoupe la chaine mot par mot grace a strsep
    while ((token = strsep(&b, delim)) != NULL) {
        if (*token != '\0') { // on ignore les mots vides dus aux espaces multiples
            Mots[NMots] = copyString(token); // on range le mot trouve dans notre tableau global
            NMots++; // on incremente notre compteur
        }
    }
    
    // on ajoute un vide a la fin du tableau car la fonction execvp en a besoin
    Mots[NMots] = NULL; 
    return NMots; // on renvoie le nombre de mots trouves
}

// fonction executee quand l'utilisateur tape exit
int Sortie(int N, char *P[]) {
    // on doit vider la memoire proprement avant de tuer le programme
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }
    printf("fermeture du programme...\n");
    exit(0); // on quitte avec le code de succes 0
    return 0; 
}

// fonction executee quand l'utilisateur tape cd
int change_dir(int n, char *p[]) {
    // si un chemin est donne, on essaie d'y aller
    if (n > 1) {
        if (chdir(p[1]) != 0) perror("erreur cd"); // si on echoue, on affiche l'erreur systeme
    } else {
        chdir(getenv("HOME")); // si aucun argument n'est donne, on rentre a la maison
    }
    return 0;
}

// fonction executee quand l'utilisateur tape pwd
int print_wd(int n, char *p[]) {
    char cwd[1024]; // tableau temporaire pour stocker le chemin
    // on demande au systeme de remplir notre tableau avec le chemin actuel
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd); // on l'affiche a l'ecran
    } else {
        perror("erreur pwd"); // on affiche une erreur si le systeme n'y arrive pas
    }
    return 0;
}

// fonction executee quand l'utilisateur tape vers
int version(int n, char *p[]) {
    printf("biceps version 1.0\n"); // on affiche simplement la version en dur
    return 0;
}

// fonction pour inserer une nouvelle commande dans notre tableau
void ajouteCom(char *nom, int (*fonc)(int, char **)) {
    // on bloque le programme si on essaie d'en ajouter trop
    if (NbComInt >= NBMAXC) {
        fprintf(stderr, "le tableau des commandes est plein\n");
        exit(1);
    }
    // on sauvegarde le nom et le code a executer a l'emplacement actuel
    TabComInt[NbComInt].nom = nom;
    TabComInt[NbComInt].fonction = fonc;
    NbComInt++; // on passe a la case suivante pour la prochaine fois
}

// fonction qui regroupe toutes les initialisations de nos commandes internes
void majComInt(void) {
    ajouteCom("exit", Sortie);
    ajouteCom("cd", change_dir);
    ajouteCom("pwd", print_wd);
    ajouteCom("vers", version);
}

// fonction qui cherche et lance une commande si elle est dans notre dictionnaire
int execComInt(int N, char **P) {
    if (N == 0 || P[0] == NULL) return 0; // on ne fait rien s'il n'y a pas de mots
    
    // on passe en revue chaque ligne de notre dictionnaire de commandes
    for (int i = 0; i < NbComInt; i++) {
        // si le premier mot tape correspond a un nom connu
        if (strcmp(P[0], TabComInt[i].nom) == 0) {
            TabComInt[i].fonction(N, P); // on declenche la fonction
            return 1; // on previent qu'on a reussi a la traiter
        }
    }
    return 0; // on previent qu'on a rien trouve
}

// fonction qui cree un processus pour lancer un programme du systeme
int execComExt(char **P) {
    pid_t pid = fork(); // on clone notre programme actuel en deux

    if (pid < 0) {
        perror("erreur fork"); // si le clonage rate
        return -1;
    } else if (pid == 0) {
        // ici on est dans le clone (l'enfant)
#ifdef TRACE
        printf("[trace] processus enfant va executer : %s\n", P[0]);
#endif
        // on demande au systeme d'ecraser l'enfant par le programme voulu
        execvp(P[0], P);
        // si on arrive ici, c'est que execvp n'a pas trouve le programme
        printf("erreur d'execution : commande introuvable\n");
        exit(1); // on tue l'enfant defaillant
    } else {
        // ici on est dans le parent (notre shell)
        waitpid(pid, NULL, 0); // on met le parent en pause jusqu'a ce que l'enfant finisse
#ifdef TRACE
        printf("[trace] processus enfant termine\n");
#endif
    }
    return 0;
}

// fonction pour decouper une ligne contenant plusieurs commandes separees par des points-virgules
void traiterSequence(char *ligne) {
    char *commande_seule; // pointeur pour stocker chaque sous-commande
    char *delimiteur_sequence = ";"; // le caractere qui separe nos commandes

    // on boucle pour decouper la phrase globale a chaque fois qu'on voit un point-virgule
    while ((commande_seule = strsep(&ligne, delimiteur_sequence)) != NULL) {
        // on verifie que le morceau recupere contient bien du texte
        if (strlen(commande_seule) > 0) {
            // on decoupe cette sous-commande en mots (espaces)
            analyseCom(commande_seule);
            
            // si on a bien des mots a traiter
            if (NMots > 0) {
                // on tente l'execution interne en priorite
                if (execComInt(NMots, Mots) == 0) {
                    // si echec, on confie la tache au systeme
                    execComExt(Mots);
                }
            }
        }
    }
}

// fonction principale au lancement du programme
int main(void) {
    char nom_machine[256];
    char *nom_utilisateur;
    char prompt[512];
    char *ligne_saisie;
    char caractere_fin;

    majComInt(); // on charge la memoire des commandes connues

    // on cherche comment s'appelle l'ordinateur
    if (gethostname(nom_machine, sizeof(nom_machine)) != 0) strcpy(nom_machine, "inconnu");
    
    // on regarde dans les variables du terminal qui est connecte
    nom_utilisateur = getenv("USER");
    if (nom_utilisateur == NULL) nom_utilisateur = "inconnu";
    
    // on adapte le symbole final si on est un administrateur ou non
    caractere_fin = (geteuid() == 0) ? '#' : '$';

    // on demarre la boucle infinie qui attend les ordres
    while (1) {
        char cwd[1024];
        char chemin_affiche[1024] = "";
        
        // on verifie dans quel dossier on se trouve actuellement pour l'afficher
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(chemin_affiche, sizeof(chemin_affiche), " [%s]", cwd);
        }
        
        // on fabrique la phrase d'accueil (prompt)
        snprintf(prompt, sizeof(prompt), "%s@%s%s%c ", nom_utilisateur, nom_machine, chemin_affiche, caractere_fin);

        // on bloque le programme en attendant que l'utilisateur tape sur entree
        ligne_saisie = readline(prompt);
        
        // si l'utilisateur appuie sur ctrl+d (fin de fichier), on arrete la boucle
        if (ligne_saisie == NULL) {
            printf("\n");
            break;
        }

        // si on a tape autre chose que juste la touche entree
        if (strlen(ligne_saisie) > 0) {
            // on sauvegarde la frappe pour la reutiliser avec les fleches
            add_history(ligne_saisie);
            // on lance le traitement de toute la ligne (qui peut contenir des points-virgules)
            traiterSequence(ligne_saisie);
        }
        // on vide la ligne lue pour preparer le prochain tour
        free(ligne_saisie);
    }
    
    // grand menage avant la fermeture definitive
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }
    return 0;
}