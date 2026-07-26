/* credential_store.cc — libsecret + file fallback implementation */

#include "credential_store.h"

#include <libsecret/secret.h>
#include <glibmm/miscutils.h>

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace Mounter {

// ── Secret Service schema ───────────────────────────────────
// Schema name is duplicated here to avoid accessing the private static member
static constexpr const char* kSchemaName = "com.github.oddsoul.Mounter.Credentials";

static const SecretSchema kSchema{
  kSchemaName,
  SECRET_SCHEMA_NONE,
  {
    {"server",   SECRET_SCHEMA_ATTRIBUTE_STRING},
    {"share",    SECRET_SCHEMA_ATTRIBUTE_STRING},
    {"username", SECRET_SCHEMA_ATTRIBUTE_STRING},
    {"domain",   SECRET_SCHEMA_ATTRIBUTE_STRING},
    {nullptr,    SECRET_SCHEMA_ATTRIBUTE_STRING}  // sentinel
  },
  0,        // reserved
  nullptr,  // reserved1
  nullptr,  // reserved2
  nullptr,  // reserved3
  nullptr,  // reserved4
  nullptr,  // reserved5
  nullptr,  // reserved6
  nullptr,  // reserved7
};

CredentialStore::CredentialStore()
{
  auto dir = Glib::build_filename(
    Glib::get_user_config_dir(), "mounter", "credentials");
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
}

std::optional<CredentialEntry> CredentialStore::lookup(
    const std::string& server,
    const std::string& share)
{
  GError* error = nullptr;
  gchar* password = secret_password_lookup_sync(
    &kSchema,
    nullptr,
    &error,
    "server", server.c_str(),
    "share",  share.c_str(),
    nullptr);

  if (error) {
    g_warning("libsecret lookup failed: %s", error->message);
    g_error_free(error);
    error = nullptr;
  }

  if (password) {
    CredentialEntry entry;
    entry.server = server;
    entry.share  = share;

    // The password field stores a JSON blob with all fields
    std::string secret_str{password};
    secret_password_free(password);

    try {
      auto j = nlohmann::json::parse(secret_str);
      entry.username = j.value("username", "");
      entry.password = j.value("password", "");
      entry.domain   = j.value("domain", "");
      return entry;
    } catch (...) {
      // Legacy format: password was stored directly
      entry.password = secret_str;
      entry.username = "";
      entry.domain   = "";
      return entry;
    }
  }

  return lookup_file(server, share);
}

bool CredentialStore::store(const CredentialEntry& entry)
{
  nlohmann::json blob;
  blob["username"] = entry.username;
  blob["password"] = entry.password;
  blob["domain"]   = entry.domain;
  std::string secret_value = blob.dump();

  GError* error = nullptr;
  bool ok = secret_password_store_sync(
    &kSchema,
    SECRET_COLLECTION_DEFAULT,
    ("SMB: " + entry.server + "/" + entry.share).c_str(),
    secret_value.c_str(),
    nullptr,
    &error,
    "server",   entry.server.c_str(),
    "share",    entry.share.c_str(),
    "username", entry.username.c_str(),
    "domain",   entry.domain.c_str(),
    nullptr);

  if (error) {
    g_warning("libsecret store failed: %s (falling back to file)", error->message);
    g_error_free(error);
    return store_file(entry);
  }

  return ok;
}

bool CredentialStore::remove(const std::string& server,
                             const std::string& share)
{
  GError* error = nullptr;
  bool ok = secret_password_clear_sync(
    &kSchema,
    nullptr,
    &error,
    "server", server.c_str(),
    "share",  share.c_str(),
    nullptr);

  if (error) {
    g_warning("libsecret clear failed: %s", error->message);
    g_error_free(error);
  }

  remove_file(server, share);
  return ok;
}

std::vector<std::pair<std::string, std::string>> CredentialStore::list_keys()
{
  std::vector<std::pair<std::string, std::string>> keys;
  auto path = fallback_file_path();
  if (std::filesystem::exists(path)) {
    try {
      std::ifstream f(path);
      auto j = nlohmann::json::parse(f);
      for (const auto& item : j) {
        keys.emplace_back(
          item.value("server", ""),
          item.value("share", ""));
      }
    } catch (...) {}
  }
  return keys;
}

// ── File fallback ───────────────────────────────────────────

std::string CredentialStore::fallback_file_path()
{
  return Glib::build_filename(
    Glib::get_user_config_dir(), "mounter", "credentials", "store.json");
}

std::optional<CredentialEntry> CredentialStore::lookup_file(
    const std::string& server,
    const std::string& share)
{
  auto path = fallback_file_path();
  if (!std::filesystem::exists(path)) return std::nullopt;

  try {
    std::ifstream f(path);
    auto j = nlohmann::json::parse(f);
    for (const auto& item : j) {
      if (item.value("server", "") == server &&
          item.value("share", "") == share) {
        CredentialEntry entry;
        entry.server   = item.value("server", "");
        entry.share    = item.value("share", "");
        entry.username = item.value("username", "");
        entry.password = item.value("password", "");
        entry.domain   = item.value("domain", "");
        return entry;
      }
    }
  } catch (...) {}

  return std::nullopt;
}

bool CredentialStore::store_file(const CredentialEntry& entry)
{
  auto path = fallback_file_path();

  nlohmann::json entries = nlohmann::json::array();
  if (std::filesystem::exists(path)) {
    try {
      std::ifstream f(path);
      entries = nlohmann::json::parse(f);
    } catch (...) {
      entries = nlohmann::json::array();
    }
  }

  bool found = false;
  for (auto& item : entries) {
    if (item.value("server", "") == entry.server &&
        item.value("share", "") == entry.share) {
      item["username"] = entry.username;
      item["password"] = entry.password;
      item["domain"]   = entry.domain;
      found = true;
      break;
    }
  }

  if (!found) {
    nlohmann::json new_entry;
    new_entry["server"]   = entry.server;
    new_entry["share"]    = entry.share;
    new_entry["username"] = entry.username;
    new_entry["password"] = entry.password;
    new_entry["domain"]   = entry.domain;
    entries.push_back(new_entry);
  }

  std::ofstream f(path);
  if (!f.is_open()) return false;
  f << entries.dump(2);
  f.close();

  std::error_code ec;
  std::filesystem::permissions(path,
    std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);

  return true;
}

bool CredentialStore::remove_file(const std::string& server,
                                   const std::string& share)
{
  auto path = fallback_file_path();
  if (!std::filesystem::exists(path)) return true;

  try {
    std::ifstream f(path);
    auto entries = nlohmann::json::parse(f);

    nlohmann::json filtered = nlohmann::json::array();
    for (const auto& item : entries) {
      if (item.value("server", "") != server ||
          item.value("share", "") != share) {
        filtered.push_back(item);
      }
    }

    std::ofstream out(path);
    out << filtered.dump(2);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace Mounter
