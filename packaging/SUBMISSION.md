# Distribution Guide

## APT Repository (Debian/Ubuntu)

### Setup (one-time)
```bash
# 1. Generate GPG key (if you don't have one)
gpg --full-generate-key
# Note the KEY_ID from output

# 2. Build the .deb
cd Mounter
ln -sf packaging/debian debian
mkdir -p debian/source && echo "3.0 (native)" > debian/source/format
dpkg-buildpackage -us -uc -b

# 3. Set up repo
./packaging/apt-repo/setup-repo.sh ../mounter_0.1.0-1_amd64.deb

# 4. Sign Release
cd repo
gpg --armor --detach-sign --default-key YOUR_KEY_ID dists/stable/Release
mv dists/stable/Release.asc dists/stable/Release.gpg

# 5. Export public key
gpg --armor --export YOUR_KEY_ID > mounter.asc

# 6. Push to GitHub Pages branch
git checkout -b apt-repo
git add repo/ && git commit -m "APT repository"
git push origin apt-repo
```

### Users install with:
```bash
curl -fsSL https://cpntodd.github.io/Mounter/install-mounter.sh | sudo bash
```

---

## Flatpak (Flathub)

### Submit to Flathub
1. Fork https://github.com/flathub/flathub
2. Create directory `com.github.oddsoul.Mounter/`
3. Copy these files into it:
   - `packaging/flatpak/com.github.oddsoul.Mounter.yml`
   - `packaging/flatpak/flathub.json`
4. Add a 128x128 PNG icon as `com.github.oddsoul.Mounter.png`
5. Create a PR to flathub/flathub

### Test locally
```bash
flatpak install org.gnome.Platform//47 org.gnome.Sdk//47
flatpak-builder build-dir packaging/flatpak/com.github.oddsoul.Mounter.yml --force-clean
flatpak-builder --run build-dir packaging/flatpak/com.github.oddsoul.Mounter.yml mounter
```

### Publish to Flathub Beta first
```bash
flatpak-builder --repo=repo build-dir packaging/flatpak/com.github.oddsoul.Mounter.yml --force-clean
flatpak build-bundle repo mounter.flatpak com.github.oddsoul.Mounter
```

---

## GitHub Releases

### Create a release .deb asset
```bash
# Build
dpkg-buildpackage -us -uc -b

# Upload to GitHub Releases
gh release create v0.1.0 ../mounter_0.1.0-1_amd64.deb \
  --title "Mounter v0.1.0" \
  --notes "Initial release. See README for features."
```
