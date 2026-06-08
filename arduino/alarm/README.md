# SAE 6 - Module Alarme Sonore
Branche **Alarme** du projet **SAE 6 "système d'alarme"** réalisé dans le cadre de notre troisième année de BUT GEII à l'IUT Lyon 1.


## Définitions
> Une alarme sonore anti-intrusion est un équipement conçu pour émettre un signal sonore puissant en cas de tentative d'effraction. 
> Ce signal, souvent supérieur à 100 décibels, est suffisamment intense pour désorienter les intrus, alerter les occupants des lieux et prévenir le voisinage.
> [...]
> Une fois l’anomalie confirmée, la sirène se déclenche instantanément, produisant un bruit intense audible à longue distance. 
> [...]
> Le bruit intense pousse les cambrioleurs à fuir avant qu’ils ne puissent agir. C’est une réaction instinctive face à un environnement hostile.Elle prévient les occupants des lieux et le voisinage d’une tentative d’effraction, permettant une réaction rapide.

-- Verisure


## Synotpique global
L'alarme sonore, telle qu'implémentée dans notre système, utilise différents composants logiciels et matériels dont voici un diagramme explicatif global :
[![](https://mermaid.ink/img/pako:eNp9VN9v2jAQ_ldOftokqCAtJeRhWgdpa4kGRLJVWtMH1zlC1GBnttOtQ_zvu5BSKA_LU3y678d9vmTDpM6QBWxZ6t9yJYyD6SJVQE-hqtp93YBdiQoDWJZF1XWm6EApnrAMIGW86YBMOJEy2LYopxXGWHL1X2RCXWCxROnSVO2UPlLYaWGPxCujJVqL9oTDglAZyFdZ0mtJkAOLrt0H_9lT2ZWFkUcMcYXiGQ1IYjoA5Qrl87xQ9mHcvMGcRxasEw7t4z4Z9yLKh5SN9Zo0sCmgoZKlYT6N53c0Ak0nP6fsDVGZpcFfD3Ojs1rS5EWuRPm411N5MwvJCZXvY8FslwP1tF1HdwLdrte4irvdLwe3J-apiV_Dt1lyC1eLEHj0I1wk4aSBtPZPsqb-mzCBOJyGY-qDW35DyGgC09k9JLMojHen5DbkC6ART4ls_ZQbUa0g1jVdSY4KjXCFVlDqvJBt0yE8krvm0dX0jfqek8_J98VVwmdRQ90G1qJQZccpEjTmN4Rt-tpb3of0buJovz7ov29nGw-FCDxuRmyjbC_iRHVfJkgU3h8SapwDP0qUdVhuiowFztTYYWs0a9Ec2aYhSplb4Zr2rNm8TJjnlKVqS5hKqJ9ar_cwo-t8xYIlbROd6oo-L5wUguZav1cNmUMzpqQdC879Xm_HwoIN-8MCf3DmUc33fH8wvPD6ow57ZUH_vHd23vcG_YE38If-qHe57bC_O93emT-8GI1GvcHoYjj0Lvt-h2FWOG3u2n_D7hex_Qd3qVDz?type=png)](https://mermaid.live/edit#pako:eNp9VN9v2jAQ_ldOftokqCAtJeRhWgdpa4kGRLJVWtMH1zlC1GBnttOtQ_zvu5BSKA_LU3y678d9vmTDpM6QBWxZ6t9yJYyD6SJVQE-hqtp93YBdiQoDWJZF1XWm6EApnrAMIGW86YBMOJEy2LYopxXGWHL1X2RCXWCxROnSVO2UPlLYaWGPxCujJVqL9oTDglAZyFdZ0mtJkAOLrt0H_9lT2ZWFkUcMcYXiGQ1IYjoA5Qrl87xQ9mHcvMGcRxasEw7t4z4Z9yLKh5SN9Zo0sCmgoZKlYT6N53c0Ak0nP6fsDVGZpcFfD3Ojs1rS5EWuRPm411N5MwvJCZXvY8FslwP1tF1HdwLdrte4irvdLwe3J-apiV_Dt1lyC1eLEHj0I1wk4aSBtPZPsqb-mzCBOJyGY-qDW35DyGgC09k9JLMojHen5DbkC6ART4ls_ZQbUa0g1jVdSY4KjXCFVlDqvJBt0yE8krvm0dX0jfqek8_J98VVwmdRQ90G1qJQZccpEjTmN4Rt-tpb3of0buJovz7ov29nGw-FCDxuRmyjbC_iRHVfJkgU3h8SapwDP0qUdVhuiowFztTYYWs0a9Ec2aYhSplb4Zr2rNm8TJjnlKVqS5hKqJ9ar_cwo-t8xYIlbROd6oo-L5wUguZav1cNmUMzpqQdC879Xm_HwoIN-8MCf3DmUc33fH8wvPD6ow57ZUH_vHd23vcG_YE38If-qHe57bC_O93emT-8GI1GvcHoYjj0Lvt-h2FWOG3u2n_D7hex_Qd3qVDz)


## Liste argumentée du matériel
Afin de mener à bien ce projet, le module d'alarme sonore a nécessité les composants matériels suivants :
- 1x Arduino UNO
    > L'Arduino UNO a été choisie pour sa simplicité d'utilisation. Un simple branchement et un petit code suffisent pour réaliser l'intégralité du module. De plus, l'Arduino UNO est idéale pour des besoins de prototypage.
- 1x haut-parleur à compression RUP5
    > Un haut-parleur à compression est un actionneur auditif très simple mais robuste. Piloté par simplement deux fils d'alimentation, ce petit haut-parleur est capable de fonctionner entre 0.01 et 13V (bien que nos tests ont montré par la suite qu'il était capable de monter à 16V) et de produire un son à plus de 100 dB. Dans notre cas, le relier à une Arduino UNO (tension de sortie max sur PWM: 5V) serait donc suffisant pour obtenir un son tonique. Il nous suffit de faire varier la tension de son alimentation pour générer un signal sonore.

## Détail du travail réalisé
Après avoir câblé le haut-parleur à un PWM de la carte, il m'a suffit de rédiger un petit programme qui génère un signal à partir d'une fréquence donnée. Le programme a ensuite évolué pour n'activer la sortie sonore que lorsque l'entrée est dans un certain état. J'ai par la suite ajouté la possibilité d'enregistrer plusieurs sons alarmes deux-tons avec des cycles différents puis de rajouter un bouton pour changer de son. Le programme fonctionne ensuite de la manière décrite par le synoptique ci-dessus :
- On scrute les entrées : si l'entrée haute passe à l'état bas et que l'entrée basse passe à l'état haut, alors la centrale nous commande d'activer l'alarme
- On charge les tons hauts, tons bas et les cycles stockés en mémoire
- On converti les cycles (en cycles par minute, CPM) en intervals (exemple : 60 CPM <=> interval de 1 sec entre chaque cycle <=> 0.5 sec entre chaque ton)
- On génère le premier ton
- Lorsque l'on arrive au bout de l'interval, on génère le deuxième ton
- Lorsque l'on arrive au bout de l'interval, on repasse au premier ton
- On continue à alterner entre ton haut et ton bas jusqu'à qu'une des entrées repasse à son état par défaut
En parallèle, une deuxième logique tourne :
- On scrute l'entrée de sélection des tonalités
- Si on détecte une impulsion à la masse, alors on change de ton
- On change l'identifiant de tonalité sélectionné
- On met à jour les intervals à partir du cycle de la nouvelle tonalité
- On force l'utilisation des nouveaux tons haut, tons bas et intervals
On répète l'ensemble de ces opérations tant que la carte est active.
Comme mentionné, on utilise deux PINs du GPIO pour détecter si la centrale commande à l'alarme de s'activer. En temps normal, le PIN 13 est à l'état bas et le PIN 12 à l'état haut. Lorsque la centrale nous déclenche, ces deux états sont inversés. Utiliser deux PINs dont on inverse les états permet de détecter les erreurs de transmission et assure que l'information envoyée est bonne.
On utilise également un troisième PIN du GPIO, le PIN 7, pour changer la tonalité sélectionnée. Utiliser une impulsion à l'état bas permet également de détecter des erreurs et d'éviter une activation accidentelle.


## Améliorations possibles
Une des grandes limitations de ce module est sa puissance. Le module est encore au stade de prototype et le système alimente le haut-parleur en 5 V ce qui lui donne une puissance sonore de sortie estimée à 75 dB. Pour augmenter cette puissance, il faudrait utiliser une source d'alimentation externe pilotée par le PWM de l'Arduino pour obtenir les bonnes tonalités. 
De plus, il suffit actuellement de couper l'alimentation de l'Arduino pour neutraliser l'alarme. Ce module sert plus de moyen de démonstration pour prouver que le reste du système fonctionne, mais il présente déjà du potentiel et de bonnes pistes ont été abordées. Cependant, il reste loin d'être un produit finit.

