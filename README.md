# SAE 6 - Système d'alarme
Repo du projet **SAE 6 "système d'alarme"** réalisé dans le cadre de notre troisième année de BUT GEII à l'IUT Lyon 1.


## Concept initial
Notre projet consiste à développer un système d’alarme domestique complet, prêt à être installé sur une maison.
Les systèmes de surveillance actifs sont des détecteurs d’ouverture de portes et fenêtres ainsi qu’une caméra équipée de reconnaissance faciale à l’entrée.
L’interface utilisateur sera supportée par un M5Stack qui communiquera sans fil, en HF, avec le module central. À ce dernier sont reliés tous les différents composants (capteurs, reconnaissance faciale et alarme). L’interface permettra de verrouiller et déverrouiller le système d’alarme à l’aide d’un code à composer sur l’interface ou d’un badge RFID (à condition que la personne utilisant le badge soit reconnue par la reconnaissance faciale). Lorsqu’une intrusion est détectée, un module d’alarme est déclenché. 
Le module d'alimentation permet, en cas de coupure de courant, d'utiliser l'énergie stockée dans une batterie et de recharger cette dernière une fois le courant rétabli. Ce module d'alimentation est aussi responsable du déclenchement du mode dégradé permettant d'utiliser les modules critiques uniquement afin de réduire la consommation d'énergie. 
La communication étant critique, le système bascule en liaison BLE en cas de problème de communication en HF pour assurer une communication en permanence.

## Cahier des charges
### Cahier des charges initial :
- Détection d'ouverture d'une porte ou fenêtre
- Digicode pour déverrouiller l'alarme
- Reconnaissance faciale
- Alarme sonore
- Communication entre le digicode et la centrale en HF
- Protocole d'appairage entre le digicode et la centrale
- Détection d'erreur ou de signal anormal (sur les liaisons filaires)
### Améliorations possibles :
- Utilisation de badges RFID plutôt qu'un code, mais voir comment rendre cette solution sécurisée
- Communications en BLE si HF non fonctionnel
- Ajout d'une batterie et d'un mode dégradé en cas de coupure de courant
- Détecteur de mouvements
- Ajout d'un module GSM pour appeler le propriétaire et les forces de l'ordre ou un service de sécurité privée
- Ajout d'un bouton panique
- Empreinte biométrique
### Contraintes :
- Nécessité d'utiliser des capteurs à l'état haut par défaut pour détecter les failles
- Différenciation entre un capteur actif et un capteur absent ou endommagé


## Synoptique global initial
Au démarrage du projet, nous avions imaginé un fonctionnement bien précis pour notre système. Nous avons listé toutes les fonctionnalités que nous souhaitions implémenter et les avons reliées ensemble logiquement. L'ensemble du système tel que nous l'avions imaginé, sa décomposition en sous-systèmes et les interactions de ces derniers sont décrits ci-dessous :
[![](https://mermaid.ink/img/pako:eNqVVl1vokAU_SuTeWqz4opKUVKbtmhb0_oR6ma7G15GGJUUGDKMq67xv-8MCgysJa2JD3DuuWfu5dwLe-gQF0MDLnyycVaIMvBi2aEdAv5DPqLB7R7EKxRhAzgedXxcAz6aY98ANrwTOBgRd-1jcDEdmpc2BIcj1wncCqZJggCFbsYdaRI1QF6Yc925rwi-xB7xgPNUiuKoQtbi8BxTugNTLydFG5pz5sRnEmNKNpiCMWYbQt-PFKk3Shy9S1zX-6NQ7Mj81wijd0zjXO1IpNuctsJb-Yxv4CJp7KUsx9uprCWO74VlqR_DQvcVtv1YZPb2XeicHkPxsSlzxKSasI92EvMeMYbprsigC8-tPJv1MOyfnliRGYn-fqx2bL9MzC3ydW7G_FqFCeUT3RS2LNlYCVw_WAaSWkSJE8teTs4XgxEK0RIHOGQ2zMoUdlYcFFT21kQBpkjuDt4yxSWEqjkvJMxZlZl9HgPU_MQpr_kZXjNVlJ8JYNvG_Pa61xuOHybgGzCfBuZzr3eTVFLqpohFIvbpgUfevwxEXGrcM7FOOTadpPQAfI7FnydV6nXlRjJJEZ-f8IRfhJwTlFnzCJdNJ2JVITOd_BxYqdbZKCFmTsb94Ww4Gd-9gIxR6gi3JOD1JTDg7TWf7qzHpMxyGaXhSVQc-SwcKgzzB3mlGqXNBHq9vLdivRW3Fodn1vDxcWBlUcUVJcSeB79eZ9bkefB60ikuJXDGH4UgsU64kDnpDzJM8tj18YiZOf4fN5BHlEfpBB27XxoXIPnmlKk0GGcj0jyZlbgD9wCFXoCYR0IDLFDMpJcNN2Al7FTCanVytTq5WpE8mccU5bPP6BpL2LwCc85hsAaX1HOhIW7UIN9SvG_8Eu4Fz4ZsxfedDcVGcZF4u9rhgXMiFP4mJEhplKyXK2gskB_zq3Xkco2-h5YUBdldikMXU5OsQwYNVe90m0kaaOzhFhqK1mzU23q7q2taR-u2dK0Gd9Bo6lf1TrvV0lpNXW132u1DDf5NhNW6pnU77YbeVFW1oXWv9BrErscIHR0_lpJvpsM_bDXspg?type=png)](https://mermaid.live/edit#pako:eNqVVl1vokAU_SuTeWqz4opKUVKbtmhb0_oR6ma7G15GGJUUGDKMq67xv-8MCgysJa2JD3DuuWfu5dwLe-gQF0MDLnyycVaIMvBi2aEdAv5DPqLB7R7EKxRhAzgedXxcAz6aY98ANrwTOBgRd-1jcDEdmpc2BIcj1wncCqZJggCFbsYdaRI1QF6Yc925rwi-xB7xgPNUiuKoQtbi8BxTugNTLydFG5pz5sRnEmNKNpiCMWYbQt-PFKk3Shy9S1zX-6NQ7Mj81wijd0zjXO1IpNuctsJb-Yxv4CJp7KUsx9uprCWO74VlqR_DQvcVtv1YZPb2XeicHkPxsSlzxKSasI92EvMeMYbprsigC8-tPJv1MOyfnliRGYn-fqx2bL9MzC3ydW7G_FqFCeUT3RS2LNlYCVw_WAaSWkSJE8teTs4XgxEK0RIHOGQ2zMoUdlYcFFT21kQBpkjuDt4yxSWEqjkvJMxZlZl9HgPU_MQpr_kZXjNVlJ8JYNvG_Pa61xuOHybgGzCfBuZzr3eTVFLqpohFIvbpgUfevwxEXGrcM7FOOTadpPQAfI7FnydV6nXlRjJJEZ-f8IRfhJwTlFnzCJdNJ2JVITOd_BxYqdbZKCFmTsb94Ww4Gd-9gIxR6gi3JOD1JTDg7TWf7qzHpMxyGaXhSVQc-SwcKgzzB3mlGqXNBHq9vLdivRW3Fodn1vDxcWBlUcUVJcSeB79eZ9bkefB60ikuJXDGH4UgsU64kDnpDzJM8tj18YiZOf4fN5BHlEfpBB27XxoXIPnmlKk0GGcj0jyZlbgD9wCFXoCYR0IDLFDMpJcNN2Al7FTCanVytTq5WpE8mccU5bPP6BpL2LwCc85hsAaX1HOhIW7UIN9SvG_8Eu4Fz4ZsxfedDcVGcZF4u9rhgXMiFP4mJEhplKyXK2gskB_zq3Xkco2-h5YUBdldikMXU5OsQwYNVe90m0kaaOzhFhqK1mzU23q7q2taR-u2dK0Gd9Bo6lf1TrvV0lpNXW132u1DDf5NhNW6pnU77YbeVFW1oXWv9BrErscIHR0_lpJvpsM_bDXspg)


## Synoptique global final
À la fin du projet, certaines fonctionnalités n'ont pas pu être réalisées, que ce soit pour des raisons budgétaires ou que ces parties étaient trop chronophages. L'ensemble du système final, sa décomposition en sous-systèmes et les interactions de ces derniers sont décrits ci-dessous :
[![](https://mermaid.ink/img/pako:eNqNlV1v2jAUhv-K5atOI4x8AG1Upk6BtohRUGBaN-XGSQxETezIMYMO8d9nB0iclNEicZG87-NzfM6xs4MBDTG04SKmm2CFGAffXY94BIgfihFL7nYgW6EU2yCIWBDjBoiRj2MbePCb1MGYhusYg6vp0PnkQbA_sEESXiAdmiSIhAU7bitogiJSsqEfa5JX6LEwnEcZytILYV0h-5ixVzCNSijdsJLxacwVYko3mIEnzDeUvRwQpTZalr4obBwRjeFA5WcpRi-YZSoqSqOtt8oOoz917MewUkmNb5kCrPBW8c6fv7jP4OpY0moLNLaIwosJuvfD_rGWao6yBx-JKltRa52WhHGyTHhJpowGmdq_PFwGxoigJU4w4R4sIssWagFKLmbtoAQzpCaMt1wLKWV6yRHKg1Wd7AsP0MuMT5zxEc54y5nvcGPKMJDG7LhHtcKAb1v-3W2vN3y6n4DPwHkcOKNe72tehFojpBdJ7-O9dJym4owrkK65O3x4GLjSmo_qKawYdvkXYbVmUytEKVUMOhKG6eTnwJUuuX5V9lW5TLdqClSTyLhyeECvV2SXH6Tq-QBiE6PBr9ncnYwGs-OWqycCnKlcxSSnX4RxJv1BoSnVvz1kUBTv7QyD0lGfz6N02HptBsGhtupKtWl712GedZwiFWUWfdwBRKIE8YgSGyxQxpV7TXTxovwOHfxfzqfxpIrh52yNFc2_oAXnNNiASxaF0JYvGlAcb7Fz8Qh3kvMgX4mLwoPySIVIXsUe2QsmReQ3pckJY3S9XEF7geJMPK3TUMToR2jJUGnBJMTMoWvCoa23u_ka0N7BLbStblPvmkbLaOtWx7q2rAZ8laZO02iZht5pt42uaZnX-wb8m0dtNbtWW78xdat1Ywm9YzYgDiNO2fjwWc2_rvt_SVRXbA?type=png)](https://mermaid.live/edit#pako:eNqNlV1v2jAUhv-K5atOI4x8AG1Upk6BtohRUGBaN-XGSQxETezIMYMO8d9nB0iclNEicZG87-NzfM6xs4MBDTG04SKmm2CFGAffXY94BIgfihFL7nYgW6EU2yCIWBDjBoiRj2MbePCb1MGYhusYg6vp0PnkQbA_sEESXiAdmiSIhAU7bitogiJSsqEfa5JX6LEwnEcZytILYV0h-5ixVzCNSijdsJLxacwVYko3mIEnzDeUvRwQpTZalr4obBwRjeFA5WcpRi-YZSoqSqOtt8oOoz917MewUkmNb5kCrPBW8c6fv7jP4OpY0moLNLaIwosJuvfD_rGWao6yBx-JKltRa52WhHGyTHhJpowGmdq_PFwGxoigJU4w4R4sIssWagFKLmbtoAQzpCaMt1wLKWV6yRHKg1Wd7AsP0MuMT5zxEc54y5nvcGPKMJDG7LhHtcKAb1v-3W2vN3y6n4DPwHkcOKNe72tehFojpBdJ7-O9dJym4owrkK65O3x4GLjSmo_qKawYdvkXYbVmUytEKVUMOhKG6eTnwJUuuX5V9lW5TLdqClSTyLhyeECvV2SXH6Tq-QBiE6PBr9ncnYwGs-OWqycCnKlcxSSnX4RxJv1BoSnVvz1kUBTv7QyD0lGfz6N02HptBsGhtupKtWl712GedZwiFWUWfdwBRKIE8YgSGyxQxpV7TXTxovwOHfxfzqfxpIrh52yNFc2_oAXnNNiASxaF0JYvGlAcb7Fz8Qh3kvMgX4mLwoPySIVIXsUe2QsmReQ3pckJY3S9XEF7geJMPK3TUMToR2jJUGnBJMTMoWvCoa23u_ka0N7BLbStblPvmkbLaOtWx7q2rAZ8laZO02iZht5pt42uaZnX-wb8m0dtNbtWW78xdat1Ywm9YzYgDiNO2fjwWc2_rvt_SVRXbA)


## Organisation du repo
Chaque composant sera réparti dans des branches différentes de ce repo. Voici la liste des branches présentes :
- **main** : branche d'accueil du repo, laissée vide pour l'instant
- **alarm** : branche du module d'alarme (réalisée par Lilian CHARRIERE)
- **cmd** : branche du module de commande (ou "digicode", bien que cette appellation soit assez réductrice ; réalisée par Corentin DEBAISIEUX)
- **hf** : branche des transmissions HF et de la modulation du signal (réalisée par Gaspard DENIZOU)
- **hub** : branche du module principal (ou "hub" ; réalisée par Bastien PORTIER)
- **rasp** : branche du module de reconnaissance faciale (réalisée par Lilian CHARRIERE)


