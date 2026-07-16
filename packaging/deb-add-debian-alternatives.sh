#!/usr/bin/env bash
# Make an Ubuntu-built .deb installable on Debian too.
#
# Ubuntu renamed every affected runtime library to *t64 during the 64-bit time_t
# transition, on every architecture. Debian renamed them only where time_t
# actually changed — the 32-bit ports — so on amd64 the Debian names stayed as
# they were. CPACK_DEBIAN_PACKAGE_SHLIBDEPS reads the names off the *build*
# machine, so a package built on ubuntu-latest asks for libqt6gui6t64, which
# simply does not exist on Debian 13 / LMDE 7:
#
#   paint.software : Depends: libqt6gui6t64 (>= 6.4.0) but it is not installable
#
# Rather than hard-code a dependency list (fragile, and it broke before), keep
# the auto-detected one and offer the Debian name as an alternative for each t64
# entry: "libqt6gui6t64 (>= 6.4.0) | libqt6gui6 (>= 6.4.0)". APT takes the first
# it can satisfy, so Ubuntu keeps the t64 package and Debian gets the plain one.
#
# Usage: deb-add-debian-alternatives.sh <package.deb>   (rewritten in place)
set -euo pipefail

DEB="${1:?usage: $0 <package.deb>}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

dpkg-deb -R "$DEB" "$WORK"

python3 - "$WORK/DEBIAN/control" <<'PY'
import re, sys

path = sys.argv[1]
with open(path) as fh:
    lines = fh.readlines()

out = []
for line in lines:
    if not line.startswith("Depends:"):
        out.append(line)
        continue
    deps = [d.strip() for d in line.split(":", 1)[1].split(",") if d.strip()]
    rewritten = []
    for dep in deps:
        # "libqt6gui6t64 (>= 6.4.0)" -> "libqt6gui6t64 (>= 6.4.0) | libqt6gui6 (>= 6.4.0)"
        # Leave alternatives and non-t64 names (libc6, libstdc++6…) alone.
        m = re.fullmatch(r"(\S+?)t64(\s*\([^)]*\))?", dep)
        if m and "|" not in dep:
            base, ver = m.group(1), m.group(2) or ""
            rewritten.append(f"{base}t64{ver} | {base}{ver}")
        else:
            rewritten.append(dep)
    out.append("Depends: " + ", ".join(rewritten) + "\n")

with open(path, "w") as fh:
    fh.writelines(out)
PY

dpkg-deb -b "$WORK" "$DEB" > /dev/null
echo "Debian alternatives added to $(basename "$DEB"):"
dpkg-deb -f "$DEB" Depends
