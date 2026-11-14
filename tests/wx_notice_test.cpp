// GFOpenTorrent: the wxWidgets notice (teams/DECISION-WXWIDGETS-EXCEPTION-
// 20261114-1543.md): the About box's words (compat/gfot_about.hpp) are the
// decision's, the README carries the same words, nothing calls the toolkit
// clean-room, and patch 0010 puts the notice in the About box. argv[1] is
// the repository root.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_about.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static int failed = 0;
static int passed = 0;

static void check(bool ok, char const* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    (ok ? passed : failed)++;
}

static std::string slurp(std::string const& path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// line breaks, quote marks and runs of spaces read as one space (the
// README wraps and quotes the notice)
static std::string flat(std::string s)
{
    std::string out;
    bool space = false;
    bool line_start = true;
    for (char c : s)
    {
        // a quoted block's "> " marks are not words
        if (line_start && c == '>')
        {
            continue;
        }
        line_start = (c == '\n');
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
        {
            space = true;
            continue;
        }
        if (space && !out.empty())
        {
            out += ' ';
        }
        space = false;
        out += c;
    }
    return out;
}

static std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

int main(int argc, char** argv)
{
    std::string notice = gfot::about::wx_notice();
    check(notice.rfind("GFOpenWXToolkit is our AmigaOS port of wxWidgets, pinned to version 3.2.12.", 0) == 0,
        "the notice starts as the decision does");
    check(notice.find("stays under its own licence") != std::string::npos, "it says wxWidgets keeps its own licence");
    check(notice.find("https://www.wxwidgets.org") != std::string::npos, "it links to the wxWidgets project");
    check(lower(notice).find("clean") == std::string::npos, "it never says clean-room");
    check(std::string(gfot::about::toolkit_name()).find("wxWidgets") != std::string::npos, "the About row names wxWidgets");

    if (argc > 1)
    {
        std::string root = argv[1];
        std::string readme = slurp(root + "/README.md");
        check(!readme.empty(), "README.md is there");
        check(flat(readme).find(notice) != std::string::npos, "the README has the notice word for word");
        check(readme.find("DECISION-WXWIDGETS-EXCEPTION-20261114-1543.md") != std::string::npos, "the README names the decision");
        check(lower(readme).find("clean-room") == std::string::npos && lower(readme).find("clean room") == std::string::npos,
            "the README never calls anything clean-room");
        std::string patch = slurp(root + "/patches/picotorrent/0010-about-wx-notice.patch");
        check(patch.find("+    desc->AppendText(gfot::about::wx_notice());") != std::string::npos, "patch 0010 adds the notice to the About box");
        check(patch.find("+    lv->InsertItem(lv->GetItemCount(), gfot::about::toolkit_name());") != std::string::npos,
            "and names the toolkit in its component list");
    }

    std::printf("wx_notice_test: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
