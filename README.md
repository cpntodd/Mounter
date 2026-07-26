# Mounter

**A GUI tool for mounting SMB/CIFS network shares on Linux.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://github.com/cpntodd/Mounter)

Stop editing `/etc/fstab` by hand. Mounter gives you a clean graphical interface to discover, mount, and manage SMB/CIFS network shares — with proper kernel-level CIFS mounts, credential storage, and persistent automounts via systemd.

## Features

- **Network Discovery** — scan your local subnet for SMB hosts and browse available shares
- **Manual Mount** — enter server, share, and credentials directly
- **Active Mount Management** — view and unmount currently connected shares
- **Persistent Mounts** — generate systemd `.mount` + `.automount` units so shares survive reboots
- **Credential Management** — stored securely via libsecret (GNOME Keyring / Secret Service) with plaintext fallback
- **Profile Saving** — save connection details for one-click re-mounting
- **Diagnostics** — check for required system dependencies at a glance
- **DE-Agnostic** — works on GNOME, XFCE, MATE, Budgie, Sway, and more. Auto-detects your desktop for native styling.

## Screenshots

*(Coming soon)*

## Installation

### From .deb Package (Debian 13 Trixie)

```bash
# Install dependencies
sudo apt install cifs-utils pkexec polkitd smbclient nmap libsecret-tools

# Install the .deb
sudo dpkg -i mounter_0.1.0-1_amd64.deb
sudo apt --fix-broken install  # if needed
```

### From APT Repository (via GitHub Pages)

```bash
# Add the repository
echo "deb [signed-by=/etc/apt/keyrings/mounter.asc] https://cpntodd.github.io/Mounter trixie main" | \
  sudo tee /etc/apt/sources.list.d/mounter.list

# Import the GPG key
curl -fsSL https://cpntodd.github.io/Mounter/mounter.asc | \
  sudo tee /etc/apt/keyrings/mounter.asc

sudo apt update
sudo apt install mounter
```

### From Flatpak

```bash
flatpak install flathub com.github.oddsoul.Mounter
```

### From Source

```bash
# Install build dependencies
sudo apt install meson libgtkmm-4.0-dev libsecret-1-dev \
  libpolkit-gobject-1-dev nlohmann-json3-dev libadwaita-1-dev

# Build
meson setup build
cd build
meson compile

# Install
sudo meson install
```

## Usage

```bash
# Run with auto-detected styling
mounter

# Force GNOME/libadwaita styling
mounter --style=adwaita

# Force plain GTK4 styling (works on any DE)
mounter --style=plain
```

### Privilege Model

Mounter uses **polkit** for privilege escalation:

- Mount/umount operations require root (via `mount.cifs`)
- A helper binary (`mounter-helper`) performs privileged operations
- The GUI invokes it via `pkexec`, which shows a graphical password prompt
- Polkit policy allows `auth_self_keep` — you authenticate once per session
- To enable passwordless operation for `sudo` group members, add a polkit rule (see `data/com.github.oddsoul.Mounter.policy`)

## Architecture

```
mounter               (GUI, user process)
  ├── Discovery Engine    (nmap + smbclient)
  ├── Mount Operation     (talks to helper via pkexec)
  ├── Credential Store    (libsecret + file fallback)
  ├── Mount Monitor       (polls /proc/mounts)
  ├── Systemd Manager     (generates .mount units)
  └── Style Manager       (DE auto-detection)

mounter-helper        (privileged process, runs as root via pkexec)
  ├── mount              (mount -t cifs)
  ├── umount             (umount with lazy fallback)
  ├── write-cred         (secure credential file creation)
  └── write-unit         (systemd unit file creation)
```

## Tech Stack

- **Language:** C++17
- **GUI:** GTK4 via gtkmm-4.0
- **Build:** Meson
- **Credential Storage:** libsecret (Secret Service API)
- **Privilege Escalation:** polkit + pkexec
- **Packaging:** .deb (dh + meson), Flatpak, APT repo via GitHub Pages

## Project Structure

```
mounter/
├── src/
│   ├── main.cc                    # Entry point
│   ├── application.h/cc           # Gtk::Application subclass
│   ├── window.h/cc                # Main window with sidebar + stack
│   ├── pages/                     # 5 tab pages
│   │   ├── discovery_page.*       # Network scan & browse
│   │   ├── mount_page.*           # Manual mount form
│   │   ├── mounted_page.*         # Active mounts list
│   │   ├── profiles_page.*        # Saved profiles
│   │   └── diagnostics_page.*     # Dependency checks
│   ├── core/                      # Business logic
│   │   ├── mount_operation.*      # mount.cifs wrapper
│   │   ├── credential_store.*     # libsecret integration
│   │   ├── mount_monitor.*        # /proc/mounts polling
│   │   ├── discovery_engine.*     # nmap + smbclient
│   │   └── systemd_manager.*      # Unit generation
│   ├── helpers/
│   │   └── mounter-helper.cc      # Privileged operations binary
│   └── ui/
│       └── style_manager.*        # DE detection & theming
├── data/                          # Desktop entry, metainfo, icons, polkit policy
├── packaging/                     # Debian, Flatpak, APT repo configs
├── meson.build                    # Build system
└── LICENSE
```

## Development Status

**Phase 1 (current):** Skeleton — builds, installs, shows window with all 5 tabs.  
**Phase 2 (next):** Core mount/umount operations with helper binary.  
**Phase 3:** Full GUI implementation (discovery, profiles, etc.).  
**Phase 4:** Persistence via systemd units.  
**Phase 5:** Polish, error handling, packaging.

## License

GNU General Public License v3.0 or later. See [LICENSE](LICENSE).

---

**Why:** Existing solutions either drag in 50+ KDE dependencies (smb4k) or are Python-based with heavy runtimes. Mounter is a focused, native C++ GTK4 tool that does one thing well: mount SMB shares without touching the terminal.
