// GFOpenTorrent: PicoTorrent's core/environment.cpp for AmigaOS, in place of
// the Windows one (known folders, the registry, Boost.Log's file setup).
// Everything lives in the program's drawer, PROGDIR:, as a portable
// PicoTorrent does on Windows: PicoTorrent.sqlite, coredb.sqlite, logs/ and
// Downloads/. ENV:GFOpenTorrent/Downloads names another download drawer.
// The language comes from locale.library's default locale.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include <memory>
#include "picotorrent/core/environment.hpp"

#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

#include "gfot_env.hpp"

#if defined __amigaos__ || defined __AMIGA__
#include <proto/exec.h>
#include <proto/locale.h>
#include <libraries/locale.h>
#endif

namespace fs = std::filesystem;
using pt::Core::Environment;

namespace {
std::string progdir()
{
#if defined __amigaos__ || defined __AMIGA__
    return "PROGDIR:";
#else
    // host tests: the drawer the test gives
    const char *d = std::getenv("GFOT_PROGDIR");
    return d ? d : ".";
#endif
}

std::string env_var(const char *name)
{
    // libnix's getenv() reads ENV:, and "GFOpenTorrent/Downloads" is the
    // file ENV:GFOpenTorrent/Downloads
    const char *v = std::getenv(name);
    std::string s = v ? v : "";
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}
}

Environment::Environment()
    : m_startupTime(std::chrono::system_clock::now())
{
}

std::shared_ptr<Environment> Environment::Create()
{
    auto env = std::shared_ptr<Environment>(new Environment());
    std::string logs = gfot::env::join(progdir(), "logs");
    mkdir(logs.c_str(), 0755);
    if (!gfot::log::add_file_log(env->GetLogFilePath().string()))
        BOOST_LOG_TRIVIAL(warning) << "No log file in " << logs << ", logging to stderr";
    BOOST_LOG_TRIVIAL(info) << "PicoTorrent starting up...";
    return env;
}

fs::path Environment::GetApplicationDataPath() { return fs::path(progdir()); }
fs::path Environment::GetApplicationPath() { return fs::path(progdir()); }
std::string Environment::GetCrashpadReportUrl() { return std::string(); }

fs::path Environment::GetCoreDbFilePath()
{
    return fs::path(gfot::env::join(progdir(), "coredb.sqlite"));
}

std::string Environment::GetCurrentLocale()
{
#if defined __amigaos__ || defined __AMIGA__
    std::string name;
    if (struct Library *LocaleBase = OpenLibrary((CONST_STRPTR) "locale.library", 38)) {
        if (struct Locale *loc = OpenLocale(nullptr)) {
            if (loc->loc_LanguageName) name = (const char *)loc->loc_LanguageName;
            CloseLocale(loc);
        }
        CloseLibrary(LocaleBase);
    }
    if (name.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to get current locale - defaulting to 'en'";
        return "en";
    }
    return gfot::env::language_code(name);
#else
    return gfot::env::language_code(env_var("GFOT_LANGUAGE"));
#endif
}

fs::path Environment::GetDatabaseFilePath()
{
    return fs::path(gfot::env::join(progdir(), "PicoTorrent.sqlite"));
}

fs::path Environment::GetKnownFolderPath(Environment::KnownFolder knownFolder)
{
    switch (knownFolder) {
    case KnownFolder::LocalAppData:
        return GetApplicationDataPath();
    case KnownFolder::UserDownloads: {
        std::string d = env_var("GFOpenTorrent/Downloads");
        return fs::path(d.empty() ? gfot::env::join(progdir(), "Downloads") : d);
    }
    }
    BOOST_LOG_TRIVIAL(fatal) << "Unknown KnownFolder specified: " << static_cast<int>(knownFolder);
    throw std::runtime_error("Unknown folder");
}

fs::path Environment::GetLogFilePath()
{
    std::time_t t = std::chrono::system_clock::to_time_t(m_startupTime);
    return fs::path(gfot::env::join(gfot::env::join(progdir(), "logs"), gfot::env::log_file_name(t)));
}

bool Environment::IsAppContainerProcess() { return false; }
bool Environment::IsInstalled() { return false; }
