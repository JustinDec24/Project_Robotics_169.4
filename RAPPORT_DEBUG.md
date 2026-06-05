# Rapport de débogage — Carte modem RF 169.4 MHz

> Journal chronologique du bring-up et du diagnostic des cartes PC + robot.
> Ce document est tenu à jour au fil des sessions de debug.

## Contexte du projet

- **Objectif** : communication radio bidirectionnelle entre un PC et un robot amphibien à **169.4 MHz**
- **Carte côté PC** : PIC18F26Q10 + CC1120 + FT231XQ-R (USB-C)
- **Carte côté robot** : même design, mais UART transparent (module miniuart3 externe à la place du FT231)
- **Référence historique** : projet RF169Hz.X de Kool Inge (EPFL BioRob, printemps 2025) — utilisait un PIC16F17125 + carte d'évaluation **CC1120EMK-169 de TI** (pas un layout CC1120 custom). Seul le TX fonctionnait à la fin du projet précédent, la réception n'a jamais marché.
- **Différence majeure avec l'année passée** : notre projet est la **première tentative de PCB CC1120 custom** (sans utiliser la carte de référence TI). Toute la partie RF du PCB est donc inédite.

## Architecture firmware

Sources partagées dans `firmware/modem/` (init bas niveau + driver CC1120 + stack applicative). Deux projets MPLAB X distincts (`Modem_remote.X` et `Modem_robot.X`) sélectionnent le rôle via un flag de compilation `MODEM_ROLE = REMOTE | ROBOT`.

Couches identifiées :
- `board/` — abstraction PIC18F26Q10 (PPS, clock, GPIO, UART, SPI, timers, ISR)
- `drivers/` — interfaces UART/SPI/timer
- `radio/cc1120*` — driver bas niveau du transceiver + tables de registres
- `radio/radio_link.c` — couche paquet, ARQ, buffer
- `protocol/protocol.c` — framing UART avec messages typés
- `app/app.c` — state machines pour les rôles REMOTE et ROBOT

---

## Phase 1 — Bring-up SPI (PARTNUMBER = 0xFF)

### Symptôme initial
À la première mise sous tension de la carte robot, la lecture du registre PARTNUMBER du CC1120 retournait systématiquement `0xFF` — signe que la ligne MISO restait haute en permanence et que le SPI ne communiquait pas du tout.

### Investigation
Suite à une lecture détaillée du datasheet PIC18F26Q10 (DS40001996A) et de l'errata, nous avons identifié plusieurs erreurs dans la configuration PPS (Peripheral Pin Select) :

1. **Codes PPS de sortie erronés** :
   - `PPS_OUT_SCK1` était à `0x10` (en réalité le code de SDO1) → corrigé à `0x0F`
   - `PPS_OUT_SDO1` était à `0x11` (code de SCK2 du MSSP2, pas du MSSP1) → corrigé à `0x10`
2. **Registre `SSP1CLKPPS` manquant** : en mode SPI master, le PIC18Q10 nécessite que la pin SCK soit également mappée en entrée vers le module MSSP via `SSP1CLKPPS`. Ajouté avec `0x13` (RC3).
3. **Polarité d'échantillonnage** : `SSP1STAT.SMP` était à 1 (échantillonnage en fin) alors qu'en SPI mode 0 il faut 0 (échantillonnage au milieu).
4. **Bit INT0 introuvable** : `INTCON2bits.INTEDG0` n'existe pas sur le Q10 → remplacé par `INTCONbits.INT0EDG`.

### Résultat
Après corrections, PARTNUMBER lit `0x48` (= CC1120). Le SPI marche.

### Fichiers modifiés
- `firmware/modem/board/board.c` (config PPS, SPI master, INT0)

---

## Phase 2 — Audit des adresses de registres CC1120

### Symptôme
Première tentative de calibration (`SCAL` après écriture des registres) : la chip plante immédiatement, MARCSTATE bloqué à `0x04` (REG_SETTLE_MC), température qui monte en <1 ms.

### Investigation
Comparaison méthodique de `cc1120_regs.h` avec la datasheet **CC1120 SWRS112H** + le user guide **SWRU295E** Table 22 :

**Erreurs trouvées sur les registres standards** (0x00–0x2E) :
- `FREQ_IF_CFG` manquait à `0x0F` → tous les registres entre `0x0F` et `0x2E` étaient décalés de -1 (écrits à la mauvaise adresse)
- `PA_CFG2` manquait à `0x2B`, ce qui décalait `PA_CFG1`, `PA_CFG0` et `PKT_LEN`

**Erreurs trouvées sur les registres étendus** (préfixe `0x2F`) :
- `FS_CAL0/1/2/3` étaient mal positionnés (décalage de ~3)
- `FS_CHP`, `FS_DIVTWO`, `FS_DSM0/1`, `FS_DVC0/1`, `FS_LBI`, `FS_VCO1/3` également mal adressés
- Beaucoup d'extended registers absents : `IF_ADC1`, `IF_ADC2`, `TOC_CFG`, `EXT_CTRL`, `GBIAS0–6`, `IFAMP`, `LNA`, `RXMIX`, `XOSC0/2/3/4`

### Résultat
Toutes les adresses ont été corrigées et alignées sur la datasheet officielle. Cela n'a pas suffi à faire passer la cal, mais c'était un prérequis bloquant à corriger.

### Fichiers modifiés
- `firmware/modem/radio/cc1120_regs.h`

---

## Phase 3 — Tentatives de cal avec config SmartRF Studio

### Code utilisé
Programme de bring-up dans `main.c` exécutant :
1. `cc1120_init_minimal()` (reset SPI + écriture des tables de registres + `SCAL`)
2. Lecture de MARCSTATE et de SNOP
3. Lecture des résultats de cal (`FS_VCO2/4/CHP`)

### Tests bissection (Phase 3a → 3i)
Variantes progressives pour isoler quel registre / quelle étape faisait planter la chip :
- Phase 3a : registres standards seuls, pas d'étendus, pas de SCAL
- Phase 3b : standards + étendus, pas de SCAL
- Phase 3c : tout sauf le SCAL final
- Phase 3d–3i : modifications fines de SETTLING_CFG, des délais, du nombre de SCAL strobes

### Hypothèses explorées
- **SETTLING_CFG = 0x08 (FSREG_TIME=0)** → temps de settling du LDO synth nul → changé en `0x0B` (FSREG_TIME=3, max)
- **Manque de SIDLE entre SRES et l'ApplyConfig** → ajout d'un SIDLE explicite
- **Délais trop courts** → passés à 10 ms entre strobes, 100 µs entre écritures registres
- **Bug d'errata silicon** → implémentation de la routine `manualCalibration()` du TI errata SWRZ039D (deux passes SCAL avec valeurs VCDAC différentes)

### Résultat
Aucune de ces variations n'a fait passer la cal. La chip plantait systématiquement à `REG_SETTLE_MC`, chauffait, devenait non-répondante (`MARCSTATE=0x1F`).

### Conclusion intermédiaire
Le problème ne semblait pas être un paramètre logiciel trivial. Il fallait soit une config registre fondamentalement différente, soit aller chercher côté hardware.

---

## Phase 4 — Comparaison avec le code "working" RF169Hz.X

### Démarche
L'utilisateur a ajouté le dossier `RF169Hz.X/` (projet de l'année passée). Lecture détaillée du `main.c`, `cc1120.c`, `cc1120_config.c` et du **rapport BioRob d'Inge Kool (printemps 2025)**.

### Insights tirés du rapport
1. Le projet précédent **n'a jamais réussi la réception**, seulement le TX (donc seulement la calibration et l'émission de paquets ont été validées).
2. Le PCB utilisé était la **carte d'évaluation TI CC1120EMK-169**, pas un layout custom. Notre PCB est inédit côté RF.
3. Le MCU était un **PIC16F17125** (16-bit, 16 pins) — codes PPS et registres totalement différents de notre PIC18F26Q10.
4. La fréquence cible était **169.0 MHz** (FREQ = 0x69A000), pas 169.4 MHz comme nous.
5. Modulation était **2-FSK 1.5 kbps** dans le rapport, mais le fichier `cc1120_config.c` versionné contient **4-GFSK 60 kbps** (probablement une itération plus tardive).

### Test diagnostic : copier la table de registres RF169Hz.X telle quelle
Tentative d'utiliser exactement les valeurs du code de l'année passée comme **test diagnostic pur** :
- Si la cal passe → notre config SmartRF était fausse
- Si la cal échoue toujours → hardware fault

### Résultat
La cal échoue toujours sur la carte robot, exactement comme avant. Donc ce n'était pas une question de valeurs de registres.

### Décision
Reverter à la config SmartRF Studio (4.8 kbps 2-GFSK à 169.4 MHz), avec les adresses corrigées de Phase 2 et SETTLING_CFG=0x0B. Cette config est restée la version "officielle".

### Fichiers modifiés
- `firmware/modem/radio/cc1120.c` (table standard + table étendue alignées SmartRF Studio 169.4 MHz)

---

## Phase 5 — Test sur la carte PC (le test décisif)

### Problème de routage UART découvert
La carte PC utilise un FT231XQ-R pour l'USB-CDC, avec un câblage **inversé** par rapport à la carte robot :
- **Robot** : MCU TX sur RC7, MCU RX sur RC6 (vers connecteur miniuart3)
- **PC** : FT231 TXD (output) → RC7, FT231 RXD (input) → RC6 (convention FTDI)

La build REMOTE doit donc avoir le PPS UART inversé. Code mis à jour pour rendre les assignations conditionnelles sur `MODEM_ROLE`.

### Test de la même firmware sur la carte PC
Flash de la même `cc1120_init_minimal()` (SmartRF Studio 169.4 MHz) sur la carte PC. Résultat :
```
MARCSTATE = 0x01  <-- IDLE, cal SUCCEEDED !
FS_VCO2 = 0x3A
FS_VCO4 = 0x13
FS_CHP  = 0x29
```
Chip reste **froide**.

### Conclusion majeure
Le même firmware, les mêmes registres, le même design PCB :
- **PC** : cal réussit, chip froide
- **Robot** : cal échoue, chip chauffe

Le bug n'est **pas** dans le firmware ni dans la config registre. C'est un **défaut hardware spécifique à la carte robot**.

### Fichiers modifiés
- `firmware/modem/board/board.c` (PPS UART conditionnel sur MODEM_ROLE)

---

## Phase 6 — Caractérisation du défaut hardware robot

À partir de ce point, toutes les tentatives sont sur la **carte robot uniquement**, dans le but d'identifier la nature exacte du défaut.

### Reflow complet du QFN
Hypothèse : soudure froide sur l'un des 32 pins du CC1120, possiblement sur l'exposed pad central. Action : flux + air chaud 320-340°C ~30 sec sur tout le QFN, refroidissement lent.

**Résultat** : aucun changement. La cal échoue toujours pareil.

### Test "chip health check" (Phase 4b)
Programme de diagnostic pur **sans aucun SCAL** :
- Reset SPI
- Lecture PARTNUMBER (attendu 0x48) et PARTVERSION (attendu 0x21 ou 0x23)
- Lecture MARCSTATE (attendu 0x01)
- Vérification des **valeurs reset documentées** de 11 registres de config
- **30 secondes en IDLE** pour mesurer la température sans aucune activité analog
- Re-lecture MARCSTATE après les 30 s

**Résultat sur carte robot** :
- PARTNUMBER = 0x48 ✓
- PARTVERSION = 0x23 ✓
- Tous les registres aux bonnes valeurs reset ✓
- MARCSTATE = 0x01 stable pendant 30 s ✓
- **Chip reste froide** ✓

→ La chip est **digitalement vivante**. La partie digitale (SPI, state machine, mémoire) est intacte.

### Test "analog subsystem probe" (Phase 4c)
Utilisation de la pin GDO0 du CC1120 (reliée à RB1 du PIC) comme "scope sans scope" en y mappant des signaux internes via le registre `IOCFG0`.

**Test A** : `IOCFG0 = 0x39` (XOSC_STABLE). 20 lectures de la pin RB1 à 100 µs d'intervalle après reset.
- **Résultat robot** : `00000000000000000000` → le XOSC ne démarre **jamais** d'après ce que la chip elle-même rapporte.

**Test B** : sanité SPI (écriture puis relecture de `IOCFG0=0x30`). OK.

**Test C** : trace MARCSTATE pendant une tentative de cal avec SIDLE rapide après 5 ms.
- **Résultat robot** : trace `07 13 13 13 13 13 ...` → le state machine saute en TX sans passer par les états de cal (`0x04`, `0x05`, `0x06`), ce qui est anormal — signe que la chip pense avoir un clock externe valide alors qu'il n'y a pas de XOSC.

### Hypothèses successives écartées
1. **Pin 32 EXT_XOSC flottant** : envisagée car en mode crystal cette pin doit être à GND. **Écartée** car la carte PC fonctionne avec le même design (pin 32 flottante aussi).
2. **Caps de charge du cristal mauvaise valeur (22 pF vs 33 pF)** : envisagée car la sonde de l'oscilloscope ajoute du C parasite et masquait le problème. **Écartée** car le test avec 33 pF sur la robot échouait aussi.
3. **Cristal 32 MHz endommagé** : envisagé car les cristaux sont sensibles à la chaleur. Mesure de fréquence à 31.9964 MHz au scope → cristal oscille bien, donc **écarté**.

---

## Phase 7 — Diagnostic différentiel pin par pin

### Méthodologie
Mesure de la tension DC sur **tous les pins critiques** du CC1120 sur les deux cartes, dans le même état (Phase 4b flashé, chip en IDLE digital après le countdown). Comparaison ligne par ligne.

### Données comparatives

| Pin | Nom | PC (OK) | Robot (KO) | Delta |
|---|---|---|---|---|
| 1 | AVDD_GUARD | 3.3 | 3.3 | 0 |
| 5, 12 | DVDD | 3.3 | 3.3 | 0 |
| 13 | AVDD_IF | 3.3 | 3.3 | 0 |
| **14** | **RBIAS** | **1.20** | **2.97** | **+1.77 ⚠️** |
| 15 | AVDD_FRONTEND | 3.3 | 3.3 | 0 |
| 17 | PA | 3.3 | 3.3 | 0 |
| 18 | TRX_SW | 0 | 0 | 0 |
| 19 | LNA_P | 0 | 0.175 | +0.175 |
| 20 | LNA_N | 0 | 0.343 | +0.343 |
| 21 | DCPL_VCO | 0 | 1.394 | +1.394 ⚠️ |
| 22 | AVDD_SYNTH | 3.3 | 3.3 (reset si on touche la cap) | 0 |
| 23 | LFC_0 | 0 | 0.387 | +0.387 |
| 24 | LFC_1 | 0 | 1.404 | +1.404 ⚠️ |
| 25 | AVDD_PFD_CHP | 3.3 | 3.3 | 0 |
| 26 | DCPL_PFD_CHP | 0 | 0.527 | +0.527 ⚠️ |
| 27 | AVDD_SYNTH_CMOS | 3.3 | 3.3 | 0 |
| 28 | AVDD_XOSC | 3.3 | 3.3 | 0 |
| 29 | DCPL_XOSC | 1.75 | 2.28 | +0.53 ⚠️ |
| 30 | XOSC_Q1 | 0.69 | 1.26 | +0.57 ⚠️ |
| 31 | XOSC_Q2 | 0.68 | 1.25 | +0.57 ⚠️ |

### Pattern observé
- **Toutes les alimentations externes (AVDD)** sont identiques entre les deux cartes ✓
- **Tous les pins liés au bias interne** (DCPL = sorties LDO, LFC, XOSC_Q1/Q2, RBIAS) sont **anormalement élevés** sur la robot
- **Le plus gros delta** est sur **RBIAS (+1.77 V)**

### Interprétation
RBIAS est la pin qui définit la **bias current master** de toute la partie analog du CC1120 (via une résistance externe `R141 = 56 kΩ` vers GND). Si RBIAS est cassée, **TOUS** les blocs analog (LDOs, VCO, ampli Pierce du XOSC, AGC, etc.) reçoivent un courant de bias incorrect → dysfonctionnement généralisé.

Le fait que RBIAS = 2.97 V (≈ saturation contre VDD_REG) suggère que la source de courant interne du CC1120 ne trouve pas de chemin de retour vers GND.

### Test décisif : R141 externe
Multimètre en ohmmètre, **carte débranchée**, mesure entre pin 14 et GND :
- **Résultat : ~56 kΩ** ✓

→ La résistance externe R141 est en parfait état. Donc le problème est **interne à la chip** : la source de courant de bias du CC1120 elle-même est endommagée.

### Conclusion finale
- **PCB design** : OK (PC le prouve)
- **Composants passifs** : OK (R141 confirmée à 56 kΩ)
- **Soudures** : OK (reflow complet, AVDD partout corrects)
- **CC1120 chip robot** : 🔴 **source de bias interne grillée**

Cause probable : les cycles répétés de cal qui échouait (chip chauffant à chaque tentative pendant des semaines) ont fini par endommager le circuit analog de génération de bias. La partie digitale, plus robuste, n'a pas été touchée — d'où le pattern "digital alive, analog dead".

### Bug secondaire détecté
Pin 22 (AVDD_SYNTH) : "la chip reset quand on touche la cap" → cap C201 a une soudure froide ou un terminal partiellement levé. À ressouder en même temps que le remplacement de la chip.

---

## État du projet à la fin de la session

### Carte PC
- ✅ SPI fonctionnel (PARTNUMBER = 0x48)
- ✅ Init complet réussi
- ✅ **Calibration réussie** (MARCSTATE = 0x01 IDLE)
- ✅ Chip reste froide
- ✅ UART bidirectionnel via FT231XQ-R
- ⏸️ TX/RX radio pas encore testé (Phase 5/6 prévues)
- **→ Carte fonctionnelle, prête pour le dev applicatif**

### Carte robot
- ✅ SPI fonctionnel
- ✅ Chip digitalement vivante (Phase 4b OK)
- 🔴 **Calibration impossible** (XOSC ne démarre pas)
- 🔴 **Source de bias interne du CC1120 grillée**
- ⚠️ Cap C201 sur AVDD_SYNTH avec soudure froide
- **→ Carte en attente de remplacement du CC1120 + ressoudure C201**

### Actions à venir
1. Commander un CC1120RGZR neuf (Mouser / Digikey, ~5-8 €)
2. Rework du QFN avec air chaud (désouder ancien, nettoyer pads, ressouder nouveau)
3. Ressouder C201 sur AVDD_SYNTH pendant la même intervention
4. Re-flasher Phase 4b puis Phase 4 → vérifier que la cal passe
5. En parallèle, développer Phase 5 (test TX) et Phase 6 (app layer + commandes UART) sur la carte PC

---

## Corrections firmware accumulées au cours du debug

| Fichier | Nature | Phase |
|---|---|---|
| `firmware/modem/board/board.c` | PPS codes corrigés, SSP1CLKPPS ajouté, SMP corrigé, INT0 fixé | 1 |
| `firmware/modem/board/board.c` | PPS UART conditionnel sur MODEM_ROLE (PC vs robot) | 5 |
| `firmware/modem/radio/cc1120_regs.h` | Adresses des registres alignées sur datasheet TI | 2 |
| `firmware/modem/radio/cc1120.c` | Séquence d'init : SRES → SIDLE → ApplyConfig → SCAL avec délais | 3 |
| `firmware/modem/radio/cc1120.c` | Table standard SmartRF Studio (169.4 MHz, 2-GFSK 4.8 kbps) | 4 |
| `firmware/modem/radio/cc1120.c` | Table étendue SmartRF Studio (FS_CAL*, XOSC*, FREQ*) | 4 |
| `firmware/modem/radio/cc1120.c` | Routine `cc1120_manual_cal()` (errata TI SWRZ039D) | 3 |
| `firmware/modem/main.c` | Programmes de diagnostic phasés (4, 4b, 4c) | 3-7 |

## Programmes de diagnostic développés (résumé fonctionnel)

| Programme | But | Quoi il exécute | Statut |
|---|---|---|---|
| **Phase 4** | Init complet + cal | Reset → registres → SCAL → lit MARCSTATE et FS_VCO/CHP | ✅ Passe sur PC |
| **Phase 4b** | Vérifier que la chip est vivante (no SCAL) | Reset → lit PARTNUMBER, PARTVERSION, 11 valeurs reset, halt 30 s en IDLE | ✅ Passe sur PC + robot (digital alive) |
| **Phase 4c** | Sonder le sous-système analog | Test XOSC_STABLE via GDO0, sanité SPI, trace MARCSTATE pendant cal avec abort rapide | ✅ Discriminant PC (cal OK) vs robot (XOSC dead) |
| **Phase 5** | Tester TX sur carte PC | Écriture TX FIFO, strobe STX, vérification NUM_TXBYTES + MARCSTATE | ✅ **Validé** (6 cycles consécutifs avec verdict OK) |
| **Phase 6** | Activer la stack applicative côté firmware | `app_init()` + `app_task()` en boucle | ✅ **Validé** (commande UART → réponse framed via la TUI, timeouts firmware fonctionnels) |

### Phase 5 — résultat validation

Sortie d'un cycle TX réussi (carte PC, ~4.5 ms en TX state) :

```
NUM_TXBYTES before STX = 11 (expect 11)
MARCSTATE trace: 07 13 13 13 13 13 13 13 13 01 01 01
NUM_TXBYTES after  = 0 (expect 0)
SNOP final status  = 0x0F (IDLE)
Verdict: TX OK
```

→ La chip parcourt bien `SETTLING (0x07) → TX (0x13)` pendant ~4.5 ms, puis revient à IDLE, et la FIFO se vide à 0. La carte PC **émet effectivement** sur 169.4 MHz.

### Bug latent corrigé pendant Phase 5

Découvert pendant la validation TX : les adresses étendues `CC1120_NUM_TXBYTES = 0x7D` et `CC1120_NUM_RXBYTES = 0x7E` dans `cc1120_regs.h` étaient **fausses** — elles pointaient vers `CFM_RX_DATA_OUT` / `CFM_TX_DATA_IN` (registres CFM_DATA non liés, retournant toujours 0). De même `LQI_VAL = 0x72` était `0x74` en réalité.

Adresses corrigées d'après SWRU295E :
- `NUM_TXBYTES = 0xD6`
- `NUM_RXBYTES = 0xD7`
- `LQI_VAL = 0x74`
- Ajout des `FIFO_NUM_TXBYTES = 0xD8` et `FIFO_NUM_RXBYTES = 0xD9`

**Impact** : `radio_link_receive()` utilise `cc1120_get_num_rxbytes()` pour décider s'il y a un paquet à lire. Avec l'ancienne adresse, cette fonction retournait toujours 0 → la couche app aurait **jamais reçu de paquet RF** même avec un robot en face. Bug latent invisible pendant tout le bring-up parce qu'on n'avait pas encore activé Phase 6.

### Bug du timer corrigé pendant Phase 6

Au démarrage de Phase 6, les timeouts firmware (CONNECT_TIMEOUT, LINK_LOST, STATS_PUSH, ARQ_ACK_TIMEOUT) ne firaient jamais : `millis()` retournait systématiquement 0. La cause : configuration Timer0 incorrecte dans `board_timer_hw_init`.

Évolution du diagnostic :

1. **Suspect initial** : `T0CON1 = 0x80` avec commentaire "CS=Fosc/4". Mais sur PIC18F26Q10, `T0CON1.T0CS<2:0>` est codé sur les bits 7-5, et `0x80` correspond à `T0CS=100` = LFINTOSC (31 kHz). Bug de bit-encoding.

2. **Tentative `T0CON1 = 0x20`** (T0CS=001 supposé Fosc/4 d'après une lecture du datasheet) : timer toujours figé. Dump des registres a confirmé que `TMR0L` ne s'incrémentait pas du tout entre deux échantillons espacés de 5000 NOP — le module Timer0 ne tournait pas.

3. **Tentative `T0CON1 = 0x4A`** (T0CS=010 = HFINTOSC, prescaler 1:1024, config copiée de MCC) : timer démarre, mais `millis()` avance à ~250 Hz au lieu de 1 kHz attendu.

4. **Conclusion** : sur PIC18F26Q10, le tap HFINTOSC visible par Timer0 est fixé à ~8 MHz indépendamment du réglage `OSCFRQ` (qui contrôle uniquement le tap visible par la CPU). Avec prescaler 1:1024 sur une source 8 MHz, on tique à 7.8 kHz → millis incrémente trop lentement. Solution finale : `T0CON1 = 0x40` (HFINTOSC source, prescaler 1:1) avec reload `0xE0C0` (= -8000 ticks) → 1 ms par overflow.

5. **Bonus** : `PMD1.TMR0MD` (Peripheral Module Disable) explicitement forcé à 0 dans `board_timer_hw_init` pour éviter un éventuel power-gate du module Timer0 hérité d'un état précédent.

---

## Outils hôte (côté PC)

### `host_tools/modem_console/` — TUI Python pour piloter le modem

Interface terminal interactive (full-screen, layout en panneaux multiples) permettant à l'utilisateur de piloter la carte PC depuis le PC :

- **Status panel** : état de la connexion radio (connecté/déconnecté, ID robot, RSSI moyen, PER, RTT, dernier TX/RX, port et baudrate)
- **Discovery panel** : tableau temps-réel des robots détectés par scan (ID, RSSI, age)
- **Event log** : feed horodaté coloré (TX, RX, LOG, erreurs)
- **Prompt de commandes** : `scan`, `connect <id>`, `disconnect`, `send "<texte>"`, `stats`, `clear`, `help`, `quit`

### Architecture
- `protocol.py` — encodage/décodage du framing UART `[0xAA][0x55][LEN][TYPE][PAYLOAD][CRC8]`, identique au firmware. CRC-8 polynôme 0x07 init 0x00, vérifié par smoke-test.
- `serial_link.py` — port série en lecture continue dans un thread daemon, événements poussés dans une queue Python (frame décodée OU bytes bruts).
- `tui.py` — app **Textual** avec widgets Header, Static (status), DataTable (discovery), RichLog (events), Input (commandes), Footer.
- `__main__.py` — entry point CLI avec `--port`, `--baud`, `--list`.

### Pré-requis côté firmware
La TUI parle au firmware via le framing protocole. Tant que Phase 6 (activation de `app_task()` dans `main.c`) n'est pas faite, le firmware n'émet que les logs UART non-framés des phases de diagnostic. La TUI les affiche quand même dans le log sous le tag `UART`, mais aucun message protocole (SCAN_RESULT, CONNECTED, STATS, etc.) ne sera reçu.

---

## Leçons apprises

1. **Toujours valider chaque couche avant de monter** : le bring-up SPI a pris du temps à cause des PPS, et chaque doute non levé en bas du stack se paye au triple en haut.
2. **Comparer systématiquement les adresses de registres avec la datasheet officielle**, pas un header copié d'une autre origine. Beaucoup d'erreurs trouvées dans `cc1120_regs.h` lors de l'audit.
3. **Le code de référence "qui marchait" l'année passée n'était pas une garantie absolue** — RF169Hz.X utilisait une carte d'évaluation TI, pas un PCB custom comme le nôtre, et seul le TX y a été validé. Le passé donne des indices, pas des solutions clé en main.
4. **Avoir deux cartes du même design est crucial pour un diagnostic différentiel**. Sans la carte PC qui fonctionne, on n'aurait jamais pu écarter le firmware et identifier le défaut comme étant local à la robot.
5. **Le pin par pin au multimètre est l'outil le plus puissant pour un debug analog**. Aucun test logiciel n'aurait pu identifier RBIAS comme étant le pin coupable — il fallait simplement mesurer.
6. **Les cycles de cal qui échouent endommagent réellement la chip** sur la durée. Une fois qu'on voit MARCSTATE bloqué à `0x04` et la chip qui chauffe, il faut arrêter de tester et chercher la cause ailleurs, sous peine de tuer le circuit analog progressivement.

---

### Carte PC — état après Phase 6

- ✅ Calibration radio CC1120 OK
- ✅ Émission TX sur 169.4 MHz validée (Phase 5)
- ✅ Stack applicative wirée et fonctionnelle (Phase 6)
- ✅ Console TUI (`host_tools/modem_console`) interagit avec le firmware via le framing UART : commandes envoyées, réponses reçues, timeouts firmware (CONNECT_TIMEOUT etc.) déclenchent correctement
- ⏸️ Test cross-board nécessite la carte robot avec son CC1120 remplacé

---

## Phase 7 — Remplacement du CC1120 robot et bring-up RX

### Action hardware
- Désouder l'ancien CC1120 du QFN32 de la carte robot avec air chaud (320-340°C, flux abondant)
- Nettoyage des pads + ressoudure d'un CC1120RGZR neuf
- Ressoudure simultanée de C201 sur AVDD_SYNTH (soudure froide identifiée Phase 7 précédente)

### Résultat
- ✅ PARTNUMBER = 0x48
- ✅ PARTVERSION = 0x23 (silicium B+, sans bug d'errata SWRZ039D)
- ✅ MARCSTATE = 0x01 stable, chip reste froide
- ✅ Calibration `SCAL` passe immédiatement
- ✅ TX fonctionnel des deux côtés

### Premier blocage cross-board : RX qui ne reçoit rien
TX émet bien (vérifié au SNOP / MARCSTATE = TX), mais aucun paquet n'arrive jamais en RX FIFO côté robot. La chip semble ne pas synchroniser.

---

## Phase 8 — Audit du registre table SmartRF Studio

### Symptôme
- Côté robot : `NUM_RXBYTES` reste à 0 alors qu'un paquet est émis à 50 cm de distance
- Aucune interruption GDO0
- MARCSTATE oscille entre RX (0x0D) et un état de récupération (0x1F)

### Démarche
Le registre table en place dans `cc1120.c` était une compilation manuelle de plusieurs sources : un export SmartRF Studio partiel + des valeurs réécrites au fil du debug. Hypothèse : incohérences entre les paramètres modem (modulation, déviation, BW) qui rendent la démodulation impossible.

### Inconsistances trouvées
- `MODCFG_DEV_E = 0x05` annoncé "2-GFSK" mais en réalité = 2-FSK
- Déviation indiquée 5 kHz dans les commentaires mais valeur registre = 80 kHz
- `CHAN_BW = 100 kHz` qui ne peut pas contenir une déviation à 80 kHz → démodulateur saturé
- `SYNC_CFG1` = 0x1F (SYNC_THR = 31, max) → faux syncs sur le bruit, chip oscillant en récupération

### Action
- Re-export SmartRF Studio frais pour le profil cible (169.4 MHz, 2-GFSK 4.8 kbps, dev 5 kHz, BW 100 kHz)
- Application **verbatim** des 64 lignes du fichier d'export, en conservant uniquement nos overrides applicatifs (`PKT_CFG1/2`, `RFEND_CFG1`, `IOCFG0 = 0x06 = PKT_SYNC_RXTX`, `FREQOFF_CFG`, `SETTLING_CFG`)

### Résultat
La démodulation devient cohérente : le DAC IQIC, le DCFILT, l'IF_MIX_CFG sont maintenant alignés avec la BW choisie. Mais **toujours aucune RX** au niveau application.

---

## Phase 9 — Trois bugs RX cumulés

L'investigation est passée par une vague de logs de debug instrumentés (compteurs INT0, RX_DONE, TX_DONE, RX_OVERFLOW, `att`/`LEN`/`bL`/`tr`/`bC`/`bN`/`bD`/`OK` poussés toutes les 2 secondes). Le pattern observé :

```
att=4 LEN=00 bL=0 tr=0 bC=0 bN=0 bD=0 OK=0
```

→ `radio_link_receive` est appelée 4 fois (driver détecte un paquet) mais lit toujours `LEN=0` → flush_rx → loop.

### Bug 9.1 — INT0 sur le mauvais front
**Symptôme** : `INT0` (le pin où GDO0 du CC1120 est routé sur RB1) firait au début de la synchronisation, quand la FIFO est encore vide. Le firmware classait alors l'événement en `TX_DONE` (`NUM_RXBYTES == 0`) au lieu de `RX_DONE`, et ne lisait jamais la fin du paquet.

**Cause** : `INTCONbits.INT0EDG = 1` (front montant) — IOCFG0 = `PKT_SYNC_RXTX` (0x06) passe à 1 au sync detect (début RX) et retombe à 0 à la fin du paquet. On voulait l'edge **descendant**.

**Fix** : `INTCONbits.INT0EDG = 0` (front descendant). L'IRQ tire alors à la fin du paquet, FIFO pleine.

### Bug 9.2 — `cc1120_flush_rx` laisse la chip en IDLE
**Symptôme** : après chaque paquet rejeté (CRC fail, len invalide, FIFO underrun), `cc1120_flush_rx()` strobe SIDLE + SFRX et retourne. La chip reste en IDLE jusqu'à la prochaine intervention soft. Si le driver ne re-strobe pas SRX, le récepteur est **mort** pour toute la suite.

**Fix** : ajouter `cc1120_strobe_srx()` à la fin de `flush_rx`. Sans ça, **un seul paquet corrompu** suffisait à tuer le RX du modem pour le reste de la session.

```c
void cc1120_flush_rx(void) {
    cc1120_set_idle();
    cc1120_strobe_sfrx();
    cc1120_strobe_srx();   /* critical: stay in RX after flush */
}
```

### Bug 9.3 — `RFEND_CFG1` mauvaise valeur (RXOFF_MODE = FSTXON)
**Symptôme initial** : après réception d'un beacon, la chip s'arrêtait de listener. La défensive ad-hoc qu'on avait fini par mettre dans la boucle main (strobe SRX toutes les 100 ms si MARCSTATE != RX) compensait, mais coûtait des paquets perdus pendant les TX en session (le défensif tombait pendant un TX_END/cal → abort).

**Cause finale (Phase 11)** : le commentaire dans `cc1120.c` décrivait un layout faux pour RFEND_CFG1 (bits 4:3 = RXOFF_MODE) alors que la datasheet TI **SWRU295E p.87** indique bits 5:4. Le 0x1F qu'on avait écrit donnait RXOFF_MODE = `01` = **FSTXON** (transmit-ready, RX coupée), pas RX.

**Fix immédiat (Phase 9, partiel)** : changer 0x0F → 0x1F (le bit qu'on touchait par hasard améliorait un peu la situation, mais sans résoudre).

→ Voir **Phase 11** pour le vrai fix.

### Résultat à la fin de Phase 9
- ✅ Liaison RF end-to-end fonctionnelle
- ✅ BEACON, CONNECT_REQ/OK, DATA, DATA_ACK, STATS PING/RESP qui s'échangent
- ⚠️ Stabilité fragile : `DISCONNECTED reason=0x03` (LINK_TIMEOUT) intermittents en session, scan qui ne voyait les robots qu'après un premier TX (connect)

Commit `8d372cd` : *"WIP: working RF link end-to-end, debug instrumentation still in place"*

---

## Phase 10 — Stack applicative et UX TUI

### Travaux fonctionnels
- Ajout de l'**ARQ stop-and-wait** sur les paquets DATA (déduplication, retransmits, timeout configurable)
- Beacon (toutes les 500 ms) émis en broadcast par le robot, parsé par REMOTE pour alimenter le panneau Discovered Robots
- STATS pushé en continu côté REMOTE en session (RSSI moyen exponentiel, PER en %, RTT mesuré par ping/pong RF)
- **TX_ACK** (UART_MSG = 0x85) : nouveau message émis vers le host à chaque DATA_ACK reçu, surface dans la TUI une confirmation "robot received last send"
- **Beacon keep-alive** : pendant une session, les beacons du robot connecté rafraîchissent `last_robot_rx_ms_` ; sans ça la watchdog LINK_LOST sautait si l'utilisateur ne tapait pas de `send` pendant 6 s
- **Flush du buffer USB-UART à l'ouverture** côté Python : sans ça, un `DISCONNECTED` qui était bufferisé par l'adaptateur FTDI/CP210x pendant que la TUI était fermée réapparaissait au boot suivant et confondait l'utilisateur

### Travaux UX
- Suppression des commandes `scan` / `scanstop` (le firmware scanne en permanence par défaut, `scan_reporting_enabled_ = true` à l'init)
- Suppression de la commande `stats` (panneau Connection mis à jour automatiquement toutes les 500 ms)
- Logs STATS retirés du feed Events (panneau Connection suffit, log devenait spam à 2 lignes/sec)
- RSSI affiché en **dBm absolus** dans la TUI (CC1120 retourne RSSI sur 1 octet signé centré sur -82 dBm pour 169 MHz → décodé en dBm réels)
- SCAN_RESULT n'affiche plus qu'**une ligne par robot découvert** (au lieu de spammer le log toutes les 500 ms)

---

## Phase 11 — Cause racine des disconnects 0x03 (RFEND_CFG bit layout)

### Symptôme persistant après Phase 10
Même corrections appliquées, l'utilisateur observait :
- À **-77 dBm de RSSI** (équivalent à 30 dB de marge au-dessus de la sensibilité CC1120) → toujours des `DISCONNECTED reason=0x03` sporadiques en pleine session
- Scan qui découvrait le robot **uniquement après une tentative de connect** (et pas dès le boot)
- Le défensif agressif (100 ms / != RX → SRX) faisait marcher scan mais cassait la stabilité en session (abortait les TX STATS-ping)
- Le défensif passif (500 ms / == IDLE) tenait la session mais cassait le scan-au-boot

### Analyse
Les deux symptômes pointaient vers un même problème : la chip **ne reste pas en RX par hardware**. Quelque chose la fait tomber régulièrement en FSTXON ou IDLE, et seul un kick logiciel répété la ramène en RX.

### Cause racine — relecture de RFEND_CFG dans le user guide TI
Le commentaire dans `cc1120.c` décrivait :
```
* RFEND_CFG1 = 0x1F:
*   bits 1:0 = 11 -> TXOFF_MODE = RX
*   bits 4:3 = 11 -> RXOFF_MODE = RX
```

Vérification ligne par ligne dans **SWRU295E pages 87-88** :

| Registre | Champ | Position bits réelle | Position bits supposée (commentaire faux) |
|---|---|---|---|
| `RFEND_CFG1` (0x29) | `RXOFF_MODE` | bits **5:4** | bits 4:3 |
| `RFEND_CFG1` (0x29) | `RX_TIME` | bits 3:1 | — |
| `RFEND_CFG0` (0x2A) | `TXOFF_MODE` | bits **5:4** | (registre jamais touché) |

Recalcul à la main :
- `RFEND_CFG1 = 0x1F = 0001 1111`
  - bits 5:4 = `01` = `RXOFF_MODE = FSTXON` ❌ (on voulait RX = `11`)
- `RFEND_CFG0 = 0x00` (jamais écrit) :
  - bits 5:4 = `00` = `TXOFF_MODE = IDLE` ❌ (on voulait RX = `11`)

→ Après **chaque** RX (= beacon, ACK, STATS-resp), la chip allait en FSTXON. Après **chaque** TX (= notre ping, notre DATA), la chip allait en IDLE. Le défensif SRX rattrapait, mais avec une race condition contre les TX en cours et contre la cal post-TX.

### Fix
```c
{ CC1120_RFEND_CFG1, 0x3Fu },  // RXOFF_MODE=11=RX, RX_TIME=max, RX_TIME_QUAL=1
{ CC1120_RFEND_CFG0, 0x30u },  // TXOFF_MODE=11=RX, CAL_END_WAKE_UP_EN=0
```

Maintenant, par configuration hardware :
- Après TX → cal (FS_AUTOCAL=01) → RX (auto via TXOFF_MODE)
- Après RX → reste en RX (auto via RXOFF_MODE)

### Simplification du firmware
Toute la machinerie défensive devient inutile. Restée comme filet de sécurité minimal (poll 500 ms, recovery uniquement si chip vraiment tombée en IDLE — uniquement après une erreur FIFO).

### Validation
- Scan affiche le robot dès le boot, sans aucune action utilisateur
- Sessions stables sur plusieurs minutes, plus de `DISCONNECTED 0x03` à RSSI nominal
- `send` confirmés par `TX_ACK` (vert gras dans la TUI)

Commit `6f57978` : *"Fix RFEND_CFG bit layout, drop debug instrumentation, clean up TUI"*

### Pourquoi ce bug était sournois
- Le **commentaire faux dans le code** matchait avec ce qu'on pensait avoir configuré → personne n'a relu le datasheet pour vérifier
- À haut RSSI (laboratoire, antennes proches), le défensif agressif rattrapait suffisamment vite pour donner l'illusion de stabilité
- Symptôme variait avec le timing de la boucle main → reproductible mais pas déterministe
- La régression "scan vs session" était impossible à arbitrer sans toucher au défensif → toujours **un** des deux symptômes apparaissait

→ Leçon : un commentaire qui dit "ce bit fait X" n'est pas un test. Toujours recroiser avec la datasheet quand le comportement ne colle pas avec ce que prétend le code.

---

## État final du projet — 2026-06-05

### Carte PC (REMOTE)
- ✅ Radio CC1120 opérationnelle (cal stable, TX/RX confirmés)
- ✅ Stack applicative complète (scan, connect, ARQ DATA, STATS, disconnect)
- ✅ TUI Python (`host_tools/modem_console`) full-featured
- ✅ Liaison stable plusieurs minutes en session

### Carte robot
- ✅ CC1120 remplacé, opérationnel
- ✅ Mêmes firmware partagé (différenciation `MODEM_ROLE = ROBOT`)
- ✅ Beacon broadcast, CONNECT/DATA/DATA_ACK/STATS gérés
- ✅ UART transparent côté MCU robot (le robot voit le texte tapé sur le PC distant et vice-versa)

### Performances mesurées
- **Sensibilité** : ~-110 dBm typique (datasheet CC1120, conforme à notre test)
- **Marge à 2 m, intérieur** : RSSI ~-77 dBm → 30 dB au-dessus du seuil de coupure
- **PER** : ~0 % en marge confortable
- **RTT** : 100-200 ms typique (1 ping/sec)
- **Débit utile** : limité par 4.8 kbps + ARQ stop-and-wait → ~few hundred bytes/sec

### Améliorations connues mais non implémentées
- Mode **promiscuous** côté robot (actuellement filtre sur `dst_id == local_id`) pour multi-robots
- **Diversity / freq-hopping** sur la bande 169.4 MHz pour la robustesse en milieu bruité
- Réduction du `BEACON_INTERVAL_MS` (500 ms) en mode "actif" pour mieux soutenir une session DATA intensive
- Mise en cache de la cal sur l'EEPROM PIC (évite ~700 µs de cal à chaque transition IDLE→RX)

---

*Dernière mise à jour : 2026-06-05 — Liaison RF stable bout en bout, RFEND_CFG bit layout corrigé.*
