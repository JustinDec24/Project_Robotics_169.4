# Modem Console — interface PC pour le modem RF 169.4 MHz

TUI (interface terminal en plein écran) qui parle au firmware côté PC
(`Modem_remote.X`) via le port série virtuel exposé par le FT231XQ-R.

Affiche en temps réel :

- l'état de la connexion radio avec le robot
- les robots détectés en continu (scan automatique, pas besoin de le déclencher)
- les statistiques de lien (RSSI en dBm, taux d'erreurs paquet, RTT) mises à jour toutes les 500 ms
- un log d'événements horodaté (TX / RX / CMD / ACK / LOG / erreurs)
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

| Commande              | Effet                                                |
| --------------------- | ---------------------------------------------------- |
| `connect <id>`        | se connecte au robot d'ID donné (ex `connect 0x10`)  |
| `disconnect`          | coupe la connexion                                   |
| `send "<texte>"`      | envoie un paquet DATA au robot connecté (avec ARQ)   |
| `clear`               | vide le log                                          |
| `help`                | rappelle la liste des commandes                      |
| `quit` / `exit`       | sort de la console                                   |

Le **scan est automatique** — dès la TUI lancée, tous les robots à portée
apparaissent dans le panneau Discovered Robots (le firmware émet un
`SCAN_RESULT` à chaque beacon reçu). Les **stats** (RSSI / PER / RTT) sont
mises à jour toutes les 500 ms automatiquement dans le panneau Connection.

Quand tu envoies un `send "..."`, un message **`ACK robot received last send`**
en vert confirme que le robot a bien reçu et acquitté le paquet (via le
nouveau message `UART_MSG_TX_ACK` 0x85 émis par le firmware).

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
- **Aucun robot dans Discovered Robots** : vérifie que la carte robot est
  sous tension, que sa LED clignote (toggle à chaque beacon TX), que les
  deux antennes 169 MHz sont en place, et qu'aucune des deux cartes n'est
  en reset (regarder le banner `[remote] modem firmware booting`).
- **`DISCONNECTED reason=0x03` au boot avant tout connect** : c'est un
  frame bufférisé par l'adaptateur USB-UART d'un run précédent. Depuis le
  fix de [serial_link.py](modem_console/serial_link.py) le buffer est
  flushé à l'ouverture, donc ça ne devrait plus arriver.
- **Banner `[remote] modem firmware booting`** dans Events : c'est le print
  brut UART du `main()` au boot, normal. Apparaît une fois par power-on.
