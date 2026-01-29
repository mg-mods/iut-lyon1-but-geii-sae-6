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
    V1 --> PB2
    Ol1 --> PB1
    Me1 --> [*]
    Mi1
    V1
    Me1
    Ol1
    PB1
[*]  }
  state Démodulation {
    direction TB
    [*] --> Me2
    Ol2 --> Me2
    Me2 --> PB3
    PB2 --> PLL
    PLL --> Mi2
    Me2
    Ol2
    PB2
    PLL
    Mi2
[*]  }
  modulation ==> Démodulation
  Mi1:Microcontroleur
  V1:VCO
  Me1:mélangeur
  Ol1:oscilateur local
  PB1:filtre passe Bas
  PB2:filtre passe Bas
  Me2:mélangeur
  Ol2:oscilateur local
  PB2:filtre passe bande
  Mi2:Microcontroleur

  ```