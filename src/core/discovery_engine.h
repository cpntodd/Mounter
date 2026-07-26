/* discovery_engine.h — Network SMB discovery
 *
 * Uses nmap for port scanning and smbclient for share enumeration.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace Mounter {

struct DiscoveredHost {
  std::string ip_address;
  std::string hostname;      // resolved via reverse DNS or NetBIOS
  bool        reachable = false;
};

struct DiscoveredShare {
  std::string name;
  std::string comment;
  bool        guest_ok = false;
};

using DiscoveryProgressCallback = std::function<void(const std::string& status)>;

class DiscoveryEngine
{
public:
  DiscoveryEngine() = default;

  /// Scan a subnet for SMB hosts and enumerate their shares.
  /// Calls the progress callback with status updates.
  void scan(const std::string& subnet,
            DiscoveryProgressCallback progress_callback);
};

} // namespace Mounter
