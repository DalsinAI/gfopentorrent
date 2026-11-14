// GFOpenTorrent: free space on a torrent's volume (compat/gfot_disk.cpp),
// the host half; the AmigaOS half is the same sums on Info()'s blocks.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_disk.hpp"

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <string>

static int failed = 0;
static int passed = 0;

static void check(bool ok, char const* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    (ok ? passed : failed)++;
}

static bool near(float a, float b)
{
    return std::fabs(a - b) < 1e-6f;
}

int main(int argc, char** argv)
{
    std::string here = argc > 1 ? argv[1] : ".";
    std::uint64_t f = 7, t = 7;

    check(gfot::disk::space(here, f, t), "space() answers for a drawer that exists");
    check(t > 0, "the volume has a size");
    check(f <= t, "free is no more than the size");

    check(!gfot::disk::space(here + "/no/such/drawer/here", f, t), "space() is false for a path that does not lock");
    check(f == 0 && t == 0, "and gives 0 free, 0 total");

    check(near(gfot::disk::free_part(25, 100), 0.25f), "25 of 100 free is 0.25");
    check(near(gfot::disk::free_part(0, 100), 0.0f), "a full volume is 0.0");
    check(near(gfot::disk::free_part(100, 100), 1.0f), "an empty volume is 1.0");
    check(near(gfot::disk::free_part(0, 0), 1.0f), "no size means never full");
    check(near(gfot::disk::free_part(200, 100), 1.0f), "free over the size reads as empty");
    // 4 TB volume, 40 GB free: the sums stay 64-bit
    std::uint64_t tb4 = 4ull * 1024 * 1024 * 1024 * 1024;
    std::uint64_t gb40 = 40ull * 1024 * 1024 * 1024;
    check(near(gfot::disk::free_part(gb40, tb4), static_cast<float>(40.0 / 4096.0)), "a 4 TB volume with 40 GB free is not cut to 32 bits");
    // CheckDiskSpace pauses below limit/100: 5% free against a 10% limit
    check(gfot::disk::free_part(5, 100) < 10 / 100.0f, "5% free is below a 10% limit");
    check(!(gfot::disk::free_part(0, 0) < 10 / 100.0f), "a volume with no size never pauses");

    std::printf("disk_space_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
