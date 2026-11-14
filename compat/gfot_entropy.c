/* GFOpenTorrent: libtorrent's crypto_random_bytes() source on AmigaOS
 * (patch 0004), which has no /dev/urandom and no random device.
 *
 * Timing noise, not a hardware source: each call stirs in the time to the
 * microsecond, the CPU clock, a stack address and a counter (on the Amiga
 * also the EClock and the free memory), and output comes from SplitMix64
 * over that state. It is good enough for what libtorrent asks of it without
 * encryption (peer IDs, DHT node IDs and tokens, uTP connection IDs, and
 * seeding its mt19937). It is not for keys. A real source can come from
 * GFOpenTLS later.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <string.h>
#include <sys/time.h>
#include <time.h>
#if defined __amigaos__ || defined __AMIGA__
#include <proto/exec.h>
#include <exec/memory.h>
#endif

typedef unsigned long long u64;
static u64 state, counter;

static u64 mix(u64 z)
{
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static void stir(void)
{
    struct timeval tv;
    volatile char here = 0;
    gettimeofday(&tv, 0);
    state = mix(state ^ ((u64)tv.tv_sec << 20) ^ (u64)tv.tv_usec);
    state = mix(state ^ (u64)clock() ^ ((u64)(unsigned long)&here << 7));
    state = mix(state ^ ++counter);
#if defined __amigaos__ || defined __AMIGA__
    state = mix(state ^ (u64)AvailMem(MEMF_ANY) ^ ((u64)(unsigned long)FindTask(0) << 32));
#endif
}

void gfot_entropy(char *buf, unsigned long len)
{
    stir();
    while (len) {
        u64 v;
        unsigned long n = len < 8 ? len : 8;
        state += 0x9e3779b97f4a7c15ULL;
        v = mix(state);
        memcpy(buf, &v, n);
        buf += n;
        len -= n;
    }
}
