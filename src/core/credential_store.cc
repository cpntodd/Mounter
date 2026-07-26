/* credential_store.cc */

#include "credential_store.h"

namespace Mounter {

std::optional<CredentialEntry> CredentialStore::lookup(
    const std::string& /*server*/,
    const std::string& /*share*/)
{
  // Phase 3: implement libsecret lookup with file fallback
  return std::nullopt;
}

bool CredentialStore::store(const CredentialEntry& /*entry*/)
{
  // Phase 3: implement
  return false;
}

bool CredentialStore::remove(const std::string& /*server*/,
                             const std::string& /*share*/)
{
  // Phase 3: implement
  return false;
}

} // namespace Mounter
