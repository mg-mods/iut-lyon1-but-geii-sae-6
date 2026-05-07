

# Modulation Haute fréquence:

## diagrame  de fonctionnement:
| | Valeur |
| ------ | ------ |
| signal UART| 9600bps|
| fréquence porteurse| 100khz|
| fréquence d'emmission| 13,56Mhz|
| Puissance d'emmission| 10dbm|

```mermaid
---
config:
  theme: forest
  themeVariables:
    lineColor: '#FFA100'

---

stateDiagram
  direction TB
  state modulation {
    direction TB
    Mi1 --> V1
    Ol1 --> PB1
    V1 --> PB2
    PB2 --> Me1 :Tension=100mVpp P = 14DBm
    PB1 --> Me1 :Tension =1 a 2 Vpp P = 4-10DBm
    Me1 --> [*]
    Mi1
    V1
    Ol1
    PB1
    PB2
    Me1
  }
  state Démodulation {
    direction TB
    [*] --> G
    G-->PD
    PD --> Me2
    Ol2 --> Me2
    Me2 --> PB3
    PB3 --> PLL
    PLL --> PB4
    PB4 -->trig
     trig -->Mi2
[*]    Me2
    Ol2
    PB3
    PLL
    Mi2
  }
  modulation --> Démodulation
  Mi1:Microcontroleur
  V1:VCO
  Ol1:oscilateur local 13Mhz
  PB1:filtre passe Bas fc=13,56Mhz
  PB2:filtre passe Bas Fc=100khz ordre2
  Me1:mélangeur
  Me2:mélangeur
  Ol2:oscilateur local 13Mhz
  PB3:filtre passe bande centré sur 100kHz ordre2
  PB4:filtre passe bas Fc=20kHz ordre 2
  trig:Triger de schmitz
  PD:Pont diode
  G:Gain 10DBm
  ```
le gain de la partie démodulation se fait avec un AOP transimpédence si à la sortie de l'antenne on doit ampliffier.
le pont diviseur sert à bien avoir la bonne puissance en entré du mélangeur.

## Simulation LTspice
![Schémas electrique de la modulation et démodulation sur LTspice](.\documentation\images\Schémas LTspice.jpg "Titre de l'image")

le VCO, la pll et les mélangeur n'étans pas repertorier sur LTspice sont simuler avec des lignes de commande.
| Composant| Ligne de commande |
| ------ | ------ |
| VCO modulation | Bvco vcomod 0 V = 2.5 + 2.5*sgn(sin(2*pi*idt( {Fp}*(1 + 0.05*(V(microout) - 0.5)) )))|
| Melangeur Modulation| bvmel1 mel 0 V = V(filtre_100k)*V(filtre_13M)|
| VCO démodulation | Bvcodemod Vco 0 V = 2.5 + 2.5*sgn(sin(2*pi*idt( {Fp}*(1 + 0.1*(V(Demodul) - 0.5)))))|
| Mélangeur Démodulation| bvmel2 mel2  0 V = V(mel)*V(oscilateur)|

### Paramètre de simulation:
| Paramètre | valeur | 
| ------ | ------ |
|Tsim | 5ms|
|F | 9600hz|
|F13| 13 560 000hz|
|Fp| 100 000hz|
