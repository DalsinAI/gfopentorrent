/* GFOpenTorrent host test: compat/gfot_entropy.c, libtorrent's random source
 * on the Amiga. One line a check, "entropy_test: N failed" at the end.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <stdio.h>
#include <string.h>

void gfot_entropy(char *buf, unsigned long len);

static int failed = 0;
static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    fflush(stdout);
    if (!ok) failed++;
}

int main(void)
{
    char a[20], b[20], c[3] = {0x55, 0x55, 0x55}, big[65536];
    unsigned long count[256] = {0}, i, low = ~0UL, high = 0;
    gfot_entropy(a, sizeof a);
    gfot_entropy(b, sizeof b);
    check("two 20-byte peer IDs differ", memcmp(a, b, sizeof a) != 0);
    gfot_entropy(c, 2);
    check("a 2-byte request leaves the third byte alone", c[2] == 0x55);
    gfot_entropy(big, sizeof big);
    for (i = 0; i < sizeof big; i++) count[(unsigned char)big[i]]++;
    for (i = 0; i < 256; i++) { if (count[i] < low) low = count[i]; if (count[i] > high) high = count[i]; }
    /* 256 per value on average; a broken mixer piles them up */
    check("64 KB spreads over every byte value", low > 150 && high < 380);
    printf("INFO byte counts: %lu..%lu (256 expected)\n", low, high);
    printf("entropy_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
