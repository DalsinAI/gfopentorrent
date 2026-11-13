/* GFOpenTorrent: gfot_interfaces() on AmigaOS, through Roadshow's interface
 * calls in bsdsocket.library 4 (ObtainInterfaceList, QueryInterfaceTagList,
 * ReleaseInterfaceList). GFOpenSocket lists its one interface,
 * "gfopensocket0", and answers SIOCGIFCONF with EOPNOTSUPP, so this is the
 * way it tells libtorrent its address and netmask.
 * The library base comes from libnix's socket settings, as GFOpenBoost's
 * amiga_ioctl.c finds it.
 * Copyright (c) 2026 AmiCradle Team. MIT licence. */
#include <errno.h>
#include <string.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <utility/tagitem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libraries/bsdsocket.h>
#include "gfot_ifq.h"

struct gfot_socket_settings {          /* the head of libnix's SocketSettings */
    int lx_network_type;               /* 1: AmiTCP-style bsdsocket.library */
    struct Library *lx_BsdSocketBase;
};
extern __stdargs struct gfot_socket_settings *_lx_get_socket_settings(void);

#define SocketBase gfot_SocketBase
static struct Library *gfot_SocketBase;
#include <proto/bsdsocket.h>

int gfot_interfaces(struct gfot_iface *out, int max)
{
    struct gfot_socket_settings *ss = _lx_get_socket_settings();
    struct List *list;
    struct Node *node;
    int n = 0;

    if (!ss || ss->lx_network_type != 1 || !ss->lx_BsdSocketBase
        || !gfot_stack_has_ifq(ss->lx_BsdSocketBase->lib_Version, ss->lx_BsdSocketBase->lib_NegSize)) {
        errno = ENOSYS;
        return -1;
    }
    gfot_SocketBase = ss->lx_BsdSocketBase;
    list = ObtainInterfaceList();
    if (!list)
        return -1;                     /* errno from the stack */
    for (node = list->lh_Head; node->ln_Succ && n < max; node = node->ln_Succ) {
        struct sockaddr_in addr, mask;
        LONG state = SM_Down;
        memset(&addr, 0, sizeof addr);
        memset(&mask, 0, sizeof mask);
        struct TagItem tags[4];
        tags[0].ti_Tag = IFQ_Address;   tags[0].ti_Data = (ULONG)&addr;
        tags[1].ti_Tag = IFQ_NetMask;   tags[1].ti_Data = (ULONG)&mask;
        tags[2].ti_Tag = IFQ_State;     tags[2].ti_Data = (ULONG)&state;
        tags[3].ti_Tag = TAG_DONE;      tags[3].ti_Data = 0;
        if (QueryInterfaceTagList(node->ln_Name, tags) != 0)
            continue;
        if (addr.sin_family != AF_INET)
            continue;
        memset(&out[n], 0, sizeof out[n]);
        strncpy(out[n].name, node->ln_Name ? (const char *)node->ln_Name : "", sizeof out[n].name - 1);
        memcpy(out[n].addr, &addr.sin_addr, 4);
        memcpy(out[n].mask, &mask.sin_addr, 4);
        out[n].up = state == SM_Up;
        n++;
    }
    ReleaseInterfaceList(list);
    return gfot_add_loopback(out, n, max);
}
