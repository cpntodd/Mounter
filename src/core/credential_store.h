/* credential_store.h — Secure credential storage
 *
 * Stores SMB credentials using libsecret (Secret Service / GNOME Keyring)
 * with a plaintext JSON file fallback.
 */

#pragma once

#include <string>
#include <optional>

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
  CredentialStore() = default;

  /// Look up stored credentials for a server/share pair.
  std::optional<CredentialEntry> lookup(const std::string& server,
                                        const std::string& share);

  /// Store credentials. Returns false if storage failed.
  bool store(const CredentialEntry& entry);

  /// Remove stored credentials.
  bool remove(const std::string& server, const std::string& share);
};

} // namespace Mounter
