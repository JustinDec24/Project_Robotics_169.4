# Project_Robotics_169.4

Liaison radio bidirectionnelle **169.4 MHz** entre un PC (poste de pilotage) et un robot amphibien, basée sur :
- **PIC18F26Q10** (MCU)
- **TI CC1120** (transceiver sub-GHz)
- PCB **custom** (deux exemplaires identiques, différenciés par le rôle au moment de la compilation)

## État du projet

✅ **Les deux cartes sont opérationnelles.** Liaison RF stable, scan + connect + transfert de données fiabilisé par ARQ, TUI Python qui pilote la carte côté PC.

Voir le rapport de synthèse **[RAPPORT_FINAL.md](RAPPORT_FINAL.md)** pour la vue d'ensemble du projet, le journal chronologique du bring-up dans **[RAPPORT_DEBUG.md](RAPPORT_DEBUG.md)**, et la procédure de test pas-à-pas dans **[TESTING.md](TESTING.md)**.

## Chaîne de communication

```
[PC] ─UART framé─> [Modem REMOTE] ─RF 169.4 MHz─> [Modem ROBOT] ─UART transparent─> [PC robot]
       (TUI Python)   (PIC + CC1120)                (PIC + CC1120)                  (terminal)
```

- UART hôte ↔ REMOTE : framing binaire `[0xAA][0x55][LEN][TYPE][PAYLOAD][CRC8]` (115200 8N1)
- UART ROBOT ↔ PC robot : transparent (octets bruts, terminal série classique)

## Structure du dépôt

```
firmware/
├── modem/                    # sources C partagées REMOTE + ROBOT
│   ├── main.c                # super-loop
│   ├── config.h              # constantes (NET_ID, timeouts, sizes)
│   ├── app/                  # state machines applicatives
│   ├── board/                # abstraction PIC18F26Q10
│   ├── drivers/              # UART, SPI, Timer0 (tick 1 ms)
│   ├── protocol/             # framing UART hôte (encode/decode)
│   ├── radio/                # driver CC1120 + radio_link + ARQ
│   └── util/                 # ring buffer, CRC-8
├── Modem_remote.X/           # projet MPLAB X (MODEM_ROLE=1)
└── Modem_robot.X/            # projet MPLAB X (MODEM_ROLE=2)

host_tools/
└── modem_console/            # TUI Python (Textual + pyserial)
```

## Démarrer

### Firmware (MPLAB X + XC8)

1. Ouvrir `firmware/Modem_remote.X/` (carte PC) ou `firmware/Modem_robot.X/` (carte robot)
2. Vérifier **Project Properties → XC8 → Define macros** : `MODEM_ROLE=1` pour REMOTE, `MODEM_ROLE=2` pour ROBOT
3. Build + Make and Program Device (PICkit 4)

### Hôte (TUI Python)

```bash
cd host_tools/modem_console
pip install -e .
python -m modem_console --port COM5 --baud 115200
```

Détails complets dans [TESTING.md](TESTING.md) et [host_tools/modem_console/README.md](host_tools/modem_console/README.md).

## Spécifications RF

| Paramètre | Valeur |
|---|---|
| Fréquence | 169.4 MHz |
| Modulation | 2-GFSK |
| Débit symboles | 4.8 kbps |
| Déviation | 5 kHz |
| Bande passante RX | 100 kHz |
| Puissance PA | ~14 dBm (25 mW) |
| Sensibilité (mesurée) | ~-110 dBm @ 1 % PER |

## Commandes de la TUI

| Commande | Action |
|---|---|
| `connect <id>` | Se connecter à un robot (e.g. `connect 0x10`) |
| `disconnect` | Couper la session |
| `send "<texte>"` | Envoyer une trame DATA fiabilisée (ARQ) |
| `clear` | Vider l'event log |
| `help` | Lister les commandes |
| `quit` / `exit` | Sortir |

Le scan est **automatique** : tous les robots dans la portée sont listés en continu dans le panneau Discovered Robots dès le lancement de la TUI.
