// GFOpenTorrent: free space on the volume a torrent saves to, in place of
// Windows' GetDiskFreeSpaceEx (MainFrame::CheckDiskSpace pauses a torrent
// when its volume runs low). AmigaOS asks Info() on a Lock of the path; the
// host build asks std::filesystem::space.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <cstdint>
#include <string>

namespace gfot
{
namespace disk
{
    // free and total bytes of the volume holding path (a drawer or a file);
    // false when the path does not lock or the volume will not say
    bool space(std::string const& path, std::uint64_t& free, std::uint64_t& total);

    // the volume's free part, 0.0 to 1.0; 1.0 when total is 0 (a volume
    // that gives no size is never "full", so nothing pauses for it)
    float free_part(std::uint64_t free, std::uint64_t total);
}
}
