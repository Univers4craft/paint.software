# Signing the APT repository (GPG)

The APT repository at <https://univers4craft.github.io/paint.software> works **unsigned**
today (users add it with `[trusted=yes]`). To sign it — so users no longer need
`[trusted=yes]` and apt verifies authenticity — do the **one-time** setup below.

The signing **private key must never leave your control**: you generate it, and you paste it
into a GitHub *secret*. The CI workflow then signs each publish automatically. (Claude set up
the workflow but deliberately does **not** generate or hold your key.)

## 1. Generate a signing key (on your machine)

No-passphrase key (simplest for CI):

```bash
cat > paintsw-key.conf <<'EOF'
%no-protection
Key-Type: RSA
Key-Length: 4096
Name-Real: paint.software
Name-Email: Univers4craft@users.noreply.github.com
Expire-Date: 0
%commit
EOF
gpg --batch --generate-key paintsw-key.conf
rm paintsw-key.conf
```

Find the key's fingerprint:

```bash
gpg --list-secret-keys --keyid-format=long
```

## 2. Export the private key

```bash
gpg --armor --export-secret-keys Univers4craft@users.noreply.github.com > private.asc
```

## 3. Add it as GitHub secrets

Repo → **Settings → Secrets and variables → Actions → New repository secret**:

- **`GPG_PRIVATE_KEY`** → paste the entire contents of `private.asc`
- *(only if you set a passphrase)* **`GPG_PASSPHRASE`** → the passphrase

Then delete the local copy — it's now safely stored:

```bash
shred -u private.asc 2>/dev/null || rm -f private.asc
```

## 4. Re-run the workflow

**Actions → “APT repository (GitHub Pages)” → Run workflow.**

Once it succeeds, the repository is signed:
- the **public key** is published at
  `https://univers4craft.github.io/paint.software/paint.software.gpg`
- `dists/stable/InRelease` and `Release.gpg` are signed
- the landing page automatically switches to the **signed** install instructions.

## 5. Signed install (what users then run)

```bash
curl -fsSL https://univers4craft.github.io/paint.software/paint.software.gpg \
  | sudo tee /usr/share/keyrings/paint.software.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/paint.software.gpg] https://univers4craft.github.io/paint.software stable main" \
  | sudo tee /etc/apt/sources.list.d/paint.software.list
sudo apt update
sudo apt install paint.software
```

That's it — no more `[trusted=yes]`, and apt verifies every update.
