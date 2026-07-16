# Security Policy · Politique de sécurité

---

## 🇬🇧 English

### Supported versions

paint.software is released continuously: every commit on `main` produces a build, published as its
own release and served by the APT repository.

| Version | Supported |
| --- | --- |
| Latest published build (`v1.1.x`, highest number) | ✅ Yes |
| Any earlier build | ❌ No — please upgrade first |

There is no long-term support branch. If you hit a problem, run `sudo apt update && sudo apt upgrade`
and check whether it still happens on the latest build before reporting.

### Reporting a vulnerability

**Please do not open a public issue for a security problem.** A public issue tells everyone how to
attack users who have not upgraded yet.

Report it privately instead, through GitHub:

1. Go to the [Security tab](https://github.com/Univers4craft/paint.software/security).
2. Click **Report a vulnerability**.
3. Describe what you found, how to reproduce it, and what an attacker could obtain.

Only you and the maintainer can see the report. Once a fix is published, the report can be turned into
a public advisory so users know why they should upgrade.

What to expect:

- An acknowledgement within **7 days**.
- An assessment (accepted / not a vulnerability / out of scope) within **30 days**.
- This is a spare-time project, not a company: these are honest targets, not contractual deadlines.

### Scope

**In scope** — the parts this project is responsible for:

- Reading a file crashing the app in an exploitable way, or doing anything beyond opening it
  (including our own `.psw` format).
- The plugin loader boundary: a plugin being loaded from somewhere it should not be, or the ABI
  mishandling data in a way that corrupts memory.
- The APT repository and its GPG signing: anything letting a third party serve a package that `apt`
  would accept as ours.
- The build and release workflows: anything letting a third party inject code into a published build.

**Out of scope** — real, but not something a fix here can address:

- **Third-party plugins run native code, by design.** A plugin is a `.so` loaded into the process; it
  has every permission you do. Installing one is trusting its author, exactly like installing any
  program. "A malicious plugin can do harm" is not a vulnerability in paint.software — it is what a
  plugin *is*. Only install plugins you trust.
- Vulnerabilities in Qt, in the system image codecs, or in any other dependency. Report those upstream
  — we can only pick up the fixed version.
- Anything requiring an attacker to already have local access to your account.
- Bugs with no security impact: those are welcome as ordinary
  [issues](https://github.com/Univers4craft/paint.software/issues).

### What we do

- Every published version is built from `main` by GitHub Actions — no manual uploads.
- The APT repository is GPG-signed; `apt` refuses anything not signed with our key.
- Contributions are reviewed and merged with the maintainer's final approval.

---

## 🇫🇷 Français

### Versions prises en charge

paint.software est publié en continu : chaque commit sur `main` produit un build, publié comme une
release à part entière et distribué par le dépôt APT.

| Version | Prise en charge |
| --- | --- |
| Dernier build publié (`v1.1.x`, numéro le plus élevé) | ✅ Oui |
| Tout build antérieur | ❌ Non — merci de mettre à jour d'abord |

Il n'y a pas de branche de support à long terme. En cas de problème, faites
`sudo apt update && sudo apt upgrade` et vérifiez qu'il persiste sur le dernier build avant de le
signaler.

### Signaler une faille

**N'ouvrez pas d'issue publique pour un problème de sécurité.** Une issue publique explique à tout le
monde comment attaquer les utilisateurs qui n'ont pas encore mis à jour.

Signalez-la en privé, via GitHub :

1. Allez dans l'[onglet Security](https://github.com/Univers4craft/paint.software/security).
2. Cliquez sur **Report a vulnerability**.
3. Décrivez ce que vous avez trouvé, comment le reproduire, et ce qu'un attaquant pourrait obtenir.

Seuls vous et le mainteneur voyez le signalement. Une fois le correctif publié, il peut être transformé
en avis public pour que les utilisateurs sachent pourquoi mettre à jour.

Ce à quoi vous attendre :

- Un accusé de réception sous **7 jours**.
- Une évaluation (acceptée / pas une faille / hors périmètre) sous **30 jours**.
- C'est un projet de temps libre, pas une entreprise : ce sont des objectifs sincères, pas des délais
  contractuels.

### Périmètre

**Dans le périmètre** — ce dont ce projet est responsable :

- L'ouverture d'un fichier qui fait planter l'application de façon exploitable, ou qui fait autre chose
  que l'ouvrir (y compris notre propre format `.psw`).
- La frontière du chargeur de plugins : un plugin chargé depuis un endroit qu'il ne devrait pas, ou
  l'ABI qui manipule mal les données au point de corrompre la mémoire.
- Le dépôt APT et sa signature GPG : tout ce qui permettrait à un tiers de servir un paquet qu'`apt`
  accepterait comme étant le nôtre.
- Les workflows de build et de publication : tout ce qui permettrait à un tiers d'injecter du code dans
  un build publié.

**Hors périmètre** — réel, mais qu'un correctif ici ne peut pas traiter :

- **Les plugins tiers exécutent du code natif, par conception.** Un plugin est un `.so` chargé dans le
  processus ; il a exactement vos droits. En installer un, c'est faire confiance à son auteur, comme
  pour n'importe quel programme. « Un plugin malveillant peut nuire » n'est pas une faille de
  paint.software — c'est ce qu'*est* un plugin. N'installez que des plugins de confiance.
- Les failles de Qt, des codecs d'image du système ou de toute autre dépendance. Signalez-les en amont
  — nous ne pouvons que récupérer la version corrigée.
- Tout ce qui suppose que l'attaquant a déjà un accès local à votre session.
- Les bugs sans impact de sécurité : ils sont les bienvenus en
  [issue](https://github.com/Univers4craft/paint.software/issues) ordinaire.

### Ce que nous faisons

- Chaque version publiée est construite depuis `main` par GitHub Actions — aucun envoi manuel.
- Le dépôt APT est signé avec GPG ; `apt` refuse tout ce qui n'est pas signé avec notre clé.
- Les contributions sont relues et fusionnées avec l'approbation finale du mainteneur.
