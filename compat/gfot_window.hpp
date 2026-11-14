// GFOpenTorrent: the main window's size on its first start. Windows uses
// 450x400 device-independent pixels; the toolkit counts 72 DPI against
// Windows' 96, so that came out at about 340x300 on a 1920x1080 Workbench,
// too small for the list and the details. Three fifths of the screen's free
// area instead, at least 640x400, never more than the area.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

namespace gfot::window
{
    struct size { int w, h; };

    constexpr int fit(int want, int least, int room)
    {
        int v = want < least ? least : want;
        return room > 0 && v > room ? room : v;
    }

    // area_w x area_h: the screen's free area (wxGetClientDisplayRect)
    constexpr size first_size(int area_w, int area_h)
    {
        return { fit(area_w * 3 / 5, 640, area_w), fit(area_h * 3 / 5, 400, area_h) };
    }
}
