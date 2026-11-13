/* GFOpenTorrent lab probe: prints what gfot_interfaces() finds, one line an
 * interface, then "ifqprobe: N failed". Checks that the stack's interface
 * has an address that isn't 0.0.0.0 and a netmask, that it is up, and that
 * lo0 is there once. Run in a lab: it needs bsdsocket.library 4.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "gfot_ifq.h"

static int failed;
static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failed++;
}

int main(void)
{
    struct gfot_iface rows[16];
    int n = gfot_interfaces(rows, 16), i, lo = 0, other = 0, other_ok = 0;
    if (n < 0)
        printf("gfot_interfaces: -1, errno %d (%s)\n", errno, strerror(errno));
    for (i = 0; i < n; i++) {
        printf("  %-16s %u.%u.%u.%u/%u.%u.%u.%u %s\n", rows[i].name,
               rows[i].addr[0], rows[i].addr[1], rows[i].addr[2], rows[i].addr[3],
               rows[i].mask[0], rows[i].mask[1], rows[i].mask[2], rows[i].mask[3],
               rows[i].up ? "up" : "down");
        if (rows[i].addr[0] == 127) lo++;
        else {
            other++;
            if ((rows[i].addr[0] | rows[i].addr[1] | rows[i].addr[2] | rows[i].addr[3])
                && rows[i].mask[0] && rows[i].up)
                other_ok++;
        }
    }
    check("gfot_interfaces() answers", n >= 0);
    check("one loopback row", lo == 1);
    check("the stack's interface: address, netmask, up", other >= 1 && other_ok == other);
    printf("ifqprobe: %d failed\n", failed);
    return failed ? 20 : 0;
}
