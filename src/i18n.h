/* i18n.h — Internationalization support via gettext
 *
 * Include this header in any file that contains user-visible strings.
 * Provides standard i18n macros:
 *   _(String)     — translate at runtime (dgettext)
 *   N_(String)    — mark for extraction only (no runtime lookup)
 *   C_(Ctx, Str)  — context-disambiguated translation (pgettext)
 */

#pragma once

#include <libintl.h>

#ifndef GETTEXT_PACKAGE
  #define GETTEXT_PACKAGE "mounter"
#endif

#define _(String)   dgettext(GETTEXT_PACKAGE, String)
#define N_(String)  (String)

// C_() provides context-disambiguated translations.
// Uses dgettext with a combined context+msgid so xgettext can extract both.
// The context is separated from msgid by \004 (ASCII EOT) per gettext convention.
#define C_(Context, String) \
  dgettext(GETTEXT_PACKAGE, Context "\004" String)
