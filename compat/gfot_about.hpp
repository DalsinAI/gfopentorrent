// GFOpenTorrent: the About box's words for the toolkit it is built on, as
// teams/DECISION-WXWIDGETS-EXCEPTION-20261114-1543.md sets them (Dale,
// 14 Nov 15:43): the same words as the README. Header only.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#pragma once

namespace gfot
{
namespace about
{
    // the component list's name for the toolkit row
    inline const char* toolkit_name()
    {
        return "GFOpenWXToolkit (wxWidgets)";
    }

    // the notice, word for word as the decision gives it
    inline const char* wx_notice()
    {
        return "GFOpenWXToolkit is our AmigaOS port of wxWidgets, pinned to version 3.2.12. "
            "wxWidgets itself comes from the upstream project at build time and stays under its own licence; "
            "we link to the wxWidgets project (https://www.wxwidgets.org) and aim to stay compatible with it.";
    }
}
}
