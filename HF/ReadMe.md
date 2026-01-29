frequence d'envie des information: 1kbit/s 
donf Frequence de la porteuse: 10khz


```mermaid
---
config:
  layout: elk
---
stateDiagram
  direction TB
  state modulation {
    direction TB
    Mi1 --> V1
    V1 --> Me1
    Ol1 --> Me1
    Me1 --> PB1
    PB1 --> [*]
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
    Me2 --> PB2
    PB2 --> PLL
    PLL --> Mi2
    Me2
    Ol2
    PB2
    PLL
    Mi2
[*]  }
  modulation --> Démodulation
  Mi1:Microcontroleur
  V1:VCO
  Me1:mélengeur
  Ol1:oscilateur local
  PB1:filtre passe bande
  Me2:mélengeur
  Ol2:oscilateur local
  PB2:filtre passe bande
  Mi2:Microcontroleur

  ```