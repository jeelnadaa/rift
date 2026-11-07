// TorrentEngine: the only part of Rift that includes libtorrent headers (PLAN 0.1, 3).
// One session; one alert thread; the app polls snapshot() and takeEvents() on a timer.
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rift::bt {

struct Settings {
    int listenPort = 0;            // 0 = random
    bool dht = true, lsd = true, upnp = true, natpmp = true, pex = true;
    int encryption = 0;            // 0 prefer, 1 require, 2 disable
    bool anonymous = false;
    int downloadLimit = 0;         // bytes/s, 0 = unlimited
    int uploadLimit = 0;
    int maxConnections = 500;
    int maxConnectionsPerTorrent = 100;
    int maxUploads = 20;
    int activeDownloads = 5, activeSeeds = 5, activeLimit = 15;
    bool queueing = true;
    std::string networkInterface;  // empty = all
    std::string version = "1.0.0"; // for user agent and peer id
    int proxyType = 0;             // 0 none, 1 socks5, 2 http
    std::string proxyHost;
    int proxyPort = 0;
};

struct FileEntry {
    int index = 0;
    std::string path;
    int64_t size = 0;
    int64_t done = 0;
    int priority = 4;              // 0 skip, 1 low, 4 normal, 7 high
};

struct Peer {
    std::string ip, client, flags, country;
    int port = 0;
    double progress = 0;
    int down = 0, up = 0;
    bool utp = false, encrypted = false, seed = false;
};

struct Tracker {
    std::string url, status, message;
    int tier = 0, seeds = -1, peers = -1, downloaded = -1;
};

struct TorrentStatus {
    std::string id;                // best info-hash, hex
    std::string name, savePath, error, currentTracker;
    std::string hashV1, hashV2, comment, creator;
    std::string state;             // metadata, checking, downloading, stalled, seeding, completed, paused, queued, error, moving
    int64_t size = 0, done = 0, totalSize = 0;
    int64_t downloaded = 0, uploaded = 0; // all time payload
    double progress = 0, ratio = 0, availability = 0;
    int down = 0, up = 0;          // payload bytes/s
    int seeds = 0, peers = 0, seedsTotal = -1, peersTotal = -1;
    int queuePos = -1;
    int downLimit = 0, upLimit = 0;
    int numFiles = 0, numPieces = 0, pieceLength = 0;
    int64_t addedTime = 0, completedTime = 0;
    bool hasMetadata = false, sequential = false, isPrivate = false, autoManaged = false, forced = false;
};

struct SessionInfo {
    int dhtNodes = 0;
    int listenPort = 0;
    bool listening = false;
    int portMapped = 0;            // 0 unknown, 1 mapped, -1 failed
    std::string externalIp;
    int64_t totalDown = 0, totalUp = 0; // session payload
};

struct Event {
    enum Type { Added, AddFailed, Metadata, Finished, Error, Removed, Moved, MoveFailed } type;
    std::string id, name, message;
};

struct Preview {
    std::string name, hash, comment, creator;
    int64_t size = 0;
    bool isPrivate = false;
    std::vector<FileEntry> files;
};

class Engine {
public:
    // sessionState: bytes from a previous saveSessionState() (DHT nodes etc.), may be empty.
    Engine(const Settings &s, const std::vector<char> &sessionState);
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void apply(const Settings &s);

    struct AddOptions {
        std::string savePath;
        bool paused = false, sequential = false, firstLast = false;
        std::vector<int> filePriorities;   // optional, by file index
        std::vector<std::string> trackers; // extra trackers
    };
    // Return the info-hash id on success; empty + error on failure. Adding is async: watch events.
    std::string addMagnet(const std::string &magnet, const AddOptions &o, std::string &error);
    std::string addTorrent(const std::vector<char> &data, const AddOptions &o, std::string &error);
    std::string addResume(const std::vector<char> &resumeData, std::string &error);
    static bool inspect(const std::vector<char> &torrentData, Preview &out, std::string &error);
    bool has(const std::string &id) const;

    void pause(const std::string &id);
    void resume(const std::string &id);
    void forceStart(const std::string &id);
    void remove(const std::string &id, bool deleteFiles);
    void recheck(const std::string &id);
    void reannounce(const std::string &id);
    void setSequential(const std::string &id, bool on);
    void setFirstLast(const std::string &id, bool on);
    void setFilePriority(const std::string &id, int index, int priority);
    void setLimits(const std::string &id, int down, int up);
    enum class QueueOp { Up, Down, Top, Bottom };
    void queue(const std::string &id, QueueOp op);
    void moveStorage(const std::string &id, const std::string &path);
    void addTracker(const std::string &id, const std::string &url);
    void pauseAll();
    void resumeAll();
    std::string magnet(const std::string &id) const;
    bool exportTorrent(const std::string &id, std::vector<char> &out);

    std::vector<TorrentStatus> snapshot() const;
    std::vector<FileEntry> files(const std::string &id) const;
    std::vector<Peer> peers(const std::string &id) const;
    std::vector<Tracker> trackers(const std::string &id) const;
    std::vector<uint8_t> pieceMap(const std::string &id, int buckets) const; // 0..255 have-ratio per bucket
    SessionInfo session() const;
    std::vector<Event> takeEvents();

    // Called on the alert thread with fresh resume data; store it (SQLite).
    std::function<void(const std::string &id, const std::vector<char> &data)> onResumeData;
    // Ask for resume data of torrents that changed (call every few minutes).
    void saveResumeData();
    // Stop: pause the session, flush resume data (waits up to timeoutMs), return session state.
    std::vector<char> shutdown(int timeoutMs = 5000);

    static std::string version();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace rift::bt
