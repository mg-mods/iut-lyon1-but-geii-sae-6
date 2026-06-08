# SAE 6 - Module Reconnaissance Faciale - Manuel utilisateur


## Première installation, enregistrement d'un nouveau profil facial
Afin de mettre en place le module de reconnaissance faciale, il est important de se munir des éléments suivants :
- Alarme domotique
- Écran avec connectique compatible Raspberry Pi 5 GPIO ou micro-HDMI
- Clavier et souris USB
Par la suite, connectez l'écran au module de reconnaissance facile (Raspberry Pi) présent à proximité du module principal de l'alarme. Connectez ensuite le clavier et la souris.
Avant de continuer, assurez-vous que vous n'avez pas débranché par erreur une autre connectique, en particulier celle de la caméra.
Démarrez le module de reconnaissance faciale. Si le module était déjà actif, il faut le redémarrer.
Une fois le système d'exploitation du module démarré, fermez les éventuelles fenêtres qui s'ouvrent automatiquement. Ouvrez une fenêtre de terminal. Entrez alors les lignes suivantes :
`cd ./facial_recognition`
`source ./init.sh`
`python3 ./headshots_capture-webcam.py [NOM_SUJET]` (remplacez `[NOM_SUJET]` par le nom du sujet en majuscule et sans caractères spéciaux)
Une nouvelle fenêtre s'ouvre. Positionnez le sujet devant la caméra et pressez la touche ESPACE du clavier pour capturer une image. Il faut au moins capturer les angles suivants, dans les conditions d'éclairages auquelles sera soumis le système. Pour des résultats optimaux, maintenez une distance de 30 à 60 cm de l'objectif.
- Face
- 3/4 gauche
- 3/4 droit
- 3/4 haut
- 3/4 bas
Il est possible de prendre plus de photos du sujet mais il est conseillé de garder un visage dégagé, une expression neutre et de ne pas surexposer le sujet.
Une fois les photos capturées, pressez la touche Q afin de fermer la fenêtre de capture.

Au lancement suivant, une fenêtre de visualisation de l'état du système s'ouvre automatiquement. Assurez-vous que le sujet est bien détecté par le système (présence d'un carré vert avec le nom du sujet autour de son visage). Si le sujet n'est pas détecté, se réferrer à la section **Résolution de problèmes**.
Si le sujet est correctement détecté, déconnectez le clavier, la souris et l'écran. Veillez à bien refermer le boitier.
Félicitations, le module de reconnaissance faciale a été correctement paramétré.


## Résolution de problèmes
### Sujet non-détecté
Dans le cas où un sujet n'est pas/plus détecté, il faut réenregistrer son profil facial. 
Avant cela, il est bonne pratique de vérifier la présence du dataset :

`dir ./facial_recognition/dataset`

Vérifiez la présence du profil recherché. Si il n'est pas présent, il faut enregistrer ce Si le dataset est présent mais non fonctionnel, il faut le réenregistrer. 
Pour se faire, il faut d'abord supprimer le dataset qui lui est associé puis réenregistrer son profil. Pour supprimer un profil du dataset, saisissez la commande suivante dans une fenêtre de terminal :
`rm ./facial_recognition/dataset/[NOM_PROFIL]`
Il suffit ensuite d'enregistrer de nouveau le profil (voir la section **Première installation, enregistrement d'un nouveau profil facial**).
### Suppression du profil d'un sujet
Pour supprimer un profil du dataset, saisissez la commande suivante dans une fenêtre de terminal :
`rm ./facial_recognition/dataset/[NOM_PROFIL]`
Une fois le profil supprimé du dataset, il est important de mettre à jour la base de données de profils :
`python3 ./model_training.py`
Cette opération peut prendre plusieurs minutes.
Une fois l'opération terminée, il est fortement recommandé de redémarrer le module de reconnaissance faciale.