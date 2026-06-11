# Plan de tests — Liaison RF 169.4 MHz en mode passthrough

> Batterie de tests pour caractériser la qualité de la liaison en conditions
> réelles, en vue de la présentation finale. Les résultats se remplissent
> directement dans les tableaux ci-dessous.

---

## TL;DR — chiffres clés pour la présentation

| Indicateur | Valeur |
|---|---|
| **Portée intérieure stable** (PER 0 %) | **≥ 15 m** |
| RSSI à 1 m / 15 m | -76 dBm / -95 dBm |
| Sensibilité chip narrow BW (datasheet) | -113 dBm |
| Marge restante à 15 m | ~18 dB → théorique > 25 m |
| RTT typique | **60 ms** |
| Latence interactive perçue | **imperceptible** (~75 ms total) |
| Débit utile en burst | **138-149 B/s** (~140 B/s soutenu) |
| Burst sans perte | **2 KB** (= 200 lignes consécutives, ~14 s) |
| Reconnect automatique | **fonctionnel** (SESSION + PASSTHROUGH) |
| 9 améliorations en revue finale | P0 bugs, robustesse radio (CHAN_BW + IQIC), observabilité (Retries), recovery |

**Démos prévues** :
1. Reconnect auto (couper alim robot, regarder la session revenir seule)
2. Bannières `*** link lost *** / *** reconnected ***` dans PuTTY en mode passthrough
3. Paste de 200 lignes qui arrive intact (preuve du buffer 2 KB)
4. RSSI / PER / RTT / **Retries** en temps réel dans la TUI

---

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
| **6 m** | **-93** | **0** | **59** | difficile, drops fréquents | élevé _(avant fix BW)_ |
| **6 m** | **-94** | **0** | **60** | **stable** ✓ | faible _(après BW 25 kHz + IQIC)_ |
| **8-9 m** | **-96** | **0** | **60** | **stable** ✓ | très faible |
| **15 m** | **-95** | **0** | **60** | **stable** ✓ | très faible — _portée maximale testée_ |

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

| Test | Action | Résultat |
|---|---|---|
| Robot reboot | Débrancher alim 5 s puis rebrancher | ✓ OK — reconnect automatique réussi |
| Sticky reconnect en SESSION TUI | Mêmes conditions, vue depuis la TUI | ✓ OK — `DISCONNECTED` → `CONNECTED` automatique |
| Sticky reconnect en PASSTHROUGH | Bannières `*** link lost ***` / `*** reconnected ***` | ✓ OK — visible dans PuTTY |

**Conclusion** : la double couche d'auto-reconnect (SESSION + PASSTHROUGH) fonctionne nominalement. L'utilisateur n'a jamais à retaper `connect` après une coupure transitoire.

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

### Performances mesurées — résultats finaux

| Métrique | Valeur mesurée |
|---|---|
| RSSI à 50 cm (baseline) | **-65 dBm** |
| RSSI à 1 m | -76 dBm |
| RSSI à 2 m | -84 dBm |
| RSSI à 8-9 m | -96 dBm |
| RSSI à 15 m (portée max testée) | **-95 dBm** (multipath favorable) |
| **Portée LOS intérieure stable** | **≥ 15 m, PER 0 %, drops nuls** |
| Marge encore disponible à 15 m | ~18 dB → portée théorique > 25-30 m |
| Sensibilité chip (datasheet, narrow BW) | -113 dBm |
| Latence par keystroke | **non perceptible** (~75 ms total : 15 ms coalescing + 60 ms RTT) |
| RTT mesuré (panneau Connection) | **59-60 ms** |
| Débit utile en burst | **138-149 B/s** sustained |
| Buffer UART RX max sans perte | **2 KB** (after fix : 100→200 lignes sans drop) |
| Reconnect auto après drop | **fonctionnel** sous SESSION et PASSTHROUGH |

### Démos à prévoir pendant la présentation

| Démo | Effet visuel | Comment faire |
|---|---|---|
| **1. Reconnect automatique** | Très impressionnant | Couper alim robot 5 s, montrer `DISCONNECTED` puis `CONNECTED` apparaître seuls dans Events |
| **2. Reconnect en passthrough** | Idem dans PuTTY | Couper alim, montrer bannières `*** link lost ***` puis `*** reconnected ***`, retaper, ça marche |
| **3. Latence par caractère** | Démontre la responsiveness | Tape une phrase dans PuTTY, montrer apparition côté PC distant en quasi-temps réel |
| **4. Burst 200 lignes** | Démontre le buffer + débit | `for /L %i in (1,1,200) do @echo line %i` → copier → coller → toutes les 200 lignes arrivent sans perte en ~14 s |
| **5. Portée** | Si possible en salle | Démarrer à 1 m (-76 dBm), s'éloigner, RSSI dégrade visiblement, session reste stable jusqu'à >10 m |
| **6. Métrique Retries** | Diagnostic en live | Pointer le panneau Connection : 0.0 retries à courte distance, augmente à longue → indicateur précoce |

### Points forts à mettre en avant

- **Liaison sub-GHz à 4.8 kbps avec portée intérieure ≥ 15 m** sur antenne simple, en environnement réel
- **Stack applicative complète** : ARQ stop-and-wait avec retries, dédup, keep-alive bidirectionnel (beacons + RTT pings), sticky reconnect automatique
- **Mode passthrough** transforme la liaison RF en câble série transparent — exploitable par n'importe quel outil qui parle un port COM
- **Robustesse** acquise par debug de plusieurs bugs profonds (CC1120 RBIAS grillé, INT0 edge bug, RFEND_CFG bit layout faux, partial-receive flush bug, etc.)
- **Itération sur le radio** : passage de 100 kHz → 25 kHz RX BW + activation IQIC = **+9-10 dB de sensibilité** → portée doublée
- **9 améliorations** en revue finale (P0 bugs, robustesse radio, observabilité, recovery)

### Points faibles à reconnaître honnêtement

- **Débit utile ~140 B/s** : utilisable pour shell interactif mais inutilisable pour `top` (refresh continu), `vi` sur gros fichier, ou transfert binaire
- **Latence ~75 ms par keystroke** : sensible mais acceptable
- **Buffer 2 KB suffit pour les pastes courants** ; au-delà il faudrait du flow control software (XON/XOFF, non implémenté)
- **Pas de chiffrement** : à mentionner si question (NET_ID = simple filtre, pas une sécurité)
- **Pas de multi-robots simultanés** : l'architecture le permet mais nécessiterait un layer de coordination en plus
- **ID robot codé en dur** (0x10) : provisioning à faire pour multi-robots

### Démarche scientifique à mettre en valeur

- **Test différentiel pin-par-pin** au multimètre qui a permis d'isoler le défaut hardware du CC1120 robot (RBIAS interne grillée)
- **Audit ligne par ligne du registre `cc1120_regs.h`** contre la datasheet officielle TI (correctifs d'adresses)
- **Vérification systématique des configurations CC1120** contre SmartRF Studio (notamment pour la BW 25 kHz)
- **Mesures de PER, retries et RTT** quantifiées pour piloter les choix d'architecture (ex: choix du `beacon-in-CONNECTED` à 1 Hz au lieu de 2 Hz pour réduire les collisions)
- **Documentation** : 3 rapports (RAPPORT_FINAL pour la synthèse, RAPPORT_DEBUG pour le journal chronologique, TESTS_PRESENTATION pour les mesures)

---

## Notes libres pendant les tests

> Espace pour noter des observations, anomalies, photos prises, etc.

```
(à remplir)
```

---

*Document à compléter au fil des tests. Le remplir directement remplace l'effort
de prise de notes parallèle.*
