# Procédure de test — Liaison radio PC ↔ Robot

Cette documentation décrit les étapes à suivre pour tester la liaison radio
169.4 MHz entre la télécommande connectée au PC et le robot.

## 1. Architecture du test

```
[PC]  ──UART framé──>  [Modem REMOTE]  ──RF 169.4 MHz──>  [Modem ROBOT]  ──UART transparent──>  [PC robot]
 │                        (PIC18F26Q10                       (PIC18F26Q10
 │                         + CC1120)                          + CC1120)
 │
 └─ App PC (gère le protocole framé : SCAN / CONNECT / DATA)

                                                              │
                                                              └─ Terminal série brut (PuTTY, minicom...)
```

Les deux liaisons UART **ne sont pas symétriques** :

| Liaison | Type | Outil PC compatible |
|---|---|---|
| PC ↔ Modem REMOTE | Framée (`AA 55 LEN TYPE PAYLOAD CRC8`) | App PC dédiée |
| Modem ROBOT ↔ PC robot | Transparente (octets bruts) | Terminal série standard |

## 2. Prérequis matériel

- 2 cartes PIC18F26Q10 + CC1120 (une "télécommande", une "robot")
- 1 PICkit 4 (ou équivalent compatible MPLAB)
- 2 PC (ou 1 PC avec 2 ports série) :
  - PC télécommande : exécute l'app PC
  - PC robot : ouvre un terminal série pour observer/taper du texte
- 2 câbles UART vers USB (USB-TTL) — 115200 8N1
- Antennes 169.4 MHz sur les deux modems

## 3. Prérequis logiciel

- **MPLAB X IDE** (v6.x ou plus)
- **Compilateur XC8 v3.10**
- App PC qui parle le protocole framé (déjà développée)
- Terminal série côté robot : PuTTY, Tera Term, minicom, ou équivalent

## 3 bis. Configuration radio

**Aucune configuration logicielle à faire** : la table des registres CC1120 est
déjà écrite dans [firmware/modem/radio/cc1120.c](firmware/modem/radio/cc1120.c).
Init automatique au démarrage via `cc1120_init_minimal()`.

Paramètres déjà figés :

| Paramètre | Valeur |
|---|---|
| Fréquence | 169.4 MHz |
| Modulation | 2-GFSK |
| Débit symboles | ~4.8 kbps |
| Déviation | 5 kHz |
| Bande passante RX | 100 kHz |
| Sync word | 0x930B51DE |
| Puissance PA | ~+14 dBm (25 mW) |

Vérifications **matérielles** à faire avant le test :

- **Antennes 169.4 MHz** sur les deux modems (pas des antennes 868/433 MHz).
- **Quartz 32 MHz** présent sur les pins XOSC_Q1/Q2 du CC1120 (le PIC, lui,
  utilise son oscillateur interne).
- **Câblage SPI** PIC ↔ CC1120 fonctionnel : si `cc1120_init_minimal()` échoue,
  l'app PC ne recevra pas la trame `LOG "REMOTE modem ready"`.
- **Conformité réglementaire** : en Europe, la bande 169 MHz est sous régime
  ETSI (puissance et duty-cycle limités selon la sous-bande). Pour un test labo
  rapide, OK ; pour un déploiement, à valider.

Pour ré-générer la table de registres avec d'autres paramètres (débit plus
élevé, AGC différente, etc.), utiliser **SmartRF Studio** (outil gratuit TI),
exporter le profil, et remplacer les valeurs dans `cc1120_std_regs_[]` /
`cc1120_ext_regs_[]` de `cc1120.c`.

## 4. Compilation et flash des firmwares

### 4.1 Modem télécommande (`Modem_remote.X`)

1. Ouvrir le projet [firmware/Modem_remote.X/](firmware/Modem_remote.X/) dans MPLAB X.
2. Vérifier dans **Project Properties → XC8 Compiler → Preprocessing and messages** :
   - **Define macros** : `MODEM_ROLE=1` (ou laisser vide, c'est la valeur par défaut)
   - **Include directories** : laisser vide (les `#include` sont relatifs)
3. Vérifier que tous les fichiers source de [firmware/modem/](firmware/modem/) sont
   bien présents dans le panneau *Source Files* / *Header Files* du projet.
4. Brancher le PICkit 4 sur la carte télécommande.
5. **Clean and Build** → **Make and Program Device**.

### 4.2 Modem robot (`Modem_robot.X`)

1. Ouvrir le projet [firmware/Modem_robot.X/](firmware/Modem_robot.X/) dans MPLAB X.
2. Vérifier dans **Project Properties → XC8 Compiler → Preprocessing and messages** :
   - **Define macros** : `MODEM_ROLE=2` *(critique — sans ça la carte se comportera comme une télécommande)*
   - **Include directories** : vide
3. Brancher le PICkit 4 sur la carte robot.
4. **Clean and Build** → **Make and Program Device**.

> ⚠️ Bien penser à débrancher la carte télécommande avant de programmer la carte
> robot, et inversement, pour éviter de flasher la mauvaise carte.

## 5. Câblage des deux postes

### Poste télécommande (PC principal)

```
[PC] ──USB──> [Câble USB-TTL] ──UART──> [Carte télécommande]
```

- TX du PC → RX de la carte
- RX du PC ← TX de la carte
- GND commun
- Alimentation de la carte (via USB-TTL ou alim séparée)

### Poste robot (autre PC)

```
[PC robot] ──USB──> [Câble USB-TTL] ──UART──> [Carte robot]
```

Même câblage que ci-dessus.

## 6. Procédure de test

### 6.1 Allumage et vérification

1. Mettre les deux cartes sous tension.
2. Sur **PC robot** : ouvrir un terminal série sur le port correspondant à la carte robot.
   - Vitesse : **115200 baud**
   - Format : **8N1** (8 bits, sans parité, 1 stop bit)
   - Pas de contrôle de flux
3. Sur **PC principal** : lancer l'app PC, sélectionner le port série de la carte télécommande.

### 6.2 Établissement de la liaison

Le scan est **automatique** dès le lancement de la TUI (le firmware émet en
continu des `SCAN_RESULT` à chaque beacon reçu). Tous les robots dans la portée
sont listés dans le panneau **Discovered Robots**, avec leur RSSI en dBm.

| Étape | Action côté PC | Trame envoyée | Réponse attendue |
|---|---|---|---|
| 1 | Lancer la TUI | — | Le robot apparaît dans Discovered Robots dans la seconde |
| 2 | `connect 0x10` (ID du robot) | `CONNECT` (0x03) | `CONNECTED` (0x82) puis STATS qui remontent en continu |
| 3 | `send "hello"` | `DATA_TX` (0x10) | Le texte apparaît dans le terminal robot **et** `TX_ACK` (0x85) confirme l'arrivée |
| 4 | Taper côté terminal robot | (octets bruts) | `DATA_RX` (0x90) reçu par l'app PC |

### 6.3 Test bidirectionnel

- Taper du texte dans l'app PC → doit apparaître dans le terminal du PC robot.
- Taper du texte dans le terminal du PC robot → doit apparaître dans l'app PC.
- Vérifier que les statistiques RSSI / PER / RTT du panneau Connection
  s'actualisent automatiquement (le firmware push `STATS` 0x84 toutes les 500 ms).

### 6.4 Test de robustesse

- Couper l'alim de la carte robot pendant la session :
  l'app PC doit recevoir `DISCONNECTED reason=0x03` (LINK_TIMEOUT) après
  6 secondes (`LINK_LOST_TIMEOUT_MS` dans [firmware/modem/config.h](firmware/modem/config.h)).
- Rallumer le robot, refaire un `connect <id>`.
- Tester la portée en éloignant les deux cartes. À RSSI ~-95 dBm la liaison
  devient marginale, à ~-105 dBm elle commence à lâcher.

## 7. Diagnostic en cas de problème

### Aucun robot détecté dans Discovered Robots

- Vérifier que la carte robot a bien été flashée avec `MODEM_ROLE=2`.
- Vérifier les antennes 169.4 MHz sur les deux modems.
- Rapprocher les deux cartes (< 1 m) pour exclure un problème de portée.
- Vérifier qu'aucune des deux cartes n'est en reset (LED d'alim / log boot).
- Vérifier que la LED du robot clignote (toggle à chaque beacon TX réussi).

### `CONNECTED` reçu mais aucune donnée ne passe

- Vérifier le câblage UART côté robot (TX/RX éventuellement croisés).
- Vérifier la vitesse du terminal côté robot : doit être **115200**.
- Vérifier que l'app PC envoie bien des trames `DATA_TX` (type 0x10) avec un CRC8 valide.

### L'app PC ne reçoit rien

- Vérifier le port COM sélectionné côté PC.
- Vérifier la vitesse UART : **115200 8N1**.
- Activer un mode debug dans l'app PC pour afficher les octets bruts reçus —
  on doit voir au minimum les trames `LOG` (0x9F) au boot et les `STATS` (0x84) périodiques.

### Erreurs de compilation MPLAB

- Erreur *"cannot open source file"* : ajouter `../modem` dans
  **Project Properties → XC8 Compiler → Include directories**.
- Erreur *"MODEM_ROLE must be ..."* : la macro `MODEM_ROLE` n'est pas définie
  ou a une valeur invalide → vérifier **Define macros**.

## 8. Référence des trames PC ↔ Modem REMOTE

Format : `[0xAA] [0x55] [LEN] [TYPE] [PAYLOAD...] [CRC8]`
(LEN = nombre d'octets de TYPE jusqu'à la fin du PAYLOAD inclus,
CRC8 calculé sur `LEN || TYPE || PAYLOAD`)

### PC → REMOTE

| Type | Hex | Payload | Description |
|---|---|---|---|
| `SCAN_START` | 0x01 | — | (Conservé pour compatibilité, scan actif par défaut) |
| `SCAN_STOP` | 0x02 | — | (Conservé pour compatibilité) |
| `CONNECT` | 0x03 | `robot_id` (1 octet) | Connexion à un robot |
| `DISCONNECT` | 0x04 | — | Déconnexion |
| `GET_STATS` | 0x05 | — | (Conservé pour compatibilité, STATS push automatique) |
| `DATA_TX` | 0x10 | données utilisateur | Données à tunneliser vers le robot |

### REMOTE → PC

| Type | Hex | Payload | Description |
|---|---|---|---|
| `SCAN_RESULT` | 0x81 | `robot_id`, `rssi`, `age_100ms` | Beacon de robot détecté |
| `CONNECTED` | 0x82 | `robot_id` | Liaison établie |
| `DISCONNECTED` | 0x83 | `reason` | Liaison perdue / fermée |
| `STATS` | 0x84 | `rssi_avg`, `per_pct`, `rtt_ms` (LE) | Stats push 500 ms |
| `TX_ACK` | 0x85 | — | DATA précédent confirmé reçu par le robot |
| `DATA_RX` | 0x90 | données | Données reçues du robot |
| `LOG` | 0x9F | texte ASCII | Message de debug |

`DISCONNECTED.reason` :

| Code | Hex | Signification |
|---|---|---|
| `USER` | 0x01 | Déconnexion demandée par l'utilisateur |
| `REMOTE` | 0x02 | Peer a fermé la session (DISCONNECT RF reçu) |
| `LINK_TIMEOUT` | 0x03 | 6 s sans aucun paquet du robot connecté |
| `CONNECT_TIMEOUT` | 0x04 | Pas de CONNECT_OK reçu dans la fenêtre 1.5 s |
| `ARQ_FAILED` | 0x05 | 6 retries DATA épuisés sans ACK |
| `RF` | 0x06 | DISCONNECT RF reçu |

Définitions complètes dans [firmware/modem/protocol/protocol.h](firmware/modem/protocol/protocol.h).
