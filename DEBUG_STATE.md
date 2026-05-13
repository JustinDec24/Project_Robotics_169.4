# Bring-up du modem REMOTE — résolu ✅

## Résumé

Premier bring-up de la carte PIC18F26Q10 + CC1120. La CC1120 ne répondait pas
au SPI : tous les reads renvoyaient `0xFF` (extended) ou `0x00` (standard),
les writes ne sticken pas, malgré que le quartz tourne (XOSC) et que toutes
les pins d'alim soient à 3.3 V.

**Root cause : 3 bugs firmware (PPS + master clock feedback)**, pas un défaut
hardware. La CC1120 était saine tout du long.

## Bugs trouvés et corrigés

### 1. Codes PPS d'output faux (PIC18F26Q10 datasheet DS40001996A Table 17-2)

| Signal | Avant (FAUX) | Après (correct) | Ce que faisait l'ancien code |
|---|---|---|---|
| `RC3PPS` (SCK1) | `0x10` | `0x0F` | Sortait SDO1 sur RC3 au lieu de SCK1 |
| `RC5PPS` (SDO1) | `0x11` | `0x10` | Sortait MSSP2 SCK sur RC5 (peripheral non configuré) |

Confirmation: code 0x10 = `MSSP1 SDO/SDA`, 0x11 = `MSSP2 SCK/SCL`, 0x0F =
`MSSP1 SCK/SCL` selon datasheet officielle.

### 2. `SSP1CLKPPS` non initialisé

En master mode, le PIC18 MSSP doit avoir `SSPxCLKPPS` configuré sur la même
broche que la sortie SCK, parce que le module relit sa propre sortie d'horloge
pour cadencer son shift register. Quote datasheet §27.2.1 :

> *"In Master mode the clock signal output to the SCK pin is also the clock
> signal input to the peripheral. The pin selected for output with the RxyPPS
> register must also be selected as the peripheral input with the SSPxCLKPPS
> register."*

Ajouté : `SSP1CLKPPS = 0x13` (RC3) dans `board_spi_hw_init()`.

### 3. SMP=1 au lieu de SMP=0 en master mode 0

L'ancien code utilisait `SSP1STAT = 0xC0` (SMP=1, CKE=1). Pour le mode SPI 0,
SMP=0 (échantillonner MISO au milieu du bit time = front montant de SCK) est
correct. SMP=1 échantillonne sur le front descendant suivant, qui est le
moment où le slave change MISO — race condition.

Corrigé en `SSP1STAT = 0x40` (SMP=0, CKE=1).

### 4. (Bonus) `INTCON2bits.INTEDG0` → `INTCONbits.INT0EDG`

Le PIC18F26Q10 n'a pas `INTCON2`. Sur la série Q, le bit de selection de
front pour INT0 est `INTCONbits.INT0EDG` (et non `INTCON2bits.INTEDG0`
comme sur les vieux PIC18). Le commentaire dans le code anticipait
d'ailleurs le changement de nom.

## Bugs hardware identifiés et patchés

### A. RESET_N pin 2 non tirée à VDD_REG

Le schéma laissait penser que la pin était tirée, mais en pratique elle
flottait. Sans pull-up et sans pull-up interne documenté chez TI, le chip
booterait potentiellement en reset permanent.

**Fix appliqué** : fil manuel direct entre pin 2 CC1120 et VDD_REG.
**Fix pour PCB v2** : ajouter une résistance pull-up 100 kΩ sur le schéma.

### B. Load caps quartz 32 MHz sous-dimensionnées

C311 / C301 étaient à 15 pF, alors que le quartz ABM8-32.000MHZ-B2-T a
CL=18 pF (datasheet). Le CL effectif était ~10.5 pF au lieu de 18 pF →
oscillation marginale.

**Fix appliqué** : caps changées à 33 pF (CL effectif ≈ 19.5 pF, OK).
**Fix optimal pour PCB v2** : 27 pF (CL effectif ≈ 18 pF nominal).

## Validation finale

Output de `cc1120_test` après tous les fixes :

```
[D] PARTNUMBER[0]: 0x48  <-- CC1120 OK !
[D] PARTNUMBER[1]: 0x48
[D] PARTNUMBER[2]: 0x48
[E] PARTVERSION:    0x23  (factory revision)
[F] MARCSTATE:      0x41  (settled IDLE)
[G] IOCFG3:         0x06  (reset value)
[G] SYNC0:          0xDE  (reset value)
[H] Write 0xAA -> read 0xAA   (write path works)
```

La CC1120 est entièrement fonctionnelle.

## Recommandations pour PCB v2

1. **Pull-up 100 kΩ** sur RESET_N → VDD_REG
2. **Load caps 27 pF** sur le quartz (vs 33 pF actuel, vs 15 pF original)
3. *(Optionnel)* router RESET_N vers une GPIO du PIC, pour permettre un
   hardware reset piloté par firmware si besoin

## Patches debug dans le code

Les fichiers suivants contiennent des `uart_write_bytes(...)` ajoutés pour
debug. À retirer une fois confirmé que le modem complet fonctionne :

- `firmware/modem/main.c` : 4 logs de boot (`BOOT/SPI ok/...`)
- `firmware/modem/app/app.c` (case `APP_INIT`) : 3 logs autour de `cc1120_init`
- `firmware/modem/radio/cc1120.c` : bloc `--- SPI sanity ---` complet
- `firmware/modem/radio/cc1120.c` : `#include "../drivers/uart.h"` ajouté

## Fichiers à GARDER (vrais correctifs)

- `firmware/modem/board/board.c` : codes PPS, `SSP1CLKPPS`, `INT0EDG`
- `firmware/cc1120_test/` : projet de test SPI minimal (utile pour debug futur)
