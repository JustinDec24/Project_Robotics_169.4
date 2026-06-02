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

| Programme | But | Quoi il exécute |
|---|---|---|
| **Phase 4** | Init complet + cal | Reset → registres → SCAL → lit MARCSTATE et FS_VCO/CHP |
| **Phase 4b** | Vérifier que la chip est vivante (no SCAL) | Reset → lit PARTNUMBER, PARTVERSION, 11 valeurs reset, halt 30 s en IDLE |
| **Phase 4c** | Sonder le sous-système analog | Test XOSC_STABLE via GDO0, sanité SPI, trace MARCSTATE pendant cal avec abort rapide |
| **Phase 5** *(prévu)* | Tester TX sur carte PC | Écriture TX FIFO, strobe STX, vérification NUM_TXBYTES + MARCSTATE |
| **Phase 6** *(prévu)* | Activer la stack applicative | `app_init()` + `app_task()` en boucle, commandes UART parsing |

---

## Leçons apprises

1. **Toujours valider chaque couche avant de monter** : le bring-up SPI a pris du temps à cause des PPS, et chaque doute non levé en bas du stack se paye au triple en haut.
2. **Comparer systématiquement les adresses de registres avec la datasheet officielle**, pas un header copié d'une autre origine. Beaucoup d'erreurs trouvées dans `cc1120_regs.h` lors de l'audit.
3. **Le code de référence "qui marchait" l'année passée n'était pas une garantie absolue** — RF169Hz.X utilisait une carte d'évaluation TI, pas un PCB custom comme le nôtre, et seul le TX y a été validé. Le passé donne des indices, pas des solutions clé en main.
4. **Avoir deux cartes du même design est crucial pour un diagnostic différentiel**. Sans la carte PC qui fonctionne, on n'aurait jamais pu écarter le firmware et identifier le défaut comme étant local à la robot.
5. **Le pin par pin au multimètre est l'outil le plus puissant pour un debug analog**. Aucun test logiciel n'aurait pu identifier RBIAS comme étant le pin coupable — il fallait simplement mesurer.
6. **Les cycles de cal qui échouent endommagent réellement la chip** sur la durée. Une fois qu'on voit MARCSTATE bloqué à `0x04` et la chip qui chauffe, il faut arrêter de tester et chercher la cause ailleurs, sous peine de tuer le circuit analog progressivement.

---

*Dernière mise à jour : 2026-06-02*
