project('mounter', 'cpp',
  version: '0.1.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=false',
  ],
  meson_version: '>=1.0.0',
)

# ── Dependencies ──────────────────────────────────────────────
gtkmm_dep      = dependency('gtkmm-4.0', version: '>=4.14')
libsecret_dep  = dependency('libsecret-1')
polkit_dep     = dependency('polkit-gobject-1')
thread_dep     = dependency('threads')

# nlohmann_json is header-only; we use it as a fallback if not found as pkg-config
json_dep = dependency('nlohmann_json', required: false)
if not json_dep.found()
  # Try the single-header approach via the common cmake module path
  json_dep = dependency('nlohmann_json',
    method: 'cmake',
    modules: ['nlohmann_json::nlohmann_json'],
    required: false,
  )
endif
if not json_dep.found()
  warning('nlohmann_json not found via pkg-config or cmake — will try system include')
  json_dep = declare_dependency()
endif

# Optional: libadwaita for modern GNOME styling when StyleManager detects GNOME
libadwaita_dep = dependency('libadwaita-1', version: '>=1.5', required: false)
if libadwaita_dep.found()
  add_project_arguments('-DHAVE_LIBADWAITA=1', language: 'cpp')
endif

# ── Install paths ─────────────────────────────────────────────
prefix       = get_option('prefix')
bindir       = prefix / get_option('bindir')
libexecdir   = prefix / get_option('libexecdir')
datadir      = prefix / get_option('datadir')
sysconfdir   = prefix / get_option('sysconfdir')
icondir      = datadir / 'icons' / 'hicolor' / 'scalable' / 'apps'
metainfodir  = datadir / 'metainfo'
desktopdir   = datadir / 'applications'
polkitdir    = datadir / 'polkit-1' / 'actions'
polkitrulesdir = datadir / 'polkit-1' / 'rules.d'
helperdir    = libexecdir / 'mounter'

# ── i18n / gettext ────────────────────────────────────────────
i18n = import('i18n')
localedir = datadir / 'locale'
add_project_arguments('-DGETTEXT_PACKAGE="mounter"', language: 'cpp')
add_project_arguments('-DLOCALEDIR="' + prefix / localedir + '"', language: 'cpp')

# ── Subdirs ───────────────────────────────────────────────────
subdir('src')
subdir('data')
subdir('po')
