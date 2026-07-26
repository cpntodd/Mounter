#!/usr/bin/env bash
set -euo pipefail

# ── Colors ───────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
NC='\033[0m'; BOLD='\033[1m'

log()   { echo -e "${GREEN}[+]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
err()   { echo -e "${RED}[-]${NC} $*"; exit 1; }

REPO_URL="https://cpntodd.github.io/Mounter/repo"
KEYRING="/etc/apt/keyrings/mounter.asc"
SOURCE_LIST="/etc/apt/sources.list.d/mounter.list"

# ── Pre-flight checks ────────────────────────────────────────
[[ "$(id -u)" -ne 0 ]] && err "This script must be run as root (use sudo or pipe to sudo bash)"

# Check distro
if ! grep -qi 'debian' /etc/os-release 2>/dev/null; then
  warn "This script is designed for Debian. Other distros may not work."
fi

# Check architecture
ARCH=$(dpkg --print-architecture 2>/dev/null || echo "unknown")
if [[ "$ARCH" != "amd64" ]]; then
  err "Unsupported architecture: $ARCH. Mounter currently supports amd64 only."
fi

log "Mounter Installer — Debian 13 (trixie) amd64"

# ── Install dependencies ─────────────────────────────────────
log "Installing prerequisites..."
apt-get update -qq
apt-get install -y -qq curl gnupg 2>/dev/null

# ── Import GPG key ───────────────────────────────────────────
log "Importing repository signing key..."
mkdir -p /etc/apt/keyrings
curl -fsSL "$REPO_URL/mounter.asc" -o "$KEYRING"
chmod 644 "$KEYRING"

# ── Add apt source ───────────────────────────────────────────
log "Adding apt repository..."
echo "deb [signed-by=$KEYRING] $REPO_URL stable main" > "$SOURCE_LIST"

# ── Install Mounter ──────────────────────────────────────────
log "Installing Mounter..."
apt-get update -qq
apt-get install -y mounter

# ── Success ──────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}   Mounter installed successfully!${NC}"
echo ""
echo -e "  Launch:  ${BOLD}mounter${NC}"
echo -e "  Or find 'Mounter' in your application menu."
echo -e "  Tab shortcuts: Ctrl+1-6"
echo ""
echo -e "  ${YELLOW}Tip:${NC} Run the Diagnostics tab to verify all dependencies."
echo ""
