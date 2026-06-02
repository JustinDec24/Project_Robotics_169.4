# Modem Console — interface PC pour le modem RF 169.4 MHz

TUI (interface terminal en plein écran) qui parle au firmware côté PC
(`Modem_remote.X`) via le port série virtuel exposé par le FT231XQ-R.

Affiche en temps réel :

- l'état de la connexion radio avec le robot
- les robots détectés lors d'un scan
- les statistiques de lien (RSSI moyen, taux d'erreurs paquet, RTT)
- un log d'événements horodaté (TX / RX / LOG / erreurs)
- un prompt de commandes interactif

## Installation

Pré-requis : **Python 3.9 ou plus récent**.

Depuis le dossier `host_tools/modem_console/` :

```powershell
# (recommandé) créer un venv pour ne pas polluer le Python système
python -m venv .venv
.\.venv\Scripts\Activate.ps1

# installer le tool en mode éditable + ses deps (pyserial, textual)
pip install -e .
```

Sur Linux / macOS, remplace `.\.venv\Scripts\Activate.ps1` par
`source .venv/bin/activate`.

## Utilisation

Pour voir la liste des ports série dispos :

```powershell
python -m modem_console --list
```

Pour lancer la console sur le port du modem PC (ex. `COM4` sous Windows) :

```powershell
python -m modem_console --port COM4
```

Ou, si l'install a créé le script `modem-console` dans le PATH du venv :

```powershell
modem-console --port COM4
```

La baudrate par défaut est 115200 (= `UART_BAUD_DEFAULT` côté firmware). Si
elle est changée côté MCU, ajuster avec `--baud`.

## Commandes interactives

Une fois la console lancée, tape les commandes dans la barre du bas :

| Commande              | Effet                                          |
| --------------------- | ---------------------------------------------- |
| `scan`                | démarre un scan des robots à portée            |
| `scanstop`            | arrête le scan                                 |
| `connect <id>`        | se connecte au robot d'ID donné (ex `0x10`)    |
| `disconnect`          | coupe la connexion                             |
| `stats`               | demande les stats du lien                      |
| `send "<texte>"`      | envoie un paquet DATA au robot connecté        |
| `clear`               | vide le log                                    |
| `help`                | rappelle la liste des commandes                |
| `quit`                | sort de la console                             |

Raccourcis clavier : `Ctrl+C` quitte, `Ctrl+L` efface le log.

## Architecture interne

| Fichier               | Rôle                                                 |
| --------------------- | ---------------------------------------------------- |
| `protocol.py`         | CRC-8, encodage/décodage du framing UART, types msg  |
| `serial_link.py`      | Thread de lecture série, file d'événements vers l'UI |
| `tui.py`              | App Textual : layout + widgets + dispatch commandes  |
| `__main__.py`         | Point d'entrée CLI (`python -m modem_console`)       |

Le protocole exact (sync bytes, CRC, types de messages) est miroir du code
firmware dans `firmware/modem/protocol/protocol.{c,h}`.

## Dépannage

- **« Could not open port »** : un autre programme (Tera Term, MPLAB IPE,
  un terminal série…) tient déjà le port. Le fermer puis relancer.
- **Le scan trouve rien** : vérifie que la carte robot est sous tension,
  alimentée et que la PC voit bien ses logs UART (`LOG` events dans le
  panneau de droite).
- **Beaucoup d'événements `UART`** au démarrage : c'est normal, ce sont les
  prints non-encadrés du firmware (banner de boot, debug). Les messages
  protocole apparaîtront en `RX` une fois l'app firmware passée en mode
  protocole.
