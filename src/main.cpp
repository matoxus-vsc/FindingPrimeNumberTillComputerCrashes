// =============================================================================
//  FindingPrimeNumberTillComputerCrashes
// -----------------------------------------------------------------------------
//  Finds prime numbers. Forever. And remembers every single one of them,
//  like a clingy ex, until your RAM files for divorce.
//
//  Algorithm: multithreaded segmented Sieve of Eratosthenes (odd numbers only).
//  Every CPU core gets its own segment to sieve, because sharing is caring
//  and idle cores are a waste of perfectly good electricity.
// =============================================================================

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <new>
#include <string>
#include <thread>
#include <vector>

using u64 = std::uint64_t;

// How many numbers each thread chews through per round.
// 16M numbers -> 8 MB sieve per thread (we only store odd numbers,
// even numbers above 2 are dead to us).
constexpr u64 SEGMENT = 1ull << 24;

// Every prime we've ever found. Never forgotten. Never freed. Never forgiven.
static std::vector<u64> g_primes;

// Stats, read by the reporter thread.
static std::atomic<u64>  g_count{0};
static std::atomic<u64>  g_largest{0};
static std::atomic<u64>  g_checkedUpTo{0};
static std::atomic<bool> g_running{true};

static const auto g_start = std::chrono::steady_clock::now();

// ------------------------------------------------------------------- helpers

static double elapsedSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - g_start).count();
}

static u64 processRamMB() {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    GetProcessMemoryInfo(GetCurrentProcess(),
                         reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
    return pmc.PrivateUsage / (1024 * 1024);
}

static unsigned systemMemoryLoad() {
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    return ms.dwMemoryLoad;  // % of total system RAM currently in use
}

// 1234567 -> "1,234,567"
static std::string fmt(u64 v) {
    std::string s = std::to_string(v);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(i, ",");
    return s;
}

static std::string fmtTime(double sec) {
    const u64 t = static_cast<u64>(sec);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02llu:%02llu:%02llu",
                  t / 3600, (t / 60) % 60, t % 60);
    return buf;
}

// How is the computer feeling right now?
static const char* mood(unsigned memLoad) {
    if (memLoad < 50) return "chillin'";
    if (memLoad < 70) return "warming up";
    if (memLoad < 80) return "sweating";
    if (memLoad < 90) return "regretting life choices";
    if (memLoad < 95) return "writing its will";
    return "seeing the light";
}

static void saveProgress(const char* status) {
    std::ofstream f("progress.txt", std::ios::trunc);
    if (!f) return;
    f << "status:          " << status << "\n"
      << "uptime:          " << fmtTime(elapsedSeconds()) << "\n"
      << "primes found:    " << g_count.load() << "\n"
      << "largest prime:   " << g_largest.load() << "\n"
      << "checked up to:   " << g_checkedUpTo.load() << "\n"
      << "process RAM MB:  " << processRamMB() << "\n";
}

// --------------------------------------------------------------------- sieve

// Plain old sieve for the first range [0, limit) - gives us the base primes.
static void initialSieve(u64 limit) {
    std::vector<std::uint8_t> composite(limit, 0);
    for (u64 i = 2; i * i < limit; ++i)
        if (!composite[i])
            for (u64 j = i * i; j < limit; j += i) composite[j] = 1;
    for (u64 i = 2; i < limit; ++i)
        if (!composite[i]) g_primes.push_back(i);
}

// Sieves [lo, hi), lo is even. Index i represents the number lo + 2i + 1.
// Reads g_primes (read-only - nobody touches it while workers are running).
static void sieveSegment(u64 lo, u64 hi, std::vector<u64>& out) {
    const u64 n = (hi - lo) / 2;
    std::vector<std::uint8_t> composite(n, 0);

    for (size_t k = 1; k < g_primes.size(); ++k) {  // k = 1 -> skip 2, the weird one
        const u64 p = g_primes[k];
        if (p * p >= hi) break;
        u64 start = (lo + p - 1) / p * p;           // first multiple >= lo
        if (start < p * p) start = p * p;
        if ((start & 1) == 0) start += p;           // odd multiples only
        for (u64 j = start; j < hi; j += 2 * p) composite[(j - lo - 1) / 2] = 1;
    }

    out.clear();
    for (u64 i = 0; i < n; ++i)
        if (!composite[i]) out.push_back(lo + 2 * i + 1);
}

// ------------------------------------------------------------------ reporter

static void reporter() {
    u64 lastCount = 0;
    double lastTime = 0.0;
    int tick = 0;

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!g_running) break;

        const double now   = elapsedSeconds();
        const u64    count = g_count.load();
        const u64    rate  = static_cast<u64>((count - lastCount) / std::max(now - lastTime, 1e-9));
        const unsigned mem = systemMemoryLoad();
        lastCount = count;
        lastTime  = now;

        std::printf("\r[%s] primes: %s | biggest: %s | %s/s | RAM: %s MB | system %u%% (%s)      ",
                    fmtTime(now).c_str(), fmt(count).c_str(), fmt(g_largest.load()).c_str(),
                    fmt(rate).c_str(), fmt(processRamMB()).c_str(), mem, mood(mem));
        std::fflush(stdout);

        if (++tick % 10 == 0) saveProgress("still alive, somehow");
    }
}

// ---------------------------------------------------------------------- main

int main() {
    const unsigned threads = std::max(1u, std::thread::hardware_concurrency());

    std::printf(
        "\n"
        "  +--------------------------------------------------------+\n"
        "  |   FINDING PRIME NUMBERS TILL COMPUTER CRASHES          |\n"
        "  |   2, 3, 5, 7, 11, 13 ... see you on the other side     |\n"
        "  +--------------------------------------------------------+\n"
        "\n");
    std::printf("  Threads recruited : %u (all of them, obviously)\n", threads);
    std::printf("  Segment size      : %s numbers\n", fmt(SEGMENT).c_str());
    std::printf("  Exit strategy     : none. Close the window if you get scared.\n\n");

    std::thread rep(reporter);
    const char* status = "finished (wait, how?)";

    try {
        initialSieve(SEGMENT);
        g_count       = g_primes.size();
        g_largest     = g_primes.back();
        g_checkedUpTo = SEGMENT;

        u64 lo = SEGMENT;
        std::vector<std::vector<u64>> results(threads);
        std::atomic<bool> oom{false};

        for (;;) {
            std::vector<std::thread> workers;
            workers.reserve(threads);
            for (unsigned t = 0; t < threads; ++t) {
                workers.emplace_back([&, t] {
                    try {
                        sieveSegment(lo + t * SEGMENT, lo + (t + 1) * SEGMENT, results[t]);
                    } catch (const std::bad_alloc&) {
                        oom = true;  // exceptions can't escape a thread, so we tattle instead
                    }
                });
            }
            for (auto& w : workers) w.join();
            if (oom) throw std::bad_alloc();

            for (auto& r : results) g_primes.insert(g_primes.end(), r.begin(), r.end());

            lo += threads * SEGMENT;
            g_count       = g_primes.size();
            g_largest     = g_primes.back();
            g_checkedUpTo = lo;
        }
    } catch (const std::bad_alloc&) {
        status = "OUT OF MEMORY - the computer has given up";
    }

    g_running = false;
    rep.join();
    saveProgress(status);

    std::printf("\n\n  ========================================================\n");
    std::printf("   %s\n", status);
    std::printf("  ========================================================\n");
    std::printf("   Survived      : %s\n", fmtTime(elapsedSeconds()).c_str());
    std::printf("   Primes found  : %llu\n", static_cast<unsigned long long>(g_count.load()));
    std::printf("   Biggest prime : %llu\n", static_cast<unsigned long long>(g_largest.load()));
    std::printf("   Saved to progress.txt for future generations.\n");
    std::printf("  ========================================================\n\n");
    std::printf("  F to pay respects (or just press Enter)...");
    std::cin.get();
    return 0;
}
