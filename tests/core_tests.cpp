// Unit tests for src/core. No framework: each CHECK prints the failing expression.
#include "../src/core/arbiter.hpp"
#include "../src/core/segments.hpp"
#include "../src/core/speed.hpp"
#include "../src/core/text.hpp"
#include "../src/core/update.hpp"

#include <cstdio>
#include <random>
#include <string>

using namespace rift::core;

static int g_fail = 0, g_pass = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (x) ++g_pass;                                                                           \
        else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #x); }                \
    } while (0)
#define CHECK_EQ(a, b)                                                                             \
    do {                                                                                           \
        const auto va = (a); const auto vb = (b);                                                  \
        if (va == vb) ++g_pass;                                                                    \
        else { ++g_fail; std::printf("FAIL %s:%d  %s == %s\n", __FILE__, __LINE__, #a, #b); }      \
    } while (0)

static void testRangeSet() {
    RangeSet s;
    s.add({10, 20});
    s.add({0, 5});
    s.add({5, 10});
    CHECK_EQ(s.ranges().size(), size_t(2) - 1);
    CHECK_EQ(s.total(), int64_t(20));
    s.add({30, 40});
    CHECK_EQ(s.serialize(), std::string("0-20,30-40"));
    auto c = s.complement(50);
    CHECK_EQ(c.size(), size_t(2));
    CHECK(c[0] == (Range{20, 30}));
    CHECK(c[1] == (Range{40, 50}));
    auto p = RangeSet::parse("0-20,30-40,bad,7-3");
    CHECK_EQ(p.serialize(), std::string("0-20,30-40"));
    CHECK(s.contains(35) && !s.contains(25));
}

static void testSchedulerSteal() {
    SegmentScheduler sch(100, 10);
    auto a = sch.acquire(1);
    CHECK(a && a->start == 0 && a->end == 100);
    auto b = sch.acquire(2); // steals second half
    CHECK(b && b->start == 50 && b->end == 100);
    CHECK_EQ(sch.job(1)->end, int64_t(50));
    // Worker 1 receives more than its (shrunk) job: only 50 bytes accepted.
    CHECK_EQ(sch.accept(1, 0, 80), int64_t(50));
    CHECK(sch.job(1) == nullptr);
    CHECK_EQ(sch.accept(2, 50, 50), int64_t(50));
    CHECK(sch.complete());
    CHECK(!sch.acquire(3));
}

static void testSchedulerTooSmall() {
    SegmentScheduler sch(15, 10);
    CHECK(sch.acquire(1));
    CHECK(!sch.acquire(2)); // 15 < 2 * 10, no split
    sch.release(1);
    auto r = sch.acquire(2);
    CHECK(r && r->start == 0 && r->end == 15);
}

// Property test: random accept/release/acquire sequences cover every byte exactly once.
static void testSchedulerFuzz() {
    std::mt19937 rng(42);
    for (int round = 0; round < 300; ++round) {
        const int64_t size = 1 + rng() % 5000;
        SegmentScheduler sch(size, 1 + rng() % 64);
        if (rng() % 3 == 0) { RangeSet pre; pre.add({0, size / 3}); sch.setDone(pre); }
        std::vector<int> written(static_cast<size_t>(size), 0);
        for (int b = 0; b < size / 3; ++b) written[static_cast<size_t>(b)] = (sch.done().contains(b) ? 1 : 0);
        int guard = 0;
        while (!sch.complete() && guard++ < 200000) {
            const int w = static_cast<int>(rng() % 6);
            const Work *j = sch.job(w);
            if (!j) { sch.acquire(w); continue; }
            const int r = static_cast<int>(rng() % 10);
            if (r == 0) { sch.release(w); continue; }
            const int64_t len = 1 + static_cast<int64_t>(rng() % 200);
            const int64_t off = j->cursor;
            const int64_t ok = sch.accept(w, off, len);
            for (int64_t i = off; i < off + ok; ++i) written[static_cast<size_t>(i)]++;
        }
        bool exact = sch.complete();
        for (int v : written)
            if (v != 1) exact = false;
        CHECK(exact);
        if (!exact) break;
    }
}

static void testSpeed() {
    SpeedMeter m;
    int64_t t = 0;
    for (int i = 0; i < 40; ++i) { m.add(250'000, t); m.tick(t); t += 250; } // 1 MB/s for 10 s
    const double cur = m.current(t - 250);
    CHECK(cur > 900'000 && cur < 1'100'000);
    CHECK(m.smoothed(t - 250) > 700'000);
    CHECK_EQ(m.current(t + 2000), 0.0); // zero means zero within 2 s
    m.tick(t + 2100);
    CHECK_EQ(m.smoothed(t + 2100), 0.0);
    CHECK_EQ(m.total(), int64_t(10'000'000));
}

static void testFormat() {
    CHECK_EQ(formatBytes(0), std::string("0 B"));
    CHECK_EQ(formatBytes(1'500'000), std::string("1.50 MB"));
    CHECK_EQ(formatBytes(1'073'741'824, true), std::string("1 GiB"));
    CHECK_EQ(formatBytes(5'940'000'000LL), std::string("5.94 GB"));
    CHECK_EQ(formatSpeed(0), std::string("0 B/s"));
    CHECK_EQ(formatSpeed(4'800'000), std::string("4.80 MB/s"));
    CHECK_EQ(formatSpeed(1'000'000, true), std::string("8 Mbit/s"));
    CHECK_EQ(formatEta(-1), std::string("∞"));
    CHECK_EQ(formatEta(45), std::string("45s"));
    CHECK_EQ(formatEta(185), std::string("3m 05s"));
    CHECK_EQ(formatEta(7620), std::string("2h 07m"));
    CHECK_EQ(formatEta(273600), std::string("3d 4h"));
    CHECK_EQ(eta(100, 0), -1.0);
    CHECK_EQ(eta(100, 50), 2.0);
}

static void testNames() {
    CHECK_EQ(sanitizeFileName("a/b\\c:d*e?.txt"), std::string("a_b_c_d_e_.txt"));
    CHECK_EQ(sanitizeFileName("CON.txt"), std::string("_CON.txt"));
    CHECK_EQ(sanitizeFileName("com1"), std::string("_com1"));
    CHECK_EQ(sanitizeFileName("name. . "), std::string("name"));
    CHECK_EQ(sanitizeFileName(""), std::string("download"));
    CHECK_EQ(sanitizeFileName(".."), std::string("download"));
    CHECK_EQ(sanitizeFileName(std::string(300, 'x') + ".iso").size(), size_t(200));
    CHECK(isSafeRelativePath("a/b/c.txt"));
    CHECK(!isSafeRelativePath("../etc/passwd"));
    CHECK(!isSafeRelativePath("a/../../b"));
    CHECK(!isSafeRelativePath("/abs"));
    CHECK(!isSafeRelativePath("C:\\x"));
    CHECK_EQ(fileNameFromContentDisposition("attachment; filename=\"report 2024.pdf\""), std::string("report 2024.pdf"));
    CHECK_EQ(fileNameFromContentDisposition("attachment; filename*=UTF-8''na%C3%AFve%20file.zip; filename=\"x.zip\""),
             std::string("na\xC3\xAFve file.zip"));
    CHECK_EQ(fileNameFromContentDisposition("attachment; filename=plain.bin"), std::string("plain.bin"));
    CHECK_EQ(fileNameFromContentDisposition("inline"), std::string());
    CHECK_EQ(fileNameFromUrl("https://h.org/dir/My%20File.iso?x=1#f"), std::string("My File.iso"));
    CHECK_EQ(fileNameFromUrl("https://h.org/"), std::string());
    CHECK_EQ(hostFromUrl("https://user:pw@Download.Blender.org:443/x"), std::string("download.blender.org"));
    CHECK_EQ(totalFromContentRange("bytes 0-99/1234").value_or(-1), int64_t(1234));
    CHECK(!totalFromContentRange("bytes 0-99/*"));
    auto v = expandBatchPattern("https://h/f[001-120].jpg");
    CHECK_EQ(v.size(), size_t(120));
    CHECK_EQ(v.front(), std::string("https://h/f001.jpg"));
    CHECK_EQ(v.back(), std::string("https://h/f120.jpg"));
    CHECK_EQ(expandBatchPattern("https://h/[a-b].jpg").size(), size_t(1));
    CHECK_EQ(expandBatchPattern("x[1-5]y", 3).size(), size_t(3));
    CHECK_EQ(categoryForFileName("Movie.MKV"), std::string("Video"));
    CHECK_EQ(categoryForFileName("setup.exe"), std::string("Programs"));
    CHECK_EQ(categoryForFileName("ubuntu.iso"), std::string("Disc images"));
    CHECK_EQ(categoryForFileName("README"), std::string("Other"));
    const std::string mg = "magnet:?xt=urn:btih:3B8F2A1C0D4E9F5A&dn=Ubuntu+24.04%20ISO&tr=udp://x";
    CHECK(isMagnet("  " + mg));
    CHECK(isHttpUrl("HTTPS://x"));
    CHECK(!isHttpUrl("file:///x"));
    CHECK_EQ(magnetInfoHash(mg), std::string("3b8f2a1c0d4e9f5a"));
    CHECK_EQ(magnetDisplayName(mg), std::string("Ubuntu 24.04 ISO"));
}

static void testArbiter() {
    auto [a, b] = splitLimit(0, 5, 5);
    CHECK(a == 0 && b == 0);
    auto [t, h] = splitLimit(1'000'000, 0, 0);
    CHECK_EQ(t + h, int64_t(1'000'000));
    CHECK_EQ(h, int64_t(500'000));
    auto [t2, h2] = splitLimit(1'000'000, 900'000, 100'000);
    CHECK(t2 > h2 && h2 >= 150'000 && t2 + h2 == 1'000'000);
    auto [t3, h3] = splitLimit(1'000'000, 0, 1'000'000);
    CHECK_EQ(t3, int64_t(150'000));
    CHECK_EQ(h3, int64_t(850'000));
}

static void testUpdate() {
    CHECK(parseVersion("1.2.3") == (std::array<int, 3>{1, 2, 3}));
    CHECK(parseVersion("v10.0.42") == (std::array<int, 3>{10, 0, 42}));
    CHECK(!parseVersion("1.2"));
    CHECK(!parseVersion("1.2.3-dev"));
    CHECK(!parseVersion("1.2.3.4"));
    CHECK(!parseVersion(""));
    CHECK(!parseVersion("v"));
    CHECK(!parseVersion("1..3"));
    CHECK(isNewerVersion("v1.2.4", "1.2.3"));
    CHECK(isNewerVersion("1.10.0", "1.9.9"));   // numeric, not string, comparison
    CHECK(isNewerVersion("2.0.0", "1.99.99"));
    CHECK(!isNewerVersion("1.2.3", "1.2.3"));
    CHECK(!isNewerVersion("1.2.2", "1.2.3"));
    CHECK(!isNewerVersion("9.9.9", "0.0.0"));   // dev builds never self-update
    CHECK(!isNewerVersion("9.9.9", "1.0.0-dev"));
    CHECK(!isNewerVersion("garbage", "1.0.0"));
    CHECK_EQ(updateAssetName("v1.4.0", "windows-setup"), std::string("Rift-1.4.0-windows-x64-setup.exe"));
    CHECK_EQ(updateAssetName("1.4.0", "macos-arm64"), std::string("Rift-1.4.0-macos-arm64.zip"));
    CHECK_EQ(updateAssetName("1.4.0", "macos-x64"), std::string("Rift-1.4.0-macos-x64.zip"));
    CHECK_EQ(updateAssetName("1.4.0", "linux-appimage"), std::string("Rift-1.4.0-linux-x86_64.AppImage"));
    CHECK_EQ(updateAssetName("1.4.0", "linux-deb"), std::string());
    CHECK_EQ(updateAssetName("1.4", "windows-setup"), std::string());
    const std::string h1(64, 'a'), h2 = "0123456789ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef";
    const std::string sums = h1 + "  Rift-1.4.0-macos-arm64.zip\n" + h2 + " *Rift-1.4.0-windows-x64-setup.exe\r\n"
                             + "zz  broken\n";
    CHECK_EQ(sha256ForFile(sums, "Rift-1.4.0-macos-arm64.zip"), h1);
    CHECK_EQ(sha256ForFile(sums, "Rift-1.4.0-windows-x64-setup.exe"),
             std::string("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"));
    CHECK_EQ(sha256ForFile(sums, "Rift-1.4.0-macos-arm64"), std::string()); // exact names only
    CHECK_EQ(sha256ForFile(sums, "missing"), std::string());
}

int main() {
    testArbiter();
    testRangeSet();
    testSchedulerSteal();
    testSchedulerTooSmall();
    testSchedulerFuzz();
    testSpeed();
    testFormat();
    testNames();
    testUpdate();
    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
