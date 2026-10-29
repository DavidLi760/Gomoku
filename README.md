# Gomoku

Jeu C++20 / SFML 3, plateau 19 x 19.

## Regles appliquees

- Cinq pierres contigues ou plus gagnent, sauf si une capture adverse legale peut casser tous les alignements ou atteindre dix pierres capturees. Un cinq qui survit a la reponse adverse gagne.
- Une paire adverse encadree est capturee. Toutes les paires prises par le meme coup sont retirees. Dix pierres capturees gagnent.
- Creer deux trois libres sur des axes differents est interdit aux deux couleurs. Un trois libre peut devenir un quatre contigu ouvert aux deux extremites en un coup.
- Pas de regle de swap ni de restriction de premier coup.

## IA

Alpha-beta avec approfondissement iteratif, table de transposition indexee par profondeur restante, classement des victoires et defenses urgentes, evaluation des alignements et captures. Budget cible : 500 ms par coup; la generation des coups peut depasser legerement cette limite. Le dernier resultat complet est conserve. Compilation optimisee avec -O3.

## Verification

`make test` execute les tests tactiques sans bibliotheques SFML ni affichage (les en-tetes fournis restent necessaires). `make` compile le jeu avec les bibliotheques SFML 3 installees.

Les tests couvrent captures simultanees, cinq cassable, double trois, ouverture, victoires dans les quatre directions et les deux couleurs, blocage obligatoire et victoire par capture. Ils ne mesurent pas un taux de victoire contre des humains. L'objectif de plus de 80 % exige un panel de joueurs defini et des parties independantes, avec alternance des couleurs; aucun taux humain n'est garanti ici.
