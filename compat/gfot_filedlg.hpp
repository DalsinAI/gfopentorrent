// GFOpenTorrent: the "Add torrent" file picker's pattern list, in the form
// wxFileDialog takes ("Torrent files|*.torrent|All files|*"). PicoTorrent
// gives it as (name, pattern) pairs for Windows' IFileOpenDialog. Windows'
// "*.*" means every file; wx's own match wants a dot, so it becomes "*"
// and a file with no dot in its name still shows.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace gfot::filedlg
{
    inline std::wstring pattern(std::wstring p)
    {
        std::wstring out;
        size_t start = 0;
        for (;;)
        {
            size_t semi = p.find(L';', start);
            std::wstring one = p.substr(start, semi == std::wstring::npos ? std::wstring::npos : semi - start);
            if (one == L"*.*") one = L"*";
            if (!one.empty())
            {
                if (!out.empty()) out += L';';
                out += one;
            }
            if (semi == std::wstring::npos) break;
            start = semi + 1;
        }
        return out.empty() ? L"*" : out;
    }

    inline std::wstring wildcard(std::vector<std::tuple<std::wstring, std::wstring>> const& types)
    {
        std::wstring out;
        for (auto const& t : types)
        {
            std::wstring name = std::get<0>(t);
            for (auto& c : name) if (c == L'|') c = L' ';
            if (!out.empty()) out += L'|';
            out += name + L'|' + pattern(std::get<1>(t));
        }
        return out.empty() ? std::wstring(L"*|*") : out;
    }
}
