// GFOpenTorrent: the record sink behind compat/boostlog/boost/log/trivial.hpp.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include <boost/log/trivial.hpp>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <sys/time.h>

namespace gfot { namespace log {
namespace {
std::FILE *g_file = nullptr;
std::function<void(const std::string &)> g_sink;
severity g_min = ::boost::log::trivial::trace;
const auto g_start = std::chrono::steady_clock::now();

std::string stamp()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    std::time_t t = tv.tv_sec;
    std::tm tm = *std::localtime(&t);
    char b[48];
    std::snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d:%02d.%06ld", tm.tm_year + 1900, tm.tm_mon + 1,
                  tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, long(tv.tv_usec));
    return b;
}

std::string uptime()
{
    long long us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - g_start).count();
    char b[40];
    std::snprintf(b, sizeof b, "%02lld:%02lld:%02lld.%06lld", us / 3600000000LL, us / 60000000LL % 60,
                  us / 1000000LL % 60, us % 1000000LL);
    return b;
}
}

bool add_file_log(const std::string &path)
{
    std::FILE *f = std::fopen(path.c_str(), "a");
    if (!f) return false;
    if (g_file) std::fclose(g_file);
    g_file = f;
    return true;
}

void set_sink(std::function<void(const std::string &)> f) { g_sink = std::move(f); }
void set_min_severity(severity s) { g_min = s; }

const char *severity_name(severity s)
{
    static const char *const n[] = {"trace", "debug", "info", "warning", "error", "fatal"};
    return unsigned(s) < 6 ? n[s] : "?";
}

std::string utf8(const wchar_t *s)
{
    std::string o;
    if (!s) return "(null)";
    for (; *s; s++) {
        unsigned long c = (unsigned long)*s;
        if (c < 0x80) o += char(c);
        else if (c < 0x800) { o += char(0xC0 | c >> 6); o += char(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { o += char(0xE0 | c >> 12); o += char(0x80 | (c >> 6 & 0x3F)); o += char(0x80 | (c & 0x3F)); }
        else if (c < 0x110000) { o += char(0xF0 | c >> 18); o += char(0x80 | (c >> 12 & 0x3F)); o += char(0x80 | (c >> 6 & 0x3F)); o += char(0x80 | (c & 0x3F)); }
        else o += '?';
    }
    return o;
}

record::~record()
{
    if (m_sev < g_min) return;
    std::string line = stamp() + " [" + uptime() + "] [main] " + severity_name(m_sev) + ": " + m_os.str();
    if (g_sink) { g_sink(line); return; }
    std::FILE *f = g_file ? g_file : stderr;
    std::fputs(line.c_str(), f);
    std::fputc('\n', f);
    std::fflush(f);
}
}}
