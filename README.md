
# The Mind Online

Pour le moment, cette version du jeu ne peut être jouer qu'en local depuis un ordinateur personnel.
## Comment jouer ?

### Compilation des différents exécutables

Pour compiler les exécutables du jeu :

```bash
  make
```

### Lancement du serveur

Pour lancer le serveur :

```bash
  make run-server
```
Pour changer le numéro de port, il vous suffit de modifier la variable associé dans le fichier Makefile

### Lancement du client du jeu

Pour lancer un client :

```bash
  make run-client
```
Pour lancer un client robot :

```bash
  make run-robot
```
### Suppression des fichiers

Pour supprimer les fichiers de compilation et les exécutables :

```bash
  make clean
```

## Visualisation des statistiques.

Les statistiques sont collectées uniquement si la partie s'est terminée correctement.

Elles sont contenus dans le fichier texte "donnees.txt".

Elles comportent : 
- Le temps de réaction moyen
- La dernière carte qui a été jouée
- La dernière manche jouée 
- La date

### Génération du PDF 
```bash
  ./stats.sh
```
La PDF contient plusieurs graphiques :
- Le temps de réaction par rapport à la dernière manche jouée
- La fréquence de la dernière carte jouée

## Notes

Le jeu n'est pas dans son état optimal malgré la gestion des déconnexions, il peut arriver que dans certains cas, la terminaison du coté serveur ne se fasse pas proprement ou qu'il reste des fuites  mémoire.
Le client robot peut également se bloquer.

### Améliorations possibles
- Automatisation de la mise en route du jeu avec script shell
- Améliorer le fonctionnement du client robot pour qu'il soit plus "intelligent" et fonctionnel
- Réaliser une interface graphique pour plus de convivialité
- Rendre le code plus sécurisé en enlevant les variables globales en les encapsulant dans une structure notamment pour le client robot.
- Ajouter des fonctionnalités tel que le Shuriken présent dans le jeu physique, ajouter un timer...