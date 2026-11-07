// rift-get: Rift's segmented HTTP/HTTPS/FTP download engine on libcurl (PLAN 4.14).
// Runs on its own thread. No Qt. The app polls snapshot() on a timer.
#pragma once
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rift::http {

enum class State { Queued, Probing, Downloading, Paused, Completed, Error };

struct Options {
    int maxConnections = 8;          // 1..32; the engine ramps up to this while it helps
    int64_t minSegment = 1 << 20;    // never split below 1 MiB
    int stallSeconds = 15;           // no bytes for this long → reconnect that segment
    int maxRetries = 12;             // per download, across all connections
    int64_t speedLimit = 0;          // bytes/s for this download, 0 = unlimited
    bool verifyTls = true;
    std::string userAgent;
    std::string referer;
    std::string cookie;              // from the browser extension; kept in memory only
    std::vector<std::string> headers; // extra "Name: value" lines
};

struct Connection {
    int64_t start = 0, cursor = 0, end = 0;
    double speed = 0;
};

struct Status {
    uint64_t id = 0;
    State state = State::Queued;
    std::string url, finalUrl;
    std::string dir, fileName;       // final name (without .riftpart)
    int64_t size = -1;               // -1 = unknown
    int64_t done = 0;
    double speed = 0;                // smoothed bytes/s
    double speedNow = 0;             // last 1 s
    double firstConnSpeed = 0;       // measured speed with one connection (acceleration indicator)
    bool resumable = false;
    int retries = 0;
    std::vector<Connection> connections;
    std::string doneRanges;          // RangeSet::serialize(), persisted by the app
    std::string etag, lastModified;
    std::string error;
    int httpCode = 0;
};

struct AddRequest {
    uint64_t id = 0;
    std::string url;
    std::string dir;
    std::string fileName;            // optional; otherwise from headers/URL
    Options options;
    bool startPaused = false;
    // Resume state from a previous run (all optional)
    int64_t size = -1;
    std::string doneRanges, etag, lastModified, finalUrl;
    bool resumable = false;
};

class Engine {
public:
    Engine();
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void add(const AddRequest &req);
    void pause(uint64_t id);
    void resume(uint64_t id);
    void restart(uint64_t id);                 // from zero
    void remove(uint64_t id, bool deleteFile);
    void setOptions(uint64_t id, const Options &o);
    void setGlobalLimit(int64_t bytesPerSec);  // 0 = unlimited; split across all connections
    void setMaxActive(int n);                  // queue: max simultaneous downloads
    std::vector<Status> snapshot() const;

    static std::string version();              // libcurl version string

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace rift::http
