/* credential_store.h — Secure credential storage
 *
 * Primary: libsecret (Secret Service / GNOME Keyring)
 * Fallback: encrypted JSON file in ~/.config/mounter/
 */

#pragma once

#include <string>
#include <optional>
#include <vector>

namespace Mounter {

struct CredentialEntry {
  std::string server;
  std::string share;
  std::string username;
  std::string password;
  std::string domain;
};

class CredentialStore
{
public:
  CredentialStore();
  ~CredentialStore() = default;

  /// Look up stored credentials for a server/share pair.
  /// Returns std::nullopt if not found.
  std::optional<CredentialEntry> lookup(const std::string& server,
                                        const std::string& share);

  /// Store credentials. Returns true on success.
  bool store(const CredentialEntry& entry);

  /// Remove stored credentials. Returns true on success.
  bool remove(const std::string& server, const std::string& share);

  /// List all stored credential keys (server/share pairs).
  std::vector<std::pair<std::string, std::string>> list_keys();

private:
  // Secret Service schema for our credential entries
  static constexpr const char* kSchemaName = "com.github.oddsoul.Mounter.Credentials";

  // File-based fallback path
  static std::string fallback_file_path();
  std::optional<CredentialEntry> lookup_file(const std::string& server,
                                             const std::string& share);
  bool store_file(const CredentialEntry& entry);
  bool remove_file(const std::string& server, const std::string& share);
};

} // namespace Mounter
