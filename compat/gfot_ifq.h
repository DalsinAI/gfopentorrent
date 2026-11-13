/* GFOpenTorrent: the stack's IPv4 interfaces for libtorrent's enum_net.cpp
 * (patches/libtorrent/0001-amiga-platform.patch, TORRENT_USE_AMIGA_IFQ).
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#ifndef GFOT_IFQ_H
#define GFOT_IFQ_H
#ifdef __cplusplus
extern "C" {
#endif
struct gfot_iface {   /* patch 0001 repeats this; run-host-tests.sh checks */
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
/* Roadshow's interface calls are bsdsocket.library slots 72-74
 * (ReleaseInterfaceList -432, ObtainInterfaceList -438,
 * QueryInterfaceTagList -444). A stack has them only when it is version 4 or
 * later AND its jump table reaches -444: AmiTCP 4.x is version 4 without
 * them. 1 when the calls are there, 0 when not. */
#define GFOT_IFQ_LAST_LVO 444
int gfot_stack_has_ifq(unsigned version, unsigned neg_size);
#ifdef __cplusplus
}
#endif
#endif
