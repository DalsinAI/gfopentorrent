/* GFOpenTorrent: the stack's IPv4 interfaces for libtorrent's enum_net.cpp
 * (patches/libtorrent/0001-amiga-platform.patch, TORRENT_USE_AMIGA_IFQ).
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#ifndef GFOT_IFQ_H
#define GFOT_IFQ_H
#ifdef __cplusplus
extern "C" {
#endif
struct gfot_iface {
    char name[16];
    unsigned char addr[4];   /* network order */
    unsigned char mask[4];
    int up;
};
/* Fills in up to max interfaces, loopback included (lo0, 127.0.0.1/8, added
 * when the stack doesn't list it). Returns how many, or -1 with errno set. */
int gfot_interfaces(struct gfot_iface *out, int max);
/* The loopback row and the dedupe, shared by both builds */
int gfot_add_loopback(struct gfot_iface *out, int n, int max);
#ifdef __cplusplus
}
#endif
#endif
