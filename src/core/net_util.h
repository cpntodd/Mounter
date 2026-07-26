/* net_util.h — Network address utilities
 *
 * Helpers for detecting and formatting IPv4/IPv6 addresses for
 * use in SMB URLs (//server/share format).
 */

#pragma once

#include <string>
#include <string_view>
#include <algorithm>

namespace Mounter::NetUtil {

/// Returns true if the address looks like an IPv6 address
/// (contains colons and may be wrapped in brackets).
inline bool is_ipv6(std::string_view addr)
{
  // Strip brackets if present
  if (!addr.empty() && addr.front() == '[' && addr.back() == ']')
    addr = addr.substr(1, addr.size() - 2);

  // IPv6 addresses contain colons
  return addr.find(':') != std::string_view::npos;
}

/// Ensure an IP address is properly formatted for SMB URLs.
/// IPv6 addresses are wrapped in brackets.
/// IPv4 addresses are returned unchanged.
inline std::string bracket_if_ipv6(std::string_view addr)
{
  std::string a{addr};

  // Already bracketed
  if (!a.empty() && a.front() == '[' && a.back() == ']')
    return a;

  if (is_ipv6(a)) {
    // Strip scope ID (%eth0) if present — SMB doesn't use it
    auto pct = a.find('%');
    if (pct != std::string::npos)
      a = a.substr(0, pct);

    return "[" + a + "]";
  }

  return a;
}

/// Strip brackets from an IPv6 address if present.
/// Returns the bare address without brackets.
inline std::string unbracket(std::string_view addr)
{
  if (!addr.empty() && addr.front() == '[' && addr.back() == ']')
    return std::string{addr.substr(1, addr.size() - 2)};
  return std::string{addr};
}

} // namespace Mounter::NetUtil
