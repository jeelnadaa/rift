// Rift core: formatting and name helpers. Pure C++20.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rift::core {

// "1.2 GB", "640 KB", "12 B". Decimal (1000) by default, binary (1024, "GiB") on request.
std::string formatBytes(int64_t bytes, bool binary = false);
// "4.8 MB/s"; bits=true → "38.4 Mbit/s". 0 → "0 B/s".
std::string formatSpeed(double bytesPerSec, bool bits = false, bool binary = false);
// "45s", "3m 05s", "2h 07m", "3d 4h"; <0 or infinite → "∞".
std::string formatEta(double seconds);
// ETA = remaining / speed, or -1 when unknown.
double eta(int64_t remaining, double speed);

// Makes a single path component safe on every OS: no separators, reserved characters,
// control characters, Windows reserved names, or trailing dots/spaces. Never empty.
std::string sanitizeFileName(const std::string &name, size_t maxBytes = 200);
// True if a relative path (from a torrent) is safe: no "..", no absolute, no drive letters.
bool isSafeRelativePath(const std::string &path);

std::string percentDecode(const std::string &s);
// From a Content-Disposition header; prefers filename* (RFC 5987). Empty when absent.
std::string fileNameFromContentDisposition(const std::string &header);
// Last path segment of a URL, percent-decoded, without query/fragment. Empty when none.
std::string fileNameFromUrl(const std::string &url);
// Host part of a URL ("download.blender.org"), lowercase, without port/userinfo.
std::string hostFromUrl(const std::string &url);
// Total size from "bytes 0-99/1234" → 1234; nullopt when unknown ("*") or malformed.
std::optional<int64_t> totalFromContentRange(const std::string &header);

// Expands one numeric pattern "file[001-120].jpg" into 120 URLs (zero padding kept).
// Text without a pattern returns itself. Caps at `limit` results.
std::vector<std::string> expandBatchPattern(const std::string &text, size_t limit = 10000);

// "Video", "Music", "Images", "Archives", "Documents", "Programs", "Disc images" or "Other".
std::string categoryForFileName(const std::string &name);
bool isMagnet(const std::string &s);
bool isHttpUrl(const std::string &s);
// Hex info-hash from "magnet:?xt=urn:btih:..." (lowercase), empty if none.
std::string magnetInfoHash(const std::string &magnet);
// Display name ("dn=") from a magnet link, percent-decoded.
std::string magnetDisplayName(const std::string &magnet);

std::string trim(const std::string &s);
std::string toLower(std::string s);

} // namespace rift::core
