# Contributing to paint.software

## 🇬🇧 English

Thank you for helping improve **paint.software**! This is an open-source Paint.NET clone and
**everyone is welcome to contribute** — no write access needed.

### How it works
Because this is a public repository, the safe open-source model is already in place:

- **Anyone** can fork the project and open a pull request.
- Only the **maintainer** (repository owner) can merge into `main`.
- Every pull request therefore lands **only after the maintainer's final approval**.

### Steps
1. **Fork** this repository to your own GitHub account.
2. **Clone** your fork and create a branch:
   ```bash
   git clone git@github.com:<you>/paint.software.git
   cd paint.software
   git checkout -b my-change
   ```
3. **Build and test** before committing:
   ```bash
   cmake -B build && cmake --build build -j
   cmake --build build --target paintdotnet_tests
   QT_QPA_PLATFORM=offscreen ./build/paintdotnet_tests
   ```
4. **Commit** with a clear message and **push** to your fork.
5. Open a **Pull Request** against `Univers4craft/paint.software:main`.
6. GitHub Actions runs the build + tests automatically. Once green and reviewed,
   the maintainer merges it.

### Guidelines
- Keep changes focused; one topic per pull request.
- Match the surrounding code style (C++17, Qt6 idioms).
- Add or update a test in `tests/test_all.cpp` when you fix a bug or add behavior.
- Remember layer images are stored **premultiplied ARGB32** — un-premultiply before raw pixel math.
- Describe *what* and *why* in the PR description.

Found a bug or have an idea? Open an **Issue** first if you want to discuss before coding.

---

## 🇫🇷 Français

Merci d'aider à améliorer **paint.software** ! C'est un clone open source de Paint.NET et
**tout le monde peut contribuer** — aucun accès en écriture n'est requis.

### Comment ça marche
Comme le dépôt est public, le modèle open source sécurisé est déjà en place :

- **N'importe qui** peut forker le projet et ouvrir une pull request.
- Seul le **mainteneur** (propriétaire du dépôt) peut fusionner dans `main`.
- Chaque pull request n'est donc intégrée qu'**après l'approbation finale du mainteneur**.

### Étapes
1. **Forkez** ce dépôt sur votre propre compte GitHub.
2. **Clonez** votre fork et créez une branche :
   ```bash
   git clone git@github.com:<vous>/paint.software.git
   cd paint.software
   git checkout -b ma-modif
   ```
3. **Compilez et testez** avant de committer :
   ```bash
   cmake -B build && cmake --build build -j
   cmake --build build --target paintdotnet_tests
   QT_QPA_PLATFORM=offscreen ./build/paintdotnet_tests
   ```
4. **Committez** avec un message clair et **poussez** vers votre fork.
5. Ouvrez une **Pull Request** vers `Univers4craft/paint.software:main`.
6. GitHub Actions compile et teste automatiquement. Une fois au vert et relue,
   le mainteneur la fusionne.

### Règles
- Gardez des modifications ciblées ; un seul sujet par pull request.
- Respectez le style du code environnant (C++17, idiomes Qt6).
- Ajoutez ou mettez à jour un test dans `tests/test_all.cpp` pour tout correctif ou ajout.
- Les images de calque sont stockées en **ARGB32 prémultiplié** — dé-prémultipliez avant les
  calculs bruts sur les pixels.
- Expliquez *quoi* et *pourquoi* dans la description de la PR.

Un bug ou une idée ? Ouvrez d'abord une **Issue** si vous souhaitez en discuter avant de coder.
