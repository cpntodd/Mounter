/* discovery_engine.cc */

#include "discovery_engine.h"

namespace Mounter {

void DiscoveryEngine::scan(const std::string& /*subnet*/,
                           DiscoveryProgressCallback /*progress_callback*/)
{
  // Phase 3: run nmap and smbclient subprocesses
}

} // namespace Mounter
