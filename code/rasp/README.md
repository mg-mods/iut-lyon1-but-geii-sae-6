# SAE 6 - Module Reconnaissance Faciale
Branche **Reconnaissance Faciale** du projet **SAE 6 "système d'alarme"** réalisé dans le cadre de notre troisième année de BUT GEII à l'IUT Lyon 1.


## Définitions
> La reconnaissance faciale est une technique qui permet à partir des traits de visage :
> - D’authentifier une personne : c’est-à-dire,  vérifier qu’une personne est bien celle qu’elle prétend être (dans le cadre d’un contrôle d’accès)
> - D’identifier une personne [...]
> En pratique, la reconnaissance peut être réalisée à partir d’images fixes (photos) ou animées (enregistrements vidéo) et se déroule en deux phases :
> - A partir de l’image, un modèle ou « gabarit » qui représente, d’un point de vue informatique, les caractéristiques de ce visage est réalisé.  [...]
> - La phase de reconnaissance est ensuite réalisée par la comparaison de ces modèles préalablement réalisés avec les modèles calculés en direct sur des visages présents sur l’image candidate.
> Dans le cas de l’authentification, le système vérifie si l'identité prétendue est bien la bonne en comparant le modèle du visage présenté au modèle préalablement enregistré correspondant à l’identité prétendue.
> [...]
> La reconnaissance faciale ne doit pas être confondue avec la détection de visage qui caractérise la présence ou non d’un visage dans une image indépendamment de la personne à qui il appartient.

-- CNIL


## Synotpique global
La reconnaissance faciale, telle qu'implémentée dans notre système, utilise différents composants logiciels et matériels dont voici un diagramme explicatif global :
[![](https://mermaid.ink/img/pako:eNp1kt-PojAQx_-Vps9qEDEgD5s0gsYcCwZZL7t4D71SkRy0pJTTXeP_fuWHcreb42U60898Z5jpFRKeUGjDY87P5ISFBF54YEB9BBdU4HjZmh9drOSVPF7irTKgFJzQqspY2l8eMaGCkniFSYZzoI48ZZnMOGvYVOCi6EleUkZ-x4Eyy30fSyQ5ktihkhLZSlX3C3EWVS5jR-CzEq3qXN6vKiIoZfGuNX0sLTMer7eboPdxfXmKOODyRAUgvCg5o6xV6DXqn6q18gT2WUI5yFhZy-5mGAIYjx0UIbCLQhc9j8dP_SA6jrLki9i2G84g1CUooaXnohCsQvTsNkL90AawDzQk2kYvoTuw3dgGtPMViXzkvb79RbbD_NJdG1V46CpdH4Tu7sWLWrwb8ad_cLKqzPH7UK_HmoKOA4K9G3roFSDfAc5mt23Ojwa6zfy3g80KfPOD7z5YoWXLN1v7VD6o5T-raBCVGgXrteeC7cbfqX2gqE1XS37UgiOYiiyBthQ1HUG1vwI3Lrw2yAGqh1DQA7TVMcHi1wEe2E3llJi9cV7c0wSv0xO0jzivlFeXCZbUyXDzih9RoapRseQ1k9C2ZlorAu0rvEB7PJ2bk5lm6AvNNKeGZcysEXyHtm5YE13TrdlcUxmaad1G8KOtO51oi4U-NS3d0M25YcxufwD9ax0W?type=png)](https://mermaid.live/edit#pako:eNp1kt-PojAQx_-Vps9qEDEgD5s0gsYcCwZZL7t4D71SkRy0pJTTXeP_fuWHcreb42U60898Z5jpFRKeUGjDY87P5ISFBF54YEB9BBdU4HjZmh9drOSVPF7irTKgFJzQqspY2l8eMaGCkniFSYZzoI48ZZnMOGvYVOCi6EleUkZ-x4Eyy30fSyQ5ktihkhLZSlX3C3EWVS5jR-CzEq3qXN6vKiIoZfGuNX0sLTMer7eboPdxfXmKOODyRAUgvCg5o6xV6DXqn6q18gT2WUI5yFhZy-5mGAIYjx0UIbCLQhc9j8dP_SA6jrLki9i2G84g1CUooaXnohCsQvTsNkL90AawDzQk2kYvoTuw3dgGtPMViXzkvb79RbbD_NJdG1V46CpdH4Tu7sWLWrwb8ad_cLKqzPH7UK_HmoKOA4K9G3roFSDfAc5mt23Ojwa6zfy3g80KfPOD7z5YoWXLN1v7VD6o5T-raBCVGgXrteeC7cbfqX2gqE1XS37UgiOYiiyBthQ1HUG1vwI3Lrw2yAGqh1DQA7TVMcHi1wEe2E3llJi9cV7c0wSv0xO0jzivlFeXCZbUyXDzih9RoapRseQ1k9C2ZlorAu0rvEB7PJ2bk5lm6AvNNKeGZcysEXyHtm5YE13TrdlcUxmaad1G8KOtO51oi4U-NS3d0M25YcxufwD9ax0W)


## Synoptique de l'analyse des images
[![](https://mermaid.ink/img/pako:eNplUkuPmzAQ_ivWnLpSiIAEEjisSgPbRs2GiOTSDT04YBK0vGQbddMo_722Qxa6y8GS_T1m5hsukNQpAReyov6TnDDlaBXFFRJfRnFJvl4QO-GGuCgr8kbjNB-hAh9I4aIYllXT8hsvBnS9ydJDr2lonRDGCBuINrTO8oIwlGKOD5gNpOSN030gDpxwkqIMC_Hvd6ih2T6GHm46oziuvigqkv1LjOaM5wl7iKFTJ3XZ0P1CnJiS7q3E7HUfEdYWPK-O6iqaPBPa4XXLxXT9LP-PHipUzXDrf5AZ0jRv7a1-vQSa9qiGusOsPRwpbk5okBzCFS7OLGc3yj0HYeIHu2CxQ9sf3ibYdlYihC6QKv3k2g04TEzYfA_WQeTtAh89eYsAbaLwablSralY7lsTzO0ujATN93beB3hQTb0ObNGzt_0p6TLBj6SuGFr6ckur0PNlG1H4jPxvD1J0S7nfiNCoLhdhGPnLtfDf9jQYwZHmKbictmQEJaEllle4SIMY-InIP1HuJ8X0NYa4ugpNg6uXui7vMlq3xxO4GS6YuLWN2CHxcywiLN9fqRiY0EXdVhzcqe3MlQu4F3gDVzPt6diZ6vZsMjfnhmmN4AzuZOKM56Y-0x3TnFuGZV9H8FeVNcamqU9mlm7YjmPZtmFd_wEDORa6?type=png)](https://mermaid.live/edit#pako:eNplUkuPmzAQ_ivWnLpSiIAEEjisSgPbRs2GiOTSDT04YBK0vGQbddMo_722Qxa6y8GS_T1m5hsukNQpAReyov6TnDDlaBXFFRJfRnFJvl4QO-GGuCgr8kbjNB-hAh9I4aIYllXT8hsvBnS9ydJDr2lonRDGCBuINrTO8oIwlGKOD5gNpOSN030gDpxwkqIMC_Hvd6ih2T6GHm46oziuvigqkv1LjOaM5wl7iKFTJ3XZ0P1CnJiS7q3E7HUfEdYWPK-O6iqaPBPa4XXLxXT9LP-PHipUzXDrf5AZ0jRv7a1-vQSa9qiGusOsPRwpbk5okBzCFS7OLGc3yj0HYeIHu2CxQ9sf3ibYdlYihC6QKv3k2g04TEzYfA_WQeTtAh89eYsAbaLwablSralY7lsTzO0ujATN93beB3hQTb0ObNGzt_0p6TLBj6SuGFr6ckur0PNlG1H4jPxvD1J0S7nfiNCoLhdhGPnLtfDf9jQYwZHmKbictmQEJaEllle4SIMY-InIP1HuJ8X0NYa4ugpNg6uXui7vMlq3xxO4GS6YuLWN2CHxcywiLN9fqRiY0EXdVhzcqe3MlQu4F3gDVzPt6diZ6vZsMjfnhmmN4AzuZOKM56Y-0x3TnFuGZV9H8FeVNcamqU9mlm7YjmPZtmFd_wEDORa6)


## Liste argumentée du matériel
Afin de mener à bien ce projet, le module de reconnaissance faciale a nécessité les composants matériels suivants :
- 1x Raspberry Pi 5
    > La Raspberry Pi 5 a été choisie pour sa simplicité d'utilisation, sa taille et sa puissance de calcul suffisante pour nos besoins. La Raspberry supporte en effet un environnement Linux qui permet de faire tourner le programme de reconnaissance faciale et permet facilement de paramétrer ce dernier.
- 1x webcam USB
    > Pour les besoins du projet, nous avons utilisé une webcam USB Logitech de modèle inconnu. Cette caméra, bien que d'assez mauvaise qualité, était suffisante pour réaliser notre batterie de tests tout en nous permettant d'avoir un rendu satisfaisant. Le gros avantage de cette caméra reste sa très faible consommation d'énergie par rapport à une caméra de meilleure qualité (ou un autre type de caméra, la webcam n'étant dépendante que de l'alimentation USB-A 1.0 de la Raspberry)

## Détail du travail réalisé
Après avoir déployé la dernière version compatible de Raspbian OS (distribution de Linux optimisée pour les Raspberry) sur notre carte et avoir réalisé les configurations système nécessaires (clavier, souris, mise à jour des bibliothèques, changement de quelques environnements), j'ai pu me concentrer sur la reconnaissance faciale. Le projet a pour base les bibliothèques OpenCV, ImUtils et face-recognition. OpenCV permet d'analyser des images (détection de visage, détection de formes et de motifs). ImUtils fournit un lot d'utilitaires permettant de réaliser des opérations sur des images (rotation, déplacement, redimensionnement). Enfin, face-recognition permet de reconnaître des formes et des motifs dans les visages afin de créer un profil (c'est-à-dire un lot de caractéristiques faciales à partir d'un ensemble d'images).
Le programme va ensuite utiliser ces bibliothèques selon la manière décrite par les synoptiques ci-dessus : 
- On récupère les images capturées par la caméra afin de leur appliquer différents filtres (exposition, luminosité, contraste, couleurs, netteté des contours, réduction de bruit, flous gaussiens, etc) pour améliorer la précision du programme
- On va ensuite prendre chaque image filtrée et utiliser OpenCV pour détecter le ou les visage(s) présent(s) dans l'image grâce à une série de motifs prédéfinis (yeux, bouche, dents, nez, sourcils, barbe/moustache, mâchoire, etc); on répètera les étapes suivantes pour chaque visage détecté
- On extrait le visage de l'image afin de mieux le traiter, indépendamment du reste de l'image
- On analyse le visage afin d'en extraire ses caractéristiques exactes (forme de la bouche, du nez, des yeux, des sourcils, etc) ; l'ensemble des caractéristiques d'un visage forme un profil
- On compare le profil perçu avec la base de données des profils existants (profils connus) : si le profil perçu correspond à un profil connu, alors on renvoie le nom du profil au programme principal, sinon on renvoie "Unknown"
- Que le profil soit connu ou non, l'emplacement du visage dans l'image est également renvoyé au programme principal
- Le programme principal récupère ces informations : si le visage est connu, alors il commute deux PINs du GPIO (l'un passe à l'état haut, l'autre à l'état bas)
- On utilise l'emplacement du visage dans l'image pour afficher un rectangle contenant le nom du profil (ou "Unknown") autour du visage détecté (si l'affichage est actif)
- On répète ces opérations tant que le programme tourne et pour chaque visage détecté
Comme mentionné, on utilise deux PINs du GPIO. Ces PINs servent à communiquer avec les autres composants. En temps normal, le PIN 13 (GPIO_27) est à l'état bas et le PIN 11 (GPIO_17) à l'état haut. Lorsqu'on détecte un visage, on inverse ces deux états. Utiliser deux PINs dont on inverse les états permet de détecter les erreurs de transmission et assure que l'information envoyée est bonne.
Il a ensuite fallu régler précisémment les filtres graphiques pour obtenir les meilleurs résultats possibles. Ces filtres se règlent à l'aide de variables présents dans les fichiers d'OpenCV et d'ImUtils.


## Améliorations possibles
Une des grandes limitations de ce module est sa précision. La webcam utilisée, bien que peu onéreuse et facile à mettre en place, a une très mauvaise qualité d'image ce qui ne facilite pas la reconnaissance faciale. Lors de tests effectués, remplacer les images du dataset (habituellement capturées à l'aide de la webcam) par des photos prises avec un téléphone améliorait déjà la précision des résultats. Bien que nous n'avons pas pu tester la capacité du programme avec une meilleure caméra, il semble logique de penser qu'une caméra de meilleure qualité (meilleure résolution, moins de bruit, meilleure gestion de la lumière et des contrastes, espace colorimétrique plus large) serait bénéfique pour le projet.
Le deuxième obstacle rencontré est lié à la performance de la Raspberry. Bien que suffisante dans notre contexte de prototypage, la détection d'un visage faisait drastiquement chuter le nombre d'images par secondes analysées par le programme (~35 IPS en l'absence de visage contre ~2 IPS lorsqu'un visage était détecté ; on peut chuter à 0.3 IPS lors de la détection de 3 visages en simultané). Une carte plus puissante pourrait permettre de résoudre ce problème, bien que nous aillons jugé les performances satisfaisantes dans notre contexte du fait que l'on pouvait tout de même déverouiller la porte.
Une autre limitation de la reconnaissance facile reste la facilité de déjouer un tel système. Il suffit en effet de montrer une photo de la personne pour que le système la reconnaisse et authorise l'accès. Cependant, coupler la reconnaissance faciale à d'autres capteurs biométrique devrait permettre de limiter ce genre d'attaques.

