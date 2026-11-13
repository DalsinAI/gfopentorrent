// GFOpenTorrent: BOOST_LOG_TRIVIAL for PicoTorrent without Boost.Log, which
// GFOpenBoost doesn't build. compat/boostlog goes on PicoTorrent's include
// path ahead of the SDK, so its #include <boost/log/trivial.hpp> finds this.
// A record is one line, written when the statement ends; until
// gfot::log::add_file_log() names a file, lines go to stderr, as Boost.Log's
// default sink does. Line format, as PicoTorrent's own add_file_log gives:
//   2026-11-13 21:55:00.123456 [00:00:01.234567] [main] info: message
// The program runs on one task, so there is no lock.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#ifndef GFOT_BOOST_LOG_TRIVIAL_HPP
#define GFOT_BOOST_LOG_TRIVIAL_HPP
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>

namespace boost { namespace log { namespace trivial {
enum severity_level { trace, debug, info, warning, error, fatal };
}}}

namespace gfot { namespace log {
using severity = ::boost::log::trivial::severity_level;

// Appends lines to the file from now on. false when it can't be opened
// (lines then still go to stderr).
bool add_file_log(const std::string &path);
// Tests: every finished line, without its newline, goes to f instead
void set_sink(std::function<void(const std::string &)> f);
// Lines below this level are dropped (default trace: all of them)
void set_min_severity(severity s);
const char *severity_name(severity s);
std::string utf8(const wchar_t *s);

class record {
public:
    explicit record(severity s) : m_sev(s) {}
    record(const record &) = delete;
    record &operator=(const record &) = delete;
    ~record();
    // wide text (L"...", std::wstring) is written as UTF-8, as Boost.Log's
    // narrow stream converts it; everything else through its operator<<
    template <class T> record &operator<<(const T &v)
    {
        if constexpr (std::is_convertible<const T &, const wchar_t *>::value)
            m_os << utf8(v);
        else if constexpr (std::is_same<T, std::wstring>::value)
            m_os << utf8(v.c_str());
        else
            m_os << v;
        return *this;
    }
    // manipulators such as std::hex
    record &operator<<(std::ostream &(*m)(std::ostream &)) { m_os << m; return *this; }
private:
    severity m_sev;
    std::ostringstream m_os;
};
}}

#define BOOST_LOG_TRIVIAL(lvl) ::gfot::log::record(::boost::log::trivial::lvl)
#endif
