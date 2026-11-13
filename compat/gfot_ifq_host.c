/* GFOpenTorrent: gfot_interfaces() on a Linux host, for the host tests
 * (TORRENT_AMIGA_HOST_TEST). getifaddrs() stands in for Roadshow's
 * interface calls; the rows are the same shape.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <errno.h>
#include <string.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "gfot_ifq.h"

int gfot_interfaces(struct gfot_iface *out, int max)
{
    struct ifaddrs *all, *i;
    int n = 0;
    if (getifaddrs(&all) != 0)
        return -1;
    for (i = all; i && n < max; i = i->ifa_next) {
        if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET)
            continue;
        memset(&out[n], 0, sizeof out[n]);
        strncpy(out[n].name, i->ifa_name, sizeof out[n].name - 1);
        memcpy(out[n].addr, &((struct sockaddr_in *)i->ifa_addr)->sin_addr, 4);
        if (i->ifa_netmask)
            memcpy(out[n].mask, &((struct sockaddr_in *)i->ifa_netmask)->sin_addr, 4);
        out[n].up = (i->ifa_flags & IFF_UP) != 0;
        n++;
    }
    freeifaddrs(all);
    return gfot_add_loopback(out, n, max);
}
