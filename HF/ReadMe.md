frequence d'envie des information: 1kbit/s 
donf Frequence de la porteuse: 10khz


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
    PB2 --> Me1 :Tension=100mVpp
    PB1 --> Me1 :Tension =1 a 2 Vpp
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
    [*] --> Me2
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
  PB1:filtre passe Bas fc=13Mhz
  PB2:filtre passe Bas Fc=100khz ordre2
  Me1:mélangeur
  Me2:mélangeur
  Ol2:oscilateur local 13Mhz
  PB3:filtre passe bande centré sur 100kHz ordre2
  PB4:filtre passe bas Fc=20kHz ordre 2
  trig:Triger de schmitz

  ```