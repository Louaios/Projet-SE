# MyShell Project (Projet SE)

Un mini-shell Unix complet en C avec gestion des processus, redirections, et variables.

## Structure du Projet

```
Projet SE/
├── Makefile            # Fichier de compilation
├── mysh.h             # En-têtes principales
├── mysh.c             # Programme principal
├── parser.c           # Parseur de commandes
├── executor.c         # Exécuteur de commandes
├── builtins.c         # Commandes internes
├── wildcards.c        # Expansion des wildcards
├── redirections.c     # Gestion des redirections
├── jobs.c             # Gestion des jobs en arrière-plan
├── variables.c        # Gestion des variables (locales et d'environnement)
├── signals.c          # Gestion des signaux
├── utils.c            # Fonctions utilitaires
├── myls.c             # Commande externe myls
└── myps.c             # Commande externe myps
```

## Compilation

```bash
make
```

Cela génère trois exécutables :
- `mysh` : le mini-shell
- `myls` : équivalent de `ls -l` avec options `-a` et `-R`
- `myps` : équivalent de `ps aux` avec couleurs

## Fonctionnalités

### 1. Lancement de Commandes

Le shell affiche une invite avec le répertoire courant (~ pour home directory) :
```
~/Desktop> ls
~/Desktop> cat file.txt
```

### 2. Séquencement

- **`;`** : Exécution inconditionnelle
  ```
  ~/> ls ; cat /etc/passwd
  ```

- **`&&`** : Exécution conditionnelle (si succès)
  ```
  ~/> gcc -o mysh myshell.c && ./mysh
  ```

- **`||`** : Exécution conditionnelle (si échec)
  ```
  ~/> test -d .can || mkdir .can
  ```

### 3. Wildcards

- **`*`** : Suite quelconque de caractères
- **`?`** : Exactement un caractère
- **`[ens]`** : Ensemble de caractères
- **`[^ens]`** : Exclusion de caractères
- **`\`** : Échappement

Exemples :
```
~/> cat *.[ch]
~/> ls ../[A-Z.]*[^~]
~/> wc -l /etc/?????
```

### 4. Commandes Internes

#### `cd [répertoire]`
Change de répertoire. Sans argument, va vers HOME.

#### `exit`
Quitte le shell sans tuer les jobs en arrière-plan.

#### `status`
Affiche le code de retour du dernier processus en foreground.
```
xxxx terminé avec comme code de retour YYY
xxxx terminé anormalement
```

#### `set [var=valeur]`
- Avec argument : définit une variable locale
- Sans argument : affiche toutes les variables locales

#### `unset variable`
Supprime une variable locale.

#### `setenv [var=valeur]`
- Avec argument : définit une variable d'environnement (dans mémoire partagée)
- Sans argument : affiche toutes les variables d'environnement

#### `unsetenv variable`
Supprime une variable d'environnement.

#### `myjobs`
Liste les jobs en arrière-plan :
```
[xxx] yyy État zzz
```

#### `myfg [job_id]`
Passe un job en foreground.

#### `mybg [job_id]`
Passe un job stoppé en background.

### 5. Redirections

- **`>`** : Redirige stdout (écrase)
- **`>>`** : Redirige stdout (ajoute)
- **`2>`** : Redirige stderr (écrase)
- **`2>>`** : Redirige stderr (ajoute)
- **`>&`** : Redirige stdout et stderr (écrase)
- **`>>&`** : Redirige stdout et stderr (ajoute)
- **`<`** : Redirige stdin
- **`|`** : Pipeline

Exemples :
```
~/> find . -type f -name \*.mp3 >> listofsongs
~/> nl < myshell.c
~/> ls | sort -r
~/> ps | grep mysh | wc -l
```

### 6. Background / Foreground

#### Lancer en arrière-plan
```
~/> emacs &
[1] 12345
```

#### Stopper avec Ctrl-Z
Arrête le processus foreground et le convertit en job :
```
^Z
[1] 12345 Stoppé emacs
```

#### Termination notification
```
emacs (jobs=[1], pid=12345) terminée avec status=0
```

### 7. Variables

Les variables locales sont propres à chaque shell. Les variables d'environnement sont partagées entre tous les exemplaires de mysh via mémoire partagée POSIX.

```bash
~/> set a=foo
~/> echo $a
foo
~/> setenv b=bar
~/> mysh         # Lance un nouveau shell
~/> echo $b      # Variable d'environnement accessible
bar
~/> set b=tmp    # Variable locale prioritaire
~/> echo $b
tmp
~/> exit
~/> echo $b      # Retour à la variable d'environnement
bar
```

### 8. Signaux

- **Ctrl-C** : 
  - Si processus en foreground : propage le signal
  - Sinon : demande confirmation et tue tous les jobs en background
  
- **Ctrl-Z** : Stoppe le processus foreground et le convertit en job

### 9. Commandes Externes

#### `myls [options] [paths...]`
Équivalent de `ls -l` avec couleurs :
- `-a` : Affiche les fichiers cachés
- `-R` : Récursif

Couleurs par type de fichier :
- Bleu : répertoires
- Vert : exécutables
- Cyan : liens symboliques
- Jaune : FIFO, périphériques
- Magenta : sockets

#### `myps`
Équivalent de `ps aux` avec couleurs par état :
- Vert : Running (R)
- Bleu : Sleeping (S, D)
- Jaune : Stopped (T, t)
- Rouge : Zombie (Z, X)

## Caractéristiques Techniques

### Gestion de la Mémoire Partagée
Les variables d'environnement utilisent :
- **Mémoire partagée POSIX** (shmget/shmat)
- **Sémaphores POSIX** pour synchronisation
- **Priorité aux écrivains** (readers-writers problem)
- **Compteur de références** pour destruction automatique

### Gestion des Processus
- Fork/exec pour exécution des commandes
- Wait/waitpid pour synchronisation
- Groupes de processus pour job control
- Signaux (SIGCHLD, SIGINT, SIGTSTP, SIGCONT)

### Parsing
- Tokenization avec gestion des guillemets et échappements
- Expansion des variables avant parsing
- Expansion des wildcards avec glob()
- Support des opérateurs composés (&&, ||, >>, etc.)

## Compilation et Exécution

```bash
# Compilation
make

# Nettoyage
make clean

# Exécution
./mysh

# Test des commandes externes
./myls -aR /tmp
./myps
```

## Exemples d'Utilisation

```bash
# Lancer le shell
./mysh

# Commandes simples
~/> ls -la
~/> cat /etc/passwd | grep root

# Séquencement
~/> mkdir test && cd test && touch file.txt
~/> rm nonexistent || echo "File not found"

# Wildcards
~/> ls *.c
~/> cat test[123].txt
~/> echo /etc/????

# Redirections
~/> ls > output.txt
~/> cat < input.txt
~/> find / -name "*.log" 2> errors.txt

# Background jobs
~/> sleep 100 &
[1] 12345
~/> myjobs
[1] 12345 En cours d'exécution sleep 100
~/> myfg 1

# Variables
~/> set PATH=/usr/bin:/bin
~/> setenv HOME=/home/user
~/> echo $PATH
~/> unset PATH
```

## Limitations Connues

- Taille maximale de ligne : 4096 caractères
- Nombre maximum d'arguments : 256
- Nombre maximum de jobs : 100
- Taille mémoire partagée : 64KB

## Auteur

Projet SE - Système d'Exploitation
Université d'Artois - Faculté des Sciences Jean Perrin
