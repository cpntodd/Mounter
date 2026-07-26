# ── Desktop entry ──────────────────────────────────────────────
desktop_conf = configuration_data()
desktop_conf.set('bindir', bindir)
desktop_file = configure_file(
  input: 'com.github.oddsoul.Mounter.desktop.in',
  output: 'com.github.oddsoul.Mounter.desktop',
  configuration: desktop_conf,
  install: true,
  install_dir: desktopdir,
)

# ── Metainfo (AppStream) ──────────────────────────────────────
metainfo_conf = configuration_data()
metainfo_conf.set('version', meson.project_version())
metainfo_file = configure_file(
  input: 'com.github.oddsoul.Mounter.metainfo.xml.in',
  output: 'com.github.oddsoul.Mounter.metainfo.xml',
  configuration: metainfo_conf,
  install: true,
  install_dir: metainfodir,
)

# ── Polkit policy ─────────────────────────────────────────────
install_data(
  'com.github.oddsoul.Mounter.policy',
  install_dir: polkitdir,
)

# ── Application icon ──────────────────────────────────────────
install_data(
  'icons/com.github.oddsoul.Mounter.svg',
  install_dir: icondir,
)
