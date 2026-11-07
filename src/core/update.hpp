// Self-update helpers with no Qt: release version comparison, which release asset this build installs
// from, and the expected SHA-256 for it from the release's SHA256SUMS. Asset names are the contract in
// AGENTS.md rule 10 (and rift-static/download.html).
#pragma once
#include <array>
#include <optional>
#include <string>

namespace rift::core {

// "1.2.3" or "v1.2.3" → {1,2,3}. Anything else (dev builds "0.0.0-dev", "1.2", "") → nullopt.
std::optional<std::array<int, 3>> parseVersion(const std::string &v);

// True when `candidate` is a release strictly newer than `current`. False if either doesn't parse, and
// always false for current "0.0.0" (a local/dev build must never replace itself with a release).
bool isNewerVersion(const std::string &candidate, const std::string &current);

// The release asset a running build updates from, or "" when it can't update itself in place.
//   target: "windows-setup" | "macos-arm64" | "macos-x64" | "linux-appimage"
std::string updateAssetName(const std::string &version, const std::string &target);

// The hex SHA-256 (lowercase) listed for `fileName` in a sha256sum-style file ("<hex>  <name>" or
// "<hex> *<name>"), or "" if missing/malformed.
std::string sha256ForFile(const std::string &sums, const std::string &fileName);

} // namespace rift::core
