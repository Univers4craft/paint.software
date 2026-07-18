# Getting paint.software into distribution software libraries

Releases already ship a `.deb` (+ signed APT repo), an **AppImage**, and a
**Flatpak bundle** — all built automatically on every push. This guide is about
the next step: listing the app in the graphical software stores so users find it
by searching, not by downloading a file.

Two realistic targets, from easiest to most impactful:

---

## 1. AUR (Arch Linux / Manjaro) — quickest

The [AUR](https://aur.archlinux.org/) lets anyone publish a package. Arch users
then install it with `yay -S paint.software-git` (or `paru`, or `makepkg`).

Everything is ready in [`packaging/aur/`](aur/): a `-git` `PKGBUILD` (it tracks
`main` automatically, matching our continuous releases) and its `.SRCINFO`.

**To publish (one-time, needs an AUR account):**

1. Create an account at https://aur.archlinux.org and add your SSH public key
   (Account → My Account → SSH Public Key).
2. Clone the (empty) AUR repo and drop the files in:
   ```bash
   git clone ssh://aur@aur.archlinux.org/paint.software-git.git
   cd paint.software-git
   cp /path/to/paint.software/packaging/aur/{PKGBUILD,.SRCINFO} .
   git add PKGBUILD .SRCINFO
   git commit -m "Initial import"
   git push
   ```
3. Done — it appears at `https://aur.archlinux.org/packages/paint.software-git`.

Because it's a `-git` package, users always get the latest `main`; you don't need
to touch it for every release.

---

## 2. Flathub — most impactful (one store, every distro)

[Flathub](https://flathub.org/) is *the* cross-distribution app store. Once the
app is on it, it shows up in **GNOME Software**, **KDE Discover** and
`flatpak install flathub io.github.univers4craft.PaintSoftware` on essentially
every Linux distribution — Fedora, Ubuntu, Mint, openSUSE, and the rest.

The Flathub-ready manifest is [`packaging/flathub/io.github.univers4craft.PaintSoftware.yml`](flathub/io.github.univers4craft.PaintSoftware.yml).
Unlike the bundle manifest, it pins a **tag + commit** (Flathub requires a fixed
source), and declares `x-checker-data` so Flathub's bot proposes an update PR
when a new `v*` tag lands.

**To submit (needs a GitHub account — the app-id `io.github.univers4craft.*`
already matches this GitHub account, which Flathub requires):**

1. Read the current [Flathub submission docs](https://docs.flathub.org/docs/for-app-authors/submission).
2. Fork https://github.com/flathub/flathub and create a branch named after the
   app-id: `io.github.univers4craft.PaintSoftware`.
3. Add the manifest (and, if Flathub asks for it inline, the metainfo) to that
   branch and open a PR against the `new-pr` branch.
4. Flathub's CI runs `flatpak-builder-lint`; fix anything it flags (the metainfo
   already passes `appstreamcli validate`). A reviewer then approves it.
5. After approval you get a dedicated `flathub/io.github.univers4craft.PaintSoftware`
   repo; pushes there publish updates. Bump the `tag` + `commit` per release (or
   let the checker bot open the PR for you).

Requirements the project already meets: MIT license, a validated AppStream
metainfo with a screenshot and release, a reverse-DNS app-id matching an account
you control, and a build with no network access beyond the pinned source.

---

## What I can't do for you

Publishing to AUR and Flathub needs **your** accounts (AUR SSH key, GitHub fork,
store reviews). The packaging is done and validated; the account-gated submission
steps above are yours to run. Ping me if the linter or a reviewer flags something
and I'll fix the manifest/metainfo.
