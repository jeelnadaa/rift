#include "text.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace rift::core {

namespace {
std::string fmt1(double v, const char *unit) {
    char buf[48];
    if (v >= 100 || v == std::floor(v)) std::snprintf(buf, sizeof buf, "%.0f %s", v, unit);
    else if (v >= 10) std::snprintf(buf, sizeof buf, "%.1f %s", v, unit);
    else std::snprintf(buf, sizeof buf, "%.2f %s", v, unit);
    return buf;
}
} // namespace

std::string formatBytes(int64_t bytes, bool binary) {
    if (bytes < 0) return "—";
    const double k = binary ? 1024.0 : 1000.0;
    static const std::array<const char *, 6> dec{"B", "KB", "MB", "GB", "TB", "PB"};
    static const std::array<const char *, 6> bin{"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double v = static_cast<double>(bytes);
    size_t u = 0;
    while (v >= k && u + 1 < dec.size()) { v /= k; ++u; }
    if (u == 0) return std::to_string(bytes) + " B";
    return fmt1(v, binary ? bin[u] : dec[u]);
}

std::string formatSpeed(double bps, bool bits, bool binary) {
    if (!(bps > 0)) return bits ? "0 bit/s" : "0 B/s";
    if (bits) {
        double v = bps * 8;
        static const std::array<const char *, 5> u{"bit/s", "kbit/s", "Mbit/s", "Gbit/s", "Tbit/s"};
        size_t i = 0;
        while (v >= 1000 && i + 1 < u.size()) { v /= 1000; ++i; }
        return i == 0 ? std::to_string(static_cast<long long>(v)) + " bit/s" : fmt1(v, u[i]);
    }
    std::string s = formatBytes(static_cast<int64_t>(bps), binary);
    return s + "/s";
}

std::string formatEta(double s) {
    if (!(s >= 0) || std::isinf(s)) return "∞";
    char buf[32];
    const long long t = static_cast<long long>(std::llround(s));
    if (t < 60) { std::snprintf(buf, sizeof buf, "%llds", t); return buf; }
    const long long m = t / 60;
    if (m < 60) { std::snprintf(buf, sizeof buf, "%lldm %02llds", m, t % 60); return buf; }
    const long long h = m / 60;
    if (h < 48) { std::snprintf(buf, sizeof buf, "%lldh %02lldm", h, m % 60); return buf; }
    std::snprintf(buf, sizeof buf, "%lldd %lldh", h / 24, h % 24);
    return buf;
}

double eta(int64_t remaining, double speed) {
    if (remaining <= 0) return 0;
    if (!(speed > 0)) return -1;
    return static_cast<double>(remaining) / speed;
}

std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::string toLower(std::string s) {
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string sanitizeFileName(const std::string &in, size_t maxBytes) {
    std::string s;
    s.reserve(in.size());
    for (unsigned char c : in) {
        if (c < 32 || c == 127 || std::string_view("<>:\"/\\|?*").find(static_cast<char>(c)) != std::string_view::npos)
            s += '_';
        else
            s += static_cast<char>(c);
    }
    // No trailing dots/spaces (Windows strips them), no leading spaces.
    while (!s.empty() && (s.back() == '.' || s.back() == ' ')) s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    if (s.empty() || s == "." || s == "..") s = "download";
    // Windows reserved device names, with or without extension.
    {
        const std::string stem = toLower(s.substr(0, s.find('.')));
        static const std::array<const char *, 4> base{"con", "prn", "aux", "nul"};
        bool reserved = std::find(base.begin(), base.end(), stem) != base.end();
        if (!reserved && stem.size() == 4 && (stem.rfind("com", 0) == 0 || stem.rfind("lpt", 0) == 0) && stem[3] >= '0' &&
            stem[3] <= '9')
            reserved = true;
        if (reserved) s = "_" + s;
    }
    if (s.size() > maxBytes) {
        // Keep the extension; cut on a UTF-8 boundary.
        const size_t dot = s.rfind('.');
        std::string ext = (dot != std::string::npos && s.size() - dot <= 16) ? s.substr(dot) : std::string();
        size_t keep = maxBytes - ext.size();
        while (keep > 0 && (static_cast<unsigned char>(s[keep]) & 0xC0) == 0x80) --keep;
        s = s.substr(0, keep) + ext;
    }
    return s;
}

bool isSafeRelativePath(const std::string &p) {
    if (p.empty()) return false;
    if (p[0] == '/' || p[0] == '\\') return false;
    if (p.size() >= 2 && p[1] == ':') return false;
    size_t i = 0;
    while (i <= p.size()) {
        size_t j = p.find_first_of("/\\", i);
        if (j == std::string::npos) j = p.size();
        const std::string part = p.substr(i, j - i);
        if (part == ".." || part == ".") return false;
        i = j + 1;
    }
    return true;
}

static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string percentDecode(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            const int a = hexv(s[i + 1]), b = hexv(s[i + 2]);
            if (a >= 0 && b >= 0) { out += static_cast<char>(a * 16 + b); i += 2; continue; }
        }
        out += s[i];
    }
    return out;
}

std::string fileNameFromContentDisposition(const std::string &h) {
    const std::string low = toLower(h);
    // filename*=UTF-8''name%20here
    size_t p = low.find("filename*=");
    if (p != std::string::npos) {
        std::string v = h.substr(p + 10);
        const size_t semi = v.find(';');
        if (semi != std::string::npos) v = v.substr(0, semi);
        v = trim(v);
        if (!v.empty() && v.front() == '"' && v.back() == '"' && v.size() >= 2) v = v.substr(1, v.size() - 2);
        const size_t q = v.find("''");
        if (q != std::string::npos) v = v.substr(q + 2);
        v = percentDecode(v);
        if (!v.empty()) return v;
    }
    p = low.find("filename=");
    while (p != std::string::npos && p > 0 && low[p - 1] == '*') p = low.find("filename=", p + 1);
    if (p == std::string::npos) return {};
    std::string v = h.substr(p + 9);
    v = trim(v);
    if (!v.empty() && v.front() == '"') {
        std::string out;
        for (size_t i = 1; i < v.size(); ++i) {
            if (v[i] == '\\' && i + 1 < v.size()) { out += v[++i]; continue; }
            if (v[i] == '"') break;
            out += v[i];
        }
        return out;
    }
    const size_t semi = v.find(';');
    return trim(semi == std::string::npos ? v : v.substr(0, semi));
}

std::string fileNameFromUrl(const std::string &url) {
    std::string u = url;
    const size_t cut = u.find_first_of("?#");
    if (cut != std::string::npos) u = u.substr(0, cut);
    const size_t scheme = u.find("://");
    if (scheme != std::string::npos) {
        const size_t path = u.find('/', scheme + 3);
        if (path == std::string::npos) return {};
        u = u.substr(path);
    }
    while (!u.empty() && u.back() == '/') u.pop_back();
    const size_t slash = u.rfind('/');
    std::string last = slash == std::string::npos ? u : u.substr(slash + 1);
    return percentDecode(last);
}

std::string hostFromUrl(const std::string &url) {
    const size_t scheme = url.find("://");
    if (scheme == std::string::npos) return {};
    size_t a = scheme + 3;
    size_t b = url.find_first_of("/?#", a);
    std::string auth = url.substr(a, b == std::string::npos ? std::string::npos : b - a);
    const size_t at = auth.rfind('@');
    if (at != std::string::npos) auth = auth.substr(at + 1);
    if (!auth.empty() && auth[0] == '[') {
        const size_t close = auth.find(']');
        return toLower(auth.substr(0, close == std::string::npos ? auth.size() : close + 1));
    }
    const size_t colon = auth.find(':');
    if (colon != std::string::npos) auth = auth.substr(0, colon);
    return toLower(auth);
}

std::optional<int64_t> totalFromContentRange(const std::string &h) {
    const size_t slash = h.rfind('/');
    if (slash == std::string::npos) return std::nullopt;
    const std::string t = trim(h.substr(slash + 1));
    if (t.empty() || t == "*") return std::nullopt;
    for (char c : t)
        if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
    try { return static_cast<int64_t>(std::stoll(t)); } catch (...) { return std::nullopt; }
}

std::vector<std::string> expandBatchPattern(const std::string &text, size_t limit) {
    const size_t open = text.find('[');
    const size_t close = open == std::string::npos ? std::string::npos : text.find(']', open);
    if (open == std::string::npos || close == std::string::npos) return {text};
    const std::string inner = text.substr(open + 1, close - open - 1);
    const size_t dash = inner.find('-');
    if (dash == std::string::npos || dash == 0 || dash + 1 >= inner.size()) return {text};
    const std::string a = inner.substr(0, dash), b = inner.substr(dash + 1);
    for (char c : a + b)
        if (!std::isdigit(static_cast<unsigned char>(c))) return {text};
    long long from = 0, to = 0;
    try { from = std::stoll(a); to = std::stoll(b); } catch (...) { return {text}; }
    const bool pad = a.size() > 1 && a[0] == '0';
    const size_t width = pad ? a.size() : 0;
    std::vector<std::string> out;
    const long long step = from <= to ? 1 : -1;
    for (long long i = from;; i += step) {
        std::string n = std::to_string(i);
        if (n.size() < width) n.insert(0, width - n.size(), '0');
        out.push_back(text.substr(0, open) + n + text.substr(close + 1));
        if (i == to || out.size() >= limit) break;
    }
    return out;
}

std::string categoryForFileName(const std::string &name) {
    const size_t dot = name.rfind('.');
    if (dot == std::string::npos) return "Other";
    const std::string e = toLower(name.substr(dot + 1));
    auto in = [&e](std::initializer_list<const char *> l) {
        for (const char *x : l)
            if (e == x) return true;
        return false;
    };
    if (in({"mp4", "mkv", "avi", "mov", "webm", "m4v", "wmv", "flv", "ts", "mpg", "mpeg"})) return "Video";
    if (in({"mp3", "flac", "wav", "aac", "ogg", "opus", "m4a", "wma", "alac"})) return "Music";
    if (in({"jpg", "jpeg", "png", "gif", "webp", "bmp", "tif", "tiff", "heic", "svg", "raw"})) return "Images";
    if (in({"zip", "rar", "7z", "tar", "gz", "tgz", "bz2", "xz", "zst", "lz", "cab"})) return "Archives";
    if (in({"iso", "img", "dmg", "vhd", "vhdx", "qcow2"})) return "Disc images";
    if (in({"exe", "msi", "msix", "appx", "deb", "rpm", "appimage", "pkg", "apk", "flatpak", "snap"})) return "Programs";
    if (in({"pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "odt", "ods", "txt", "md", "epub", "csv", "rtf"}))
        return "Documents";
    return "Other";
}

bool isMagnet(const std::string &s) { return toLower(trim(s)).rfind("magnet:?", 0) == 0; }

bool isHttpUrl(const std::string &s) {
    const std::string l = toLower(trim(s));
    return l.rfind("http://", 0) == 0 || l.rfind("https://", 0) == 0 || l.rfind("ftp://", 0) == 0;
}

static std::string magnetParam(const std::string &m, const std::string &key) {
    const size_t q = m.find('?');
    if (q == std::string::npos) return {};
    size_t i = q + 1;
    while (i < m.size()) {
        size_t amp = m.find('&', i);
        if (amp == std::string::npos) amp = m.size();
        const std::string kv = m.substr(i, amp - i);
        const size_t eq = kv.find('=');
        if (eq != std::string::npos && toLower(kv.substr(0, eq)) == key) return kv.substr(eq + 1);
        i = amp + 1;
    }
    return {};
}

std::string magnetInfoHash(const std::string &m) {
    // Several xt= may exist; scan for btih.
    const std::string low = toLower(m);
    const size_t p = low.find("urn:btih:");
    if (p == std::string::npos) return {};
    size_t e = low.find_first_of("&", p);
    std::string h = low.substr(p + 9, e == std::string::npos ? std::string::npos : e - p - 9);
    return h;
}

std::string magnetDisplayName(const std::string &m) {
    std::string v = magnetParam(m, "dn");
    for (char &c : v)
        if (c == '+') c = ' ';
    return percentDecode(v);
}

} // namespace rift::core
