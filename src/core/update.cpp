#include "update.hpp"

#include <cctype>
#include <sstream>

namespace rift::core {

std::optional<std::array<int, 3>> parseVersion(const std::string &raw) {
    std::string v = raw;
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    std::array<int, 3> out{};
    size_t pos = 0;
    for (int part = 0; part < 3; ++part) {
        if (pos >= v.size() || !std::isdigit(static_cast<unsigned char>(v[pos]))) return std::nullopt;
        long n = 0;
        size_t start = pos;
        while (pos < v.size() && std::isdigit(static_cast<unsigned char>(v[pos]))) {
            n = n * 10 + (v[pos] - '0');
            if (n > 1'000'000 || pos - start > 7) return std::nullopt;
            ++pos;
        }
        out[static_cast<size_t>(part)] = static_cast<int>(n);
        if (part < 2) {
            if (pos >= v.size() || v[pos] != '.') return std::nullopt;
            ++pos;
        }
    }
    if (pos != v.size()) return std::nullopt; // "1.2.3-dev", "1.2.3.4"
    return out;
}

bool isNewerVersion(const std::string &candidate, const std::string &current) {
    const auto c = parseVersion(candidate), cur = parseVersion(current);
    if (!c || !cur) return false;
    if (*cur == std::array<int, 3>{0, 0, 0}) return false;
    return *c > *cur; // std::array compares lexicographically: major, then minor, then patch
}

std::string updateAssetName(const std::string &version, const std::string &target) {
    if (!parseVersion(version)) return {};
    std::string v = version;
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    if (target == "windows-setup") return "Rift-" + v + "-windows-x64-setup.exe";
    if (target == "macos-arm64") return "Rift-" + v + "-macos-arm64.zip";
    if (target == "macos-x64") return "Rift-" + v + "-macos-x64.zip";
    if (target == "linux-appimage") return "Rift-" + v + "-linux-x86_64.AppImage";
    return {};
}

std::string sha256ForFile(const std::string &sums, const std::string &fileName) {
    std::istringstream in(sums);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t sp = line.find(' ');
        if (sp != 64) continue;
        std::string name = line.substr(sp);
        size_t i = 0;
        while (i < name.size() && name[i] == ' ') ++i;
        if (i < name.size() && name[i] == '*') ++i; // binary-mode marker
        name.erase(0, i);
        if (name != fileName) continue;
        std::string hex = line.substr(0, 64);
        for (char &ch : hex) {
            if (!std::isxdigit(static_cast<unsigned char>(ch))) return {};
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return hex;
    }
    return {};
}

} // namespace rift::core
