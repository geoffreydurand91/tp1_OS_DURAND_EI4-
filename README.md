## Architecture des fichiers

* 'biceps.c' : L'interface utilisateur. Ce fichier contient la boucle  principale, la gestion du prompt dynamique, l'historique des saisies ('readline'), et la définition des commandes internes basiques ('cd', 'pwd', 'vers', 'exit').
* 'gescom.c' : Le moteur d'exécution (librairie de gestion des commandes). Il isole toute la logique de traitement : découpage des chaînes de caractères, création des processus enfants ('fork'/'execvp'), routage des tubes de communication (pipes), et application des redirections de flux.
* 'gescom.h' : Le fichier d'en-tête liant l'interface au moteur. Il déclare les structures et fonctions publiques de 'gescom.c' utilisables par le programme principal.
* 'Makefile' : Le script d'automatisation de la compilation. Il permet de générer les fichiers objets séparément et de réaliser l'édition des liens finale pour produire l'exécutable 'biceps'.