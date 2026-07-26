# ── Source files for the main GUI binary ──────────────────────
mounter_sources = files(
  'main.cc',
  'application.cc',
  'window.cc',
  'pages/discovery_page.cc',
  'pages/mount_page.cc',
  'pages/mounted_page.cc',
  'pages/profiles_page.cc',
  'pages/diagnostics_page.cc',
  'core/mount_operation.cc',
  'core/credential_store.cc',
  'core/mount_monitor.cc',
  'core/discovery_engine.cc',
  'core/systemd_manager.cc',
  'ui/style_manager.cc',
)

mounter_deps = [
  gtkmm_dep,
  libsecret_dep,
  polkit_dep,
  thread_dep,
  json_dep,
]

if libadwaita_dep.found()
  mounter_deps += libadwaita_dep
endif

# Pass the helper install path at compile time
helper_path_def = '-DHELPER_PATH="' + helperdir + '/mounter-helper"'

executable('mounter',
  mounter_sources,
  dependencies: mounter_deps,
  cpp_args: [helper_path_def],
  install: true,
  install_dir: bindir,
)

# ── Helper binary (runs privileged operations via pkexec) ─────
helper_sources = files(
  'helpers/mounter-helper.cc',
)

executable('mounter-helper',
  helper_sources,
  dependencies: [json_dep],
  install: true,
  install_dir: helperdir,
)
