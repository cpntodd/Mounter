#!/usr/bin/env bash
# Setup APT repository for Mounter
# Usage: ./setup-repo.sh /path/to/mounter_0.1.0-1_amd64.deb

set -euo pipefail

DEB_PATH="${1:-}"
REPO_DIR="repo"

if [[ -z "$DEB_PATH" || ! -f "$DEB_PATH" ]]; then
    echo "Usage: $0 <path/to/mounter_x.y.z_amd64.deb>"
    echo "Build the .deb first: cd Mounter && dpkg-buildpackage -us -uc -b"
    exit 1
fi

# ── Create repo structure ───────────────────────────────────
mkdir -p "$REPO_DIR"/{pool/main,conf,dists/stable/main/binary-amd64}

# ── Copy .deb to pool ──────────────────────────────────────
DEB_NAME=$(basename "$DEB_PATH")
cp "$DEB_PATH" "$REPO_DIR/pool/main/$DEB_NAME"

# ── Create conf/distributions ───────────────────────────────
cat > "$REPO_DIR/conf/distributions" << CONFEOF
Origin: Mounter Project
Label: Mounter
Suite: stable
Codename: stable
Architectures: amd64
Components: main
Description: APT repository for Mounter — SMB/CIFS mount GUI for Linux
SignWith: default
CONFEOF

# ── Generate Packages ───────────────────────────────────────
cd "$REPO_DIR"
dpkg-scanpackages --multiversion pool/main > dists/stable/main/binary-amd64/Packages
gzip -kf dists/stable/main/binary-amd64/Packages

# ── Generate Release ────────────────────────────────────────
cat > dists/stable/Release << RELEOF
Origin: Mounter Project
Label: Mounter
Suite: stable
Codename: stable
Architectures: amd64
Components: main
Description: APT repository for Mounter — SMB/CIFS mount GUI for Linux
Date: $(date -Ru)
RELEOF

# ── Append checksums ────────────────────────────────────────
{
    echo "MD5Sum:"
    find dists/stable -type f \( -name "Packages" -o -name "Packages.gz" \) -exec md5sum {} \; | while read hash path; do
        size=$(stat -c%s "$path")
        echo " $hash $size $(echo "$path" | sed 's|dists/stable/||')"
    done
    echo "SHA256:"
    find dists/stable -type f \( -name "Packages" -o -name "Packages.gz" \) -exec sha256sum {} \; | while read hash path; do
        size=$(stat -c%s "$path")
        echo " $hash $size $(echo "$path" | sed 's|dists/stable/||')"
    done
} >> dists/stable/Release

echo ""
echo "APT repo structure created in ./$REPO_DIR/"
echo ""
echo "Next steps:"
echo "  1. Sign the Release file:"
echo "     gpg --armor --detach-sign $REPO_DIR/dists/stable/Release"
echo "     mv $REPO_DIR/dists/stable/Release.asc $REPO_DIR/dists/stable/Release.gpg"
echo ""
echo "  2. Export public key for users:"
echo "     gpg --armor --export YOUR_KEY_ID > $REPO_DIR/mounter.asc"
echo ""
echo "  3. Push $REPO_DIR/ to GitHub Pages branch"
echo ""
