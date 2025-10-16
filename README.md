# WebServSplit
Voici une **explication complète en français** du fonctionnement global du projet Webserv (Piggy version), organisée selon les **5 dossiers principaux**, avec pour chaque partie :

1️⃣ **Ce qu’elle fait actuellement (MVP)**
2️⃣ **Comment elle s’intègre dans le flux global**
3️⃣ **Ce qu’il reste à ajouter ou améliorer**

---

# 🧠 Vue d’ensemble du fonctionnement

Ton serveur **Webserv** est une application modulaire en C++98, structurée autour de **5 couches**.
Chaque couche correspond à un **dossier** dans `src/`, et représente un niveau d’abstraction précis :

```
[ app ]        → boucle principale, epoll, orchestration
[ net ]        → gestion des sockets et connexions
[ http ]       → logique protocolaire (HTTP)
[ config ]     → parsing et stockage de la configuration
[ core ]       → outils bas niveau (Fd, Epoll, Timer)
```

Le flux de données suit ce schéma :

```
Client → Listener (net) → Connection (net)
→ HttpRequest / Router / StaticHandler (http)
→ HttpResponse (http)
→ Connection::send() (net)
→ epoll (core) → HttpServer (app)
```

---

## 1️⃣ `src/app/` – **HttpServer : la boucle principale**

### 🔹 Rôle actuel

* Contient la classe `HttpServer`, qui orchestre l’ensemble du serveur.
* Crée la boucle principale basée sur **epoll**.
* Surveille les événements sur les sockets :

  * `EPOLLIN` → lecture (nouvelle requête ou donnée entrante)
  * `EPOLLOUT` → écriture (envoi d’une réponse)
* Gère le cycle de vie des connexions (`accept`, `read`, `write`, `close`).

### 🔹 Intégration dans le flux

* C’est le **chef d’orchestre** du projet.
* Il reçoit les événements d’Epoll (fournis par `core/EpollReactor`),
  et délègue le traitement réseau à `net/Connection`,
  puis le traitement HTTP à `http/` (Router, Handlers...).

### 🔹 À ajouter / améliorer

✅ Court terme :

* Gestion d’un **signal d’arrêt propre** (ex. `SIGINT` → stop server).
* Logs plus détaillés pour le débogage.

🚀 Moyen terme :

* Intégrer un **Timer** (depuis `core/TimerWheel`) pour gérer les **timeouts** :

  * fermeture automatique des connexions inactives.
* Gestion du **Keep-Alive** :

  * au lieu de fermer la connexion après chaque réponse,
    on la garde ouverte quelques secondes pour d’autres requêtes.

---

## 2️⃣ `src/net/` – **Réseau : Listener & Connection**

### 🔹 Rôle actuel

* `Listener` ouvre un socket non bloquant et écoute (`bind`, `listen`).
* `Connection` représente un client connecté :

  * Lit les données (`recv` non bloquant).
  * Construit un buffer d’entrée `inbuf`.
  * Quand une requête complète est reçue, passe la main à `http/`.
  * Prépare la réponse dans `outbuf` puis envoie avec `send`.

### 🔹 Intégration dans le flux

* `HttpServer` utilise `Listener` pour accepter les clients.
* `Connection` est l’interface entre les couches **réseau** et **HTTP**.
* Il s’occupe de gérer les états :

  * lecture → traitement → écriture → fermeture.

### 🔹 À ajouter / améliorer

✅ Court terme :

* Nettoyage automatique des connexions fermées.
* Meilleure gestion des erreurs `recv` / `send`.

🚀 Moyen terme :

* Support du **POST** :

  * gérer la lecture du corps de requête (`Content-Length`, `chunked`).
  * stocker le contenu dans un fichier temporaire avant transfert.
* Support du **DELETE** :

  * suppression de fichiers via `unlink()`.
* **Gestion de l’upload** via `upload_store` dans la config.

---

## 3️⃣ `src/http/` – **Logique HTTP : parser, router, handlers**

### 🔹 Rôle actuel

* `HttpRequest` : parse la requête (méthode, cible, version, headers).
* `Router` : sélectionne le bon `ServerConfig` et `LocationConfig` en fonction de :

  * l’adresse IP / port,
  * le `Host`,
  * le chemin (`location` avec correspondance par préfixe).
* `StaticHandler` : sert un fichier depuis `root` ou renvoie 404.
* `HttpResponse` : construit la réponse complète :

  ```
  HTTP/1.1 200 OK
  Content-Length: ...
  Content-Type: ...
  Connection: close
  ```

### 🔹 Intégration dans le flux

* C’est la **couche de logique métier HTTP**.
* `Connection` appelle le routeur dès que la requête est complète.
* `Router` choisit la bonne action :

  * GET → `StaticHandler`
  * (plus tard) POST → `CgiHandler` ou upload
  * return directive → redirection HTTP 3xx

### 🔹 À ajouter / améliorer

✅ Court terme :

* Générer les bons codes d’erreur HTTP (400, 405, 413…).
* Support de `return` pour redirection (déjà prévu).

🚀 Moyen terme :

* Activer **CGI** (`CgiHandler`) :

  * `fork()` + `execve()` + pipes.
  * Transmettre les variables d’environnement HTTP.
* Gestion du **POST multipart/form-data**.
* Ajout d’un **moteur de template minimal** (facultatif, pour debug).
* Détection du **Content-Type** par extension.

---

## 4️⃣ `src/config/` – **Parsing et stockage de la configuration**

### 🔹 Rôle actuel

* `Parser` lit un fichier Nginx-like et crée une structure `Config`.
* Supporte les directives :

  * `listen`
  * `server_name`
  * `root`, `index`
  * `error_page`
  * `allowed_methods`
  * `autoindex`
  * `return`
  * `cgi`
  * `upload_store`
* `Config` contient :

  * une liste de `ServerConfig`
  * chaque `ServerConfig` contient ses `LocationConfig`.

### 🔹 Intégration dans le flux

* Chargé au démarrage dans `main.cpp`.
* Fournit toutes les informations à `Router` et `StaticHandler`.
* La structure `Config` est **constante** à l’exécution (lecture seule).

### 🔹 À ajouter / améliorer

✅ Court terme :

* Validation stricte : ports valides, chemins existants.
* Support des variables ou des valeurs par défaut globales.

🚀 Moyen terme :

* Gérer des **inclusions** de fichiers (`include conf.d/*.conf`).
* Support de directives globales (client_max_body_size, timeout, etc.).
* Parser plus robuste (gestion des erreurs plus claire).

---

## 5️⃣ `src/core/` – **Infrastructure bas niveau : Fd, Epoll, Timer**

### 🔹 Rôle actuel

* `Fd` : encapsule un descripteur de fichier (RAII).
  → garantit la fermeture automatique (`close()` dans le destructeur).
* `EpollReactor` :

  * ouvre un epoll (`epoll_create1`),
  * ajoute/modifie/supprime des fd (`epoll_ctl`),
  * attend des événements (`epoll_wait`).
* `TimerWheel` : placeholder (non encore utilisé).

### 🔹 Intégration dans le flux

* Utilisé par `HttpServer` pour la boucle principale.
* Fournit une interface simple aux couches supérieures.
* C’est le **fondement système** du serveur.

### 🔹 À ajouter / améliorer

✅ Court terme :

* Ajouter un **log d’événements epoll** pour le débogage.

🚀 Moyen terme :

* Implémenter `TimerWheel` :

  * gérer les délais pour timeout, keep-alive, CGI, etc.
  * chaque connexion peut avoir un "expiry time".
* Support multiplateforme :

  * adapter le backend pour `kqueue` (BSD/macOS).

---

# 🔄 Résumé général

| Couche    | Rôle actuel              | Évolutions prévues                   |
| --------- | ------------------------ | ------------------------------------ |
| `app/`    | Boucle principale, epoll | Timeout + Keep-Alive + Stop signal   |
| `net/`    | Listener + Connection    | POST / DELETE / Upload               |
| `http/`   | Parsing et routage HTTP  | CGI + redirection + multipart        |
| `config/` | Parsing du fichier conf  | Validation + include + global params |
| `core/`   | Fd, epoll, timer infra   | Implémenter TimerWheel + logs        |

---

