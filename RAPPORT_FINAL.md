# Rapport final — Liaison radio PC ↔ Robot @ 169.4 MHz

> Projet semestriel — EPFL BioRob 2026
> Justin Décaillet

## 1. Synthèse

Conception et mise en service de bout en bout d'une liaison radio sub-GHz à **169.4 MHz** entre un PC (poste de pilotage) et un robot amphibien, sur base d'un PCB **custom** intégrant un PIC18F26Q10 et un TI CC1120, avec une **stack applicative complète** (scan, connexion, transfert de données fiabilisé, statistiques de liaison) et une **TUI Python** côté hôte.

**État final** : les deux cartes sont **opérationnelles**. La liaison RF fonctionne, l'ARQ stop-and-wait délivre les paquets sans perte à RSSI nominal, l'interface utilisateur permet de scanner, connecter, échanger des données et observer la qualité de liaison en temps réel.

Ce rapport résume l'architecture finale, les performances mesurées, et le parcours de bring-up qui s'est étalé sur sept phases majeures dont les plus pénibles ont demandé du diagnostic differential pin-par-pin au multimètre et la relecture mot par mot du user guide CC1120.

Pour le journal chronologique détaillé phase par phase, voir [RAPPORT_DEBUG.md](RAPPORT_DEBUG.md).

---

## 2. Architecture du système

### 2.1 Chaîne complète

```
┌─────────┐  UART framé    ┌──────────────┐  RF 169.4 MHz   ┌─────────────┐  UART transparent  ┌──────────┐
│  PC     │ ────────────>  │ Modem REMOTE │ ─────────────>  │ Modem ROBOT │  ───────────────>  │ PC robot │
│ (TUI)   │  <────────────  │ (PIC + CC1120)│  <─────────────  │(PIC + CC1120)│  <───────────────  │(terminal)│
└─────────┘                 └──────────────┘                 └─────────────┘                    └──────────┘
```

Deux PCB **identiques** au design (même MCU, même CC1120, même routage RF). La différenciation se fait au moment de la compilation via le flag `MODEM_ROLE = REMOTE | ROBOT`, qui sélectionne :

- Le rôle dans la state machine applicative
- L'orientation TX/RX des pins UART (la carte PC a un FT231 USB-CDC avec ses propres conventions ; la carte robot expose un UART direct)

### 2.2 Pile firmware

```
┌──────────────────────────────────────┐
│  app/ — state machine (REMOTE/ROBOT) │
├──────────────────────────────────────┤
│  protocol/ — framing UART (host I/O) │
│  radio/radio_link — ARQ, beacons     │
├──────────────────────────────────────┤
│  radio/cc1120 — driver SPI bas niveau│
│  drivers/ — UART, SPI, Timer0 (1ms)  │
├──────────────────────────────────────┤
│  board/ — abstraction PIC18F26Q10    │
└──────────────────────────────────────┘
```

Tout est partagé entre les deux rôles. Seuls quelques `#if MODEM_ROLE` ciblent les divergences.

### 2.3 Outils hôte

`host_tools/modem_console/` — application **Textual** (TUI Python full-screen) :

- **Status panel** : état de la connexion, ID robot, RSSI, PER, RTT, dernier TX/RX
- **Discovery panel** : tableau temps-réel des robots détectés (ID, RSSI dBm)
- **Event log** : feed coloré horodaté (TX/RX/CMD/ACK/ERROR)
- **Prompt** : `connect <id>`, `disconnect`, `send "<texte>"`, `clear`, `help`, `quit`

Le panneau Connection est mis à jour automatiquement par les frames `UART_MSG_STATS` que le firmware push toutes les 500 ms.

---

## 3. Spécifications techniques

### 3.1 Paramètres RF

| Paramètre | Valeur | Source |
|---|---|---|
| Fréquence porteuse | 169.4 MHz | SmartRF Studio export (FREQ = 0x69E466) |
| Modulation | 2-GFSK | MODCFG_DEV_E = 0x0B |
| Débit symboles | 4.8 kbps | SYMBOL_RATE2/1/0 = 43 A9 2F |
| Déviation | 5 kHz | DEVIATION_M = 0x06 |
| Bande passante RX | 100 kHz | CHAN_BW = 0x02 |
| Sync word | 0x930B51DE (32 bits) | SYNC3..0 |
| CRC | CRC-16 hardware | PKT_CFG1.CRC_CFG |
| Mode paquet | Variable length + APPEND_STATUS | PKT_CFG0 = 0x20 |
| Puissance PA | ~14 dBm (25 mW) | PA_CFG2 = 0x7D |
| Sensibilité (mesurée) | ~-110 dBm @ 1% PER | Datasheet CC1120, validé en pratique |

### 3.2 Protocole RF (couche radio_link)

Format paquet (CC1120 variable packet length mode) :
```
[LEN][NET_ID][SRC_ID][DST_ID][TYPE][SEQ][PAYLOAD...]   + [RSSI][CRC_OK<<7|LQI]
```
- `LEN` = nombre d'octets de NET_ID jusqu'à fin payload
- `APPEND_STATUS` : 2 octets ajoutés par le matériel (RSSI signé + LQI + flag CRC OK)

| Type | Hex | Description |
|---|---|---|
| `BEACON` | 0x20 | Broadcast périodique (500 ms) émis par robots |
| `CONNECT_REQ` | 0x30 | REMOTE → robot, demande de connexion |
| `CONNECT_OK` | 0x31 | Robot → REMOTE, acceptation |
| `DISCONNECT` | 0x32 | Fin de session, peer informée |
| `DATA` | 0x40 | Données applicatives (avec ARQ) |
| `DATA_ACK` | 0x41 | Acquittement ARQ |
| `STATS` | 0x50 | PING/PONG pour mesure RTT |

ARQ stop-and-wait sur DATA : retransmit avec backoff aléatoire, max 6 retries, déduplication des doublons côté récepteur, ACK auto-émis.

### 3.3 Protocole UART hôte ↔ REMOTE

Format : `[0xAA][0x55][LEN][TYPE][PAYLOAD...][CRC8]`
- CRC-8 polynôme 0x07, init 0x00, calculé sur `LEN || TYPE || PAYLOAD`
- LEN = nombre d'octets de TYPE jusqu'à fin payload

Hôte → REMOTE : `CONNECT`, `DISCONNECT`, `DATA_TX` (`SCAN_*` et `GET_STATS` existent encore dans le firmware par compatibilité mais ne sont plus envoyés par la TUI).

REMOTE → Hôte : `SCAN_RESULT`, `CONNECTED`, `DISCONNECTED`, `STATS`, `TX_ACK`, `DATA_RX`, `LOG`.

### 3.4 Performances mesurées

| Métrique | Valeur observée | Conditions |
|---|---|---|
| Distance LOS intérieur, RSSI > -90 dBm | ~2 m sans aucun effort | Cartes posées sur table |
| Distance avec dégradation acceptable | ~5-10 m intérieur | Avec obstacles, meubles |
| Marge théorique en signal | 30 dB à 2 m (RSSI -77, sensibilité -110) | Vérifié par RSSI réel |
| RTT (ping-pong RF) | 100-200 ms typique | Limité par 4.8 kbps + ARQ |
| Débit utile | ~few hundred bytes/sec | Limité par air-time + ARQ stop-and-wait |
| PER en marge confortable | ≈ 0 % | RSSI > -95 dBm |
| Stabilité session | > 5 min sans coupure | À RSSI nominal, après fix Phase 11 |

---

## 4. Difficultés rencontrées — résumé

Le projet a traversé sept phases majeures de bring-up. Chacune révélait un problème distinct, parfois en cascade. Voici les difficultés synthétisées par ordre chronologique.

### 4.1 Bring-up SPI (Phase 1)

**Symptôme** : `PARTNUMBER` lu = `0xFF`, SPI muet.

**Causes** (firmware) :
- Codes PPS du PIC18Q10 erronés dans `board_spi_hw_init` :
  - `RC3PPS = 0x10` au lieu de `0x0F` (sortait SDO sur la pin SCK)
  - `RC5PPS = 0x11` au lieu de `0x10` (sortait MSSP2 sur la pin SDO)
- `SSP1CLKPPS` jamais initialisé : en master mode, le module MSSP doit relire sa propre sortie SCK comme entrée pour cadencer son shift register
- `SSP1STAT.SMP = 1` (échantillonnage en fin de bit) au lieu de `0` (milieu) pour SPI mode 0
- `INTCON2bits.INTEDG0` (n'existe pas sur Q10) au lieu de `INTCONbits.INT0EDG`

**Résolution** : corrections dans `board.c`, PARTNUMBER lit `0x48` (CC1120).

### 4.2 Audit des adresses de registres (Phase 2)

**Symptôme** : tentative de calibration → MARCSTATE bloqué sur 0x04, chip qui chauffe en <1 s.

**Cause** : le header `cc1120_regs.h` repris d'une source non vérifiée avait :
- `FREQ_IF_CFG` manquant → tous les registres entre 0x0F et 0x2E décalés
- Beaucoup d'extended registers absents (FS_CAL*, FS_VCO*, XOSC*, IFAMP, LNA, ...)

**Résolution** : refonte complète de `cc1120_regs.h` ligne par ligne d'après SWRS112H + SWRU295E Table 22.

### 4.3 Calibration impossible — premier diagnostic différentiel (Phase 3-5)

**Symptôme** : même après audit registres, calibration plante sur la carte robot. MARCSTATE = 0x04, chip chauffe et devient non-répondante.

**Itérations** : variations de SETTLING_CFG (FSREG_TIME), ajout de SIDLE explicite, tentative de la routine errata SWRZ039D (deux passes SCAL avec VCDAC différents). **Aucune** ne corrige le problème.

**Diagnostic décisif** : flash de la **même** firmware sur la carte PC. Calibration **réussit immédiatement** côté PC :
```
MARCSTATE = 0x01 IDLE  ✓
FS_VCO2 = 0x3A, FS_VCO4 = 0x13, FS_CHP = 0x29
```
Chip PC reste froide.

**Conclusion** : pas un bug firmware. Bug **hardware spécifique à la carte robot**.

### 4.4 Défaut hardware identifié au multimètre (Phase 6)

**Méthode** : mesure DC pin par pin sur les deux CC1120, comparaison ligne par ligne.

**Résultat clé** :

| Pin | Nom | PC (OK) | Robot (KO) |
|---|---|---|---|
| 14 | **RBIAS** | **1.20 V** | **2.97 V** ⚠️ |
| 21 | DCPL_VCO | 0 V | 1.39 V |
| 24 | LFC_1 | 0 V | 1.40 V |
| 26 | DCPL_PFD_CHP | 0 V | 0.53 V |

**Interprétation** : RBIAS définit la bias current master de toute la partie analog (LDO, VCO, ampli Pierce XOSC). Saturation à VDD_REG ⇒ source de courant interne du CC1120 endommagée. Vérification à l'ohmmètre carte débranchée : R141 = 56 kΩ ✓ (composant externe OK).

**Cause probable** : semaines de cycles de calibration qui échouait, chip qui chauffait à chaque tentative, fatigue thermique du circuit analog.

**Résolution** : commande d'un CC1120RGZR neuf (~5 €), rework QFN à l'air chaud + ressoudure C201 (AVDD_SYNTH) qui avait aussi une soudure froide.

### 4.5 RX impossible après remplacement (Phase 7-9)

Une fois le CC1120 robot remplacé, TX fonctionnel des deux côtés. Mais **aucune RX** : `NUM_RXBYTES` reste à 0.

Trois bugs cumulés découverts par instrumentation (compteurs d'événements push toutes les 2 s) :

1. **Bit-encoding modem incorrect** : `MODCFG_DEV_E = 0x05` annoncé "2-GFSK" mais en réalité 2-FSK ; déviation registre à 80 kHz alors que CHAN_BW = 100 kHz. Résolu par re-export SmartRF Studio appliqué verbatim.
2. **INT0 sur le mauvais front** : `INTCONbits.INT0EDG = 1` (front montant) faisait fire l'IRQ au sync detect (début RX), FIFO encore vide → l'événement était classifié `TX_DONE` au lieu de `RX_DONE`. Correction : front descendant (`INT0EDG = 0`), IRQ à la fin du paquet.
3. **`cc1120_flush_rx` laissait la chip en IDLE** : un seul paquet rejeté (CRC fail, len invalide) tuait le RX pour le reste de la session. Correction : ajout d'un `cc1120_strobe_srx()` à la fin de `flush_rx`.

À ce stade, liaison RF end-to-end fonctionnelle, mais avec stabilité fragile (disconnects 0x03 sporadiques).

### 4.6 Bug subtil de bit-layout RFEND_CFG (Phase 11)

**Symptôme** : à RSSI -77 dBm (30 dB de marge), toujours des `DISCONNECTED reason=0x03` (LINK_TIMEOUT) sporadiques. Scan ne découvrait les robots qu'après un premier TX.

**Investigation** : un défensif logiciel (re-strober SRX si MARCSTATE ≠ RX) cachait à la fois la cause et le diagnostic — agressif = scan marche mais 0x03 fréquents (interruption des TX) ; passif = sessions stables mais scan ne marche pas. Pas d'arbitrage possible sans toucher au défensif.

**Cause racine** : le commentaire dans `cc1120.c` décrivait pour `RFEND_CFG1` une layout de bits **fausse**. D'après SWRU295E p.87 :
- `RFEND_CFG1.RXOFF_MODE` = bits 5:4 (le commentaire disait 4:3)
- `TXOFF_MODE` est dans `RFEND_CFG0`, pas dans `RFEND_CFG1` (le commentaire le mettait dans bits 1:0 de CFG1)

Donc avec `RFEND_CFG1 = 0x1F` qu'on avait écrit :
- Bits 5:4 = `01` = **FSTXON** (pas RX !)
- → après chaque RX, la chip quittait le mode RX

Et `RFEND_CFG0` n'ayant jamais été écrit :
- Bits 5:4 = `00` = **IDLE**
- → après chaque TX, la chip tombait en IDLE

Le défensif software rattrapait, mais avec une race condition contre les TX en cours et contre la cal post-TX (FS_AUTOCAL).

**Fix** :
```c
{ CC1120_RFEND_CFG1, 0x3Fu },  // RXOFF_MODE = 11 = RX
{ CC1120_RFEND_CFG0, 0x30u },  // TXOFF_MODE = 11 = RX
```

Maintenant la chip reste en RX par hardware. Le défensif software peut être enlevé (gardé comme filet de sécurité minimal).

**Leçon** : un commentaire qui dit "ce bit fait X" n'est pas une preuve. Toujours recroiser avec la datasheet quand le comportement ne colle pas avec ce que prétend le code.

---

## 4.7 Mode passthrough — bridge UART transparent (Phase 12)

### Motivation
Une fois la liaison RF stabilisée, l'objectif suivant a été de démontrer qu'on pouvait l'utiliser comme un **câble série transparent à distance**, c'est-à-dire qu'un terminal sur le PC pilote puisse interagir avec un shell Linux tournant sur un PC côté robot, à travers les 169.4 MHz.

### Architecture du mode passthrough

```
[PC pilote]                                                          [PC robot]
   |                                                                       |
   | PuTTY (raw)            REMOTE                  ROBOT      PuTTY/getty |
   |   ─raw bytes─>  [framing skipped]   ─RF DATA─>  [transparent]  ─>  shell
   |                                                                       |
   | <─raw bytes─    [framing skipped]   <─RF DATA─  [transparent]  <─  echo
```

Quand la TUI envoie la nouvelle commande `passthrough` (UART_MSG = 0x06), le firmware REMOTE bascule dans l'état `APP_REMOTE_PASSTHROUGH` :
- Le décodeur de frame UART hôte est court-circuité : tous les octets reçus sur l'UART sont poussés directement dans le buffer de coalescing DATA → envoyés tels quels en RF avec ARQ
- Les paquets DATA reçus en RF sont écrits **bruts** sur l'UART hôte (sans encapsulation framée)
- Les messages framés normalement émis (STATS push 2 Hz, TX_ACK, DISCONNECTED, SCAN_RESULT) sont **supprimés** pour ne pas polluer la sortie shell

Le robot, lui, n'a aucune logique passthrough — il est **déjà transparent par design** depuis le début (UART RX → DATA buffer, DATA RX → UART TX). Aucune modification firmware côté robot pour le passthrough lui-même.

### Sticky reconnect

Une fois en passthrough, la TUI est fermée (PuTTY a pris le port). En cas de drop de liaison, l'utilisateur n'a aucun moyen d'envoyer une commande pour se reconnecter. Le firmware gère donc le reconnect tout seul :
- À un LINK_TIMEOUT (6 s sans rx) ou ARQ_FAILED, on **ne tombe pas en IDLE_SCAN**
- Une bannière `*** link lost, reconnecting... ***` est écrite **directement** sur l'UART hôte (PuTTY l'affiche en clair)
- `CONNECT_REQ` est re-émis vers le robot toutes les 1 s
- Dès que le robot répond `CONNECT_OK`, une bannière `*** reconnected ***` confirme et le shell reprend
- Seul un power-cycle de la carte REMOTE permet de sortir du mode passthrough

### Robustification du keep-alive (effet de bord du passthrough)

Pour que le passthrough survive à des sessions longues, deux bugs latents du keep-alive ont été corrigés :

**Bug A — pong perdu = liaison morte**. Le code de RTT ping avait un guard `!rtt_ping_pending_` : si un seul PONG était perdu, le flag pending restait `true` pour toujours et plus aucun ping n'était émis jusqu'au LINK_TIMEOUT (6 s). Fix : suppression du guard. Le ping est émis sur l'intervalle quoi qu'il arrive ; le token dans le PONG continue de lier la mesure RTT au bon ping.

**Bug B — pas de keep-alive 2 voies en session**. Le robot arrêtait de broadcaster ses beacons dès qu'il passait en `APP_ROBOT_CONNECTED`. Seuls les pongs (1 Hz) servaient de keep-alive sur le REMOTE. Fix : le robot continue de beacon-er toutes les 500 ms même en CONNECTED. Le REMOTE a donc maintenant **12 chances de keep-alive** dans la fenêtre 6 s (12 beacons + 6 pongs) au lieu de 6.

Conséquence : à -77 dBm le link tient maintenant indéfiniment sans drop spontané, et un drop réel (robot rebooté, hors portée) se récupère en ~7-8 s au lieu de 30 s.

### Effet de bord du beacon-en-CONNECTED — bug PuTTY-loop

Le robot beacon-ant en CONNECTED a déclenché un comportement inattendu : `remote_handle_beacon` émettait un `UART_MSG_SCAN_RESULT` framé sur l'UART hôte à **chaque** beacon, sans filtrer par état. En passthrough, ces octets framés arrivaient dans PuTTY comme du binaire. Quand le CRC du SCAN_RESULT tombait par hasard sur `0x05` (ENQ), PuTTY répondait avec son **answerback** par défaut, la chaîne `"PuTTY\r\n"`. Cette réponse partait via RF vers le robot, ressortait sur la session distante, et l'affichage local de l'echo donnait l'illusion d'un loop "PuTTY PuTTY PuTTY...".

Fix : SCAN_RESULT framé n'est émis qu'en état `APP_REMOTE_IDLE_SCAN` (le seul état où c'est utile à la TUI). En CONNECTING/SESSION/PASSTHROUGH, le beacon met juste à jour le cache + le watchdog, sans output UART.

### Démonstration

Le mode passthrough permet de :
- Se logger sur un PC distant (`agetty -L 115200 ttyUSB0 vt100` sur le robot)
- Taper des commandes shell (`ls`, `cd`, `cat`, `nano`)
- Éditer des fichiers (latence ~150 ms par caractère, ~10 s pour redessiner l'écran de `nano`)
- Tout cela à travers 169.4 MHz sur ~5-10 m d'intérieur

C'est l'aboutissement du projet — la liaison RF est devenue un "câble série virtuel" exploitable par n'importe quel programme qui parle un port COM.

---

## 5. Leçons apprises

1. **Valider chaque couche avant d'empiler la suivante**. Le bring-up SPI a été l'investissement le plus rentable — chaque doute non levé en bas du stack se paye au triple en haut.
2. **Avoir deux cartes du même design est crucial** pour le diagnostic différentiel. Sans la carte PC qui fonctionnait, on n'aurait jamais pu trancher entre bug firmware et défaut hardware.
3. **Le pin-par-pin au multimètre est l'outil ultime pour le debug analog**. Aucun test logiciel n'aurait identifié RBIAS comme coupable — il fallait simplement mesurer.
4. **Les cycles de cal qui échouent endommagent réellement la chip à long terme**. Une chip qui chauffe pendant qu'on essaie 50 variantes de cal n'est pas une chip en bonne santé après 50 cycles.
5. **Les commentaires ne sont pas vérifiables**. Un commentaire faux peut survivre des mois en cachant un bug — surtout quand le bug est masqué par un workaround logiciel.
6. **L'instrumentation temporaire est essentielle**. Les compteurs d'événements (INT0, RX_DONE, TX_DONE, FIFO_ERR) push toutes les 2 s dans le log UART ont été ce qui a permis d'identifier les trois bugs RX cumulés de Phase 9. Sans données quantitatives, on tourne en rond.
7. **Le code de référence "qui marchait" l'année passée n'est pas une garantie absolue** — RF169Hz.X utilisait une carte d'évaluation TI, pas un PCB custom comme le nôtre, et seul le TX y avait été validé.

---

## 6. Limites connues et améliorations futures

- **Multi-robots simultanés** : actuellement le firmware filtre les paquets RF sur `dst_id == local_id`, ce qui suffit pour un robot à la fois. Pour gérer une flotte, il faudrait un schéma d'adressage et un mécanisme d'anti-collision sur les CONNECT_REQ.
- **Robustesse en milieu bruité** : la bande 169 MHz ISR est généralement très calme, mais en présence d'interférences (radioamateurs proches, autres équipements télémétrie) la liaison peut souffrir. Un mécanisme de **frequency hopping** ou **diversity** apporterait de la résilience.
- **Cache de calibration** : la calibration FS_AUTOCAL prend ~700 µs à chaque transition IDLE→RX/TX. Le sauvegarder en EEPROM côté PIC permettrait de réduire la latence post-boot.
- **Beacon interval adaptatif** : actuellement 500 ms fixe. Pourrait être augmenté en idle (économie d'énergie côté robot) et réduit pendant les sessions actives (meilleure keep-alive).
- **Hardware PCB v2** :
  - Pull-up 100 kΩ sur RESET_N → VDD_REG (était flottant en v1)
  - Load caps quartz 32 MHz à 27 pF (au lieu de 33 pF actuels) pour CL nominal
  - Optionnellement router RESET_N vers une GPIO du PIC pour permettre un hardware reset piloté

---

## 7. Structure du dépôt

```
Project_Robotics_169.4/
├── firmware/
│   ├── modem/                  # sources partagées REMOTE + ROBOT
│   │   ├── app/                # state machines
│   │   ├── board/              # abstraction PIC18F26Q10 (PPS, clock, GPIO)
│   │   ├── drivers/            # UART, SPI, Timer0
│   │   ├── protocol/           # framing UART hôte
│   │   ├── radio/              # driver CC1120 + radio_link + ARQ
│   │   ├── util/               # ring buffer, CRC-8
│   │   ├── config.h            # constantes globales (NET_ID, timeouts)
│   │   └── main.c              # super-loop
│   ├── Modem_remote.X/         # projet MPLAB X (MODEM_ROLE=REMOTE=1)
│   └── Modem_robot.X/          # projet MPLAB X (MODEM_ROLE=ROBOT=2)
├── host_tools/
│   └── modem_console/          # TUI Python (Textual + pyserial)
│       └── modem_console/
│           ├── protocol.py     # encodage/décodage framing
│           ├── serial_link.py  # thread reader série
│           ├── tui.py          # UI Textual
│           └── __main__.py     # entry point CLI
├── README.md                   # vue d'ensemble + getting started
├── TESTING.md                  # procédure de test pas-à-pas
├── RAPPORT_DEBUG.md            # journal chronologique du bring-up
├── DEBUG_STATE.md              # snapshot Phase 1 (bring-up SPI)
└── RAPPORT_FINAL.md            # ce document
```

---

## 8. Conclusion

Le projet a abouti à une liaison RF fonctionnelle de bout en bout, validée sur deux PCB custom indépendants, avec un protocole applicatif complet et une interface utilisateur ergonomique. Le parcours a fait remonter à la fois des bugs firmware "classiques" (bit-encoding, mauvaise pin, mauvaise fréquence d'interruption) et un défaut hardware non trivial (CC1120 endommagé par des semaines de calibrations infructueuses).

La plupart du temps de debug a été absorbé non par la programmation à proprement parler, mais par le **diagnostic** : isoler la couche fautive (firmware vs hardware vs paramètres RF), constuire des instruments de mesure logiciels (compteurs d'événements push UART), et recroiser systématiquement le comportement observé avec la datasheet officielle TI plutôt qu'avec les commentaires du code.

Le projet est livré dans un état où les **deux cartes fonctionnent**, la liaison est **stable à RSSI nominal**, et l'architecture firmware est suffisamment modulaire pour permettre l'évolution vers du multi-robots ou du frequency-hopping si nécessaire.

---

*Dernière mise à jour : 2026-06-05.*
