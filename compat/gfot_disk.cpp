// GFOpenTorrent: free space on a torrent's volume (gfot_disk.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_disk.hpp"

#if defined(__amigaos__) || defined(__AMIGA__)
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>

bool gfot::disk::space(std::string const& path, std::uint64_t& free, std::uint64_t& total)
{
    free = total = 0;
    // A volume that is not mounted (a card or stick taken out) must read as
    // "no answer", not put up the system's "Please insert volume" requester
    // on every tick: no requesters for this process around the Lock.
    struct Process* me = (struct Process*)FindTask(nullptr);
    APTR window = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    BPTR lock = Lock((CONST_STRPTR)path.c_str(), SHARED_LOCK);
    me->pr_WindowPtr = window;
    if (!lock)
    {
        return false;
    }
    // Info() wants a longword-aligned InfoData: AllocVec gives one
    struct InfoData* id = (struct InfoData*)AllocVec(sizeof(struct InfoData), MEMF_PUBLIC | MEMF_CLEAR);
    bool ok = false;
    if (id && Info(lock, id))
    {
        std::uint64_t bpb = (std::uint64_t)id->id_BytesPerBlock;
        std::uint64_t blocks = (std::uint64_t)id->id_NumBlocks;
        std::uint64_t used = (std::uint64_t)id->id_NumBlocksUsed;
        total = blocks * bpb;
        free = used < blocks ? (blocks - used) * bpb : 0;
        ok = true;
    }
    if (id)
    {
        FreeVec(id);
    }
    UnLock(lock);
    return ok;
}
#else
#include <filesystem>
#include <system_error>

bool gfot::disk::space(std::string const& path, std::uint64_t& free, std::uint64_t& total)
{
    free = total = 0;
    std::error_code ec;
    std::filesystem::space_info si = std::filesystem::space(path, ec);
    if (ec)
    {
        return false;
    }
    free = si.available;
    total = si.capacity;
    return true;
}
#endif

float gfot::disk::free_part(std::uint64_t free, std::uint64_t total)
{
    if (total == 0)
    {
        return 1.0f;
    }
    if (free >= total)
    {
        return 1.0f;
    }
    return static_cast<float>(static_cast<double>(free) / static_cast<double>(total));
}
