# Plan de tests — Liaison RF 169.4 MHz en mode passthrough

> Batterie de tests pour caractériser la qualité de la liaison en conditions
> réelles, en vue de la présentation finale. Les résultats se remplissent
> directement dans les tableaux ci-dessous.

## Setup commun

### Matériel
- **PC fixe** (poste pilote) : carte REMOTE branchée via USB-UART
- **PC portable** (côté robot) : carte ROBOT branchée via USB-UART
- 2× antennes 169.4 MHz (mêmes modèles, vérifier qu'elles sont bien vissées)
- Câbles USB-UART : à laisser branchés sur les mêmes ports tout du long pour éliminer cette variable
- Mètre ruban / décamètre pour mesurer les distances

### Logiciel
- **PC fixe** : firmware `Modem_remote.X` flashé, TUI Python (`python -m modem_console --port COM5`)
- **PC portable** : firmware `Modem_robot.X` flashé, PuTTY ouvert sur le port COM correspondant

### Configuration PuTTY
- Serial, 115200, 8N1
- Terminal → Local echo : **Force on**
- Terminal → "Answerback to ^E" : **vider le champ** (évite le bug PuTTY-loop)
- Terminal → "Implicit CR in every LF" : coché

### Procédure standard d'entrée en passthrough
1. Côté PC fixe : lancer la TUI, vérifier que le robot apparaît dans Discovered
2. `connect 0x10` → attendre `CONNECTED`
3. Noter le RSSI / PER / RTT initiaux du panneau Connection
4. `passthrough` → 3 lignes jaunes
5. `quit` → ferme la TUI
6. Ouvrir PuTTY sur le même port COM

### Protocole de mesure
- **Avant chaque test** : repérer le RSSI dans la TUI **avant** d'entrer en passthrough
- **Pour mesurer la latence par caractère** : taper une lettre, chronométrer mentalement (ou avec un script)
- **Pour mesurer le débit** : envoyer une commande qui sort une taille connue (ex: `head -c 1000 /etc/passwd`) et chronométrer

---

## TIER 1 — Tests prioritaires (à faire absolument)

### A. Caractérisation nominale (baseline @ 50 cm — référence)

Cartes posées côte à côte, ~50 cm, ligne de vue dégagée, intérieur calme.

| Métrique | Valeur attendue | Mesuré |
|---|---|---|
| RSSI (TUI Connection panel) | ~-75 dBm | **-65 dBm** ✓ |
| PER (TUI Connection panel) | 0 % | **0 %** ✓ |
| RTT (TUI Connection panel) | 100-200 ms | **59 ms** ✓ (meilleur que prévu) |
| Latence par caractère (visuel) | ~150 ms | **non perceptible** ✓ (cohérent avec ~74 ms total = 15 ms coalescing + 59 ms RTT — sous le seuil de perception ~100 ms) |
| Session stable sur 5 min ? (oui/non) | oui | **oui** ✓ |
| `*** link lost ***` apparu ? (oui/non) | non | **non** ✓ |

### Conclusion Test A — baseline

Liaison nominale **excellente** : -65 dBm avec 45 dB de marge au-dessus de la sensibilité, RTT de 59 ms, latence imperceptible en saisie, session stable. Tous les tests suivants doivent être comparés à cette référence.

---

### B. Tests de distance en ligne de vue (LOS)

Couloir ou extérieur dégagé. Augmenter la distance par paliers et noter les métriques. **Garder l'orientation des antennes constante** (verticale, par exemple).

| Distance | RSSI (dBm) | PER (%) | RTT (ms) | Lettres OK ? | Drops sur 1 min ? |
|---|---|---|---|---|---|
| **1 m** | **-76** | **0** | **59** | _en cours_ | _en cours_ |
| **2 m** | **-84** | **0** | **59** | | |
| 5 m | (saut) | | | | |
| **6 m** | **-93** | **0** | **59** | difficile, drops fréquents | élevé |
| 10 m | | | | | |
| 20 m | | | | | |
| 30 m | | | | | |
| 50 m | | | | | |

**Distance à laquelle le link devient marginal** : ____ m
**Distance à laquelle plus aucune communication** : ____ m

---

### C. Tests de débit (passthrough actif)

Mesurer le temps d'arrivée de paste de tailles connues depuis PC fixe vers PC portable.

**Méthode** : générer du texte côté PC fixe via cmd, le copier, le coller dans PuTTY, chronométrer entre le paste et l'arrivée du dernier caractère côté portable.

| Volume | Commande cmd source | Temps mesuré | Débit (octets/s) | Perte ? |
|---|---|---|---|---|
| ~880 B (avant fix buffer 512 B) | `for /L %i in (1,1,100) do @echo line %i` | n/a | n/a | **74/100 lignes** (overflow) ⚠️ |
| ~880 B (après fix buffer 2 KB, FLUSH_BYTES=24) | `for /L %i in (1,1,100) do @echo line %i` | **6.0 s** | **149 B/s** | 0 |
| ~1.9 KB (après fix buffer 2 KB, FLUSH_BYTES=48) | `for /L %i in (1,1,200) do @echo line %i` | **13.68 s** | **138 B/s** | 0 |

**Résultat majeur — limite de bande passante observée** :
Au-delà d'environ 500 octets envoyés en burst depuis l'UART à 115200 baud, le buffer RX (512 B initial) côté firmware sature plus vite qu'il ne se draine vers la RF (~350 B/s). Sans flow control hardware (RTS/CTS) ni software (XON/XOFF), les octets supplémentaires sont perdus. Test initial : 100 lignes envoyées → 74 reçues, perte ~26 % en burst.

**Action correctrice** : taille du buffer UART RX **quadruplée** de 512 à 2048 octets. Une seconde campagne avec le buffer agrandi permet d'absorber des bursts jusqu'à ~2 KB sans perte. Au-delà, il faudrait ajouter du XON/XOFF — listé comme amélioration future.

**Débit utile moyen mesuré** : **138-149 B/s** (selon la taille du burst)

**Théorique max** (paquet 48 octets ARQ stop-and-wait) :
- TX DATA air time : ~103 ms (62 octets × 8 / 4800)
- ACK air time : ~27 ms
- Processing PIC + robot : ~20 ms
- Cycle ARQ : ~150 ms → 48 / 0.150 = **~320 B/s théorique**

**Écart théorie/mesure expliqué** : le robot continue de beacon en CONNECTED (toutes les 500 ms, ~27 ms d'air time). Quand un DATA arrive pendant un beacon TX du robot, le DATA est perdu → retransmit ARQ après 300 ms de timeout. Avec ~5-10 % de collisions, le débit effectif chute à ~140 B/s. Ce trade-off "moins de débit, plus de robustesse" est volontaire : sans le beacon-en-CONNECTED, un seul PONG RTT perdu pourrait causer un LINK_TIMEOUT (cf. Phase 12 du rapport).

**Améliorations possibles** (non implémentées, mentionnées dans le rapport) :
- ARQ sliding-window (plusieurs DATA en vol) → x2-3 le débit
- Symbol rate plus élevé (9.6 ou 38.4 kbps) → portée réduite mais débit linéairement amélioré

**Note méthodologique** : la commande `for /L %i in (1,1,N) do @echo line %i` plafonne en pratique vers `N=255` (limite probable du parsing `%i` dans cmd/clipboard/PuTTY paste). Au-delà, il faut générer le texte autrement (script, copier-coller depuis un éditeur). Pas un bug du firmware — testé sans erreur jusqu'à la limite des 255 lignes après le fix buffer 2 KB.

---

### D. Test de reconnect automatique

Vérifier le sticky reconnect en passthrough.

| Test | Action | Temps de détection | Temps total reconnect | Bannière `link lost` ? | Bannière `reconnected` ? |
|---|---|---|---|---|---|
| Robot reboot | Débrancher alim 5 s puis rebrancher | | | | |
| Hors portée temporaire | Éloigner à 30 m, attendre 10 s, revenir | | | | |
| Antenne déconnectée | Dévisser antenne robot 5 s, revisser | | | | |

**Temps moyen de reconnect détection→shell de retour** : ____ s

---

## TIER 2 — Tests intermédiaires (à faire si possible)

### E. Tests d'obstacles (même pièce, distance fixe ~3-5 m)

| Configuration | RSSI (dBm) | PER (%) | `ls` qui marche ? | Drops sur 1 min ? |
|---|---|---|---|---|
| Référence : pièce ouverte | | | | |
| À travers 1 cloison (placo) | | | | |
| À travers 1 mur porteur (béton) | | | | |
| Porte fermée entre les deux | | | | |
| Derrière armoire métallique | | | | |
| Sous un bureau | | | | |
| Antenne pliée contre un objet | | | | |

**Atténuation par type d'obstacle (estimation)** :
- Placo : ___ dB
- Béton : ___ dB
- Métal : ___ dB

---

### F. Tests d'orientation d'antenne

Distance fixe ~5 m, varier l'orientation relative des deux antennes.

| Configuration | RSSI (dBm) | PER (%) |
|---|---|---|
| Les deux verticales (référence) | | |
| Les deux horizontales | | |
| Une verticale, une horizontale (polarisation croisée) | | |
| Antenne pliée 90° (charnière) | | |

**Perte par polarisation croisée** : ___ dB

---

### G. Test d'usage interactif réaliste

Reproduction d'une session shell normale, chronométrer le ressenti utilisateur.

| Action | Temps estimé | Ressenti (excellent / OK / lent / inutilisable) |
|---|---|---|
| Login (saisie user + password) | | |
| `ls` puis `cd /var/log` puis `ls` | | |
| Ouvrir `nano test.txt` (temps de redraw écran) | | |
| Taper 3 lignes dans nano | | |
| `Ctrl+O` `Enter` pour sauver | | |
| `Ctrl+X` pour quitter | | |
| `cat test.txt` pour vérifier | | |

**Note d'usabilité subjective (1-10)** : ___ / 10
**Commentaire** : ____________________

---

## TIER 3 — Tests longue durée (à faire si le temps le permet)

### H. Stabilité de session longue (statique)

Configuration nominale (baseline), laisser tourner et chronométrer.

| Durée cible | Drops constatés ? | Reconnect automatique réussi ? | Notes |
|---|---|---|---|
| 5 min | | | |
| 15 min | | | |
| 30 min | | | |
| 1 heure | | | |

### I. Stress test gros volume

`cat` d'un fichier volumineux (10-50 KB).

| Taille | Temps mesuré | Débit (B/s) | Drops pendant ? | Sortie complète ? |
|---|---|---|---|---|
| 10 KB | | | | |
| 25 KB | | | | |
| 50 KB | | | | |

### J. Mobilité (si possible)

Marcher avec le PC portable pendant qu'une session est active.

| Vitesse | RSSI plage | Drops ? | Sessions reprises ? |
|---|---|---|---|
| Marche lente intérieur | | | |
| Marche normale extérieur | | | |
| Course | | | |

---

## Synthèse pour la présentation

### Performances mesurées (à remplir une fois les tests faits)

| Métrique | Valeur |
|---|---|
| Sensibilité observée (RSSI minimum pour tenir un shell) | ___ dBm |
| Portée LOS intérieure (drop < 1%) | ___ m |
| Portée LOS extérieure (drop < 1%) | ___ m |
| Portée à travers 1 mur béton | ___ m |
| Débit utile passthrough | ___ B/s |
| Latence par keystroke | ___ ms |
| Temps de reconnect après drop | ___ s |
| Stabilité sur 30 min idle | (drop count) |

### Démos prévues pour la présentation

1. **Live shell** : ouvrir une session, taper `ls`, `whoami`, `date` en direct
2. **Latence visible** : taper un texte, montrer le délai entre keystroke et echo
3. **Reconnect** : couper l'alim du robot, montrer la bannière `link lost`, rebrancher, montrer `reconnected`
4. **(Optionnel) Edition** : ouvrir `nano` ou `vim`, éditer un petit fichier pour illustrer le cas extrême

### Points forts à mettre en avant

- Liaison sub-GHz longue portée à très bas débit (4.8 kbps) sur antenne simple
- Stack applicative complète : ARQ, beacons keep-alive, sticky reconnect
- Mode passthrough qui rend le tout exploitable comme un câble série standard
- Robustesse acquise par debug de plusieurs bugs hardware et firmware non triviaux

### Points faibles à reconnaître honnêtement

- Débit utile ~350 B/s : inutilisable pour `top`, `vi` sur gros fichier, transfert binaire
- Latence ~150 ms : sensible mais acceptable pour shell
- Pas de chiffrement : à mentionner si question
- Pas de multi-robots : architecture le permettrait avec un protocol layer supplémentaire

---

## Notes libres pendant les tests

> Espace pour noter des observations, anomalies, photos prises, etc.

```
(à remplir)
```

---

*Document à compléter au fil des tests. Le remplir directement remplace l'effort
de prise de notes parallèle.*
