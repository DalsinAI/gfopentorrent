/* GFOpenTorrent: the part of gfot_interfaces() both builds share.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <string.h>
#include "gfot_ifq.h"

int gfot_add_loopback(struct gfot_iface *out, int n, int max)
{
    int i;
    for (i = 0; i < n; i++)
        if (out[i].addr[0] == 127)
            return n;
    if (n >= max)
        return n;
    memset(&out[n], 0, sizeof out[n]);
    strcpy(out[n].name, "lo0");
    out[n].addr[0] = 127; out[n].addr[3] = 1;
    out[n].mask[0] = 255;
    out[n].up = 1;
    return n + 1;
}

int gfot_stack_has_ifq(unsigned version, unsigned neg_size)
{
    return version >= 4 && neg_size >= GFOT_IFQ_LAST_LVO;
}
