// GFOpenTorrent host test: PicoTorrent's Environment for AmigaOS
// (compat/picotorrent/environment_amiga.cpp) and the rules in gfot_env.cpp.
// On the host the drawer is GFOT_PROGDIR and the language GFOT_LANGUAGE, in
// place of PROGDIR: and locale.library. One line a check,
// "environment_test: N failed" at the end.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include <memory>
#include "picotorrent/core/environment.hpp"
#include "gfot_env.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <string>
#include <sys/stat.h>

using pt::Core::Environment;
static int failed = 0;
static void check(const char *what, bool ok, const std::string &detail = std::string())
{
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
    std::fflush(stdout);
    if (!ok) failed++;
}

int main()
{
    using namespace gfot::env;
    check("join after a volume", join("PROGDIR:", "logs") == "PROGDIR:logs", join("PROGDIR:", "logs"));
    check("join inside a drawer", join("Work:Torrents", "x.log") == "Work:Torrents/x.log");
    check("join after a trailing slash", join("Work:Torrents/", "x") == "Work:Torrents/x");
    check("join to nothing", join("", "x") == "x");

    std::tm tm = {};
    tm.tm_year = 126; tm.tm_mon = 10; tm.tm_mday = 3; tm.tm_hour = 9; tm.tm_min = 5; tm.tm_sec = 7; tm.tm_isdst = -1;
    check("log file name", log_file_name(std::mktime(&tm)) == "PicoTorrent.20261103090507.log", log_file_name(std::mktime(&tm)));

    check("deutsch.language is de", language_code("deutsch.language") == "de");
    check("Deutsch is de", language_code("Deutsch") == "de");
    check("francais in Latin-1 is fr", language_code("fran\xe7" "ais.language") == "fr");
    check("espa\xc3\xb1" "ol (UTF-8) is es", language_code("espa\xc3\xb1" "ol") == "es");
    check("norsk is nb", language_code("norsk") == "nb");
    check("unknown and empty are en", language_code("klingon.language") == "en" && language_code("") == "en");

    char tmpl[] = "/tmp/gfotenv-XXXXXX";
    const char *dir = mkdtemp(tmpl);
    check("temp drawer", dir != nullptr);
    if (!dir) return 1;
    std::string d = dir;
    setenv("GFOT_PROGDIR", (d + "/").c_str(), 1);   // as "PROGDIR:" ends in its separator
    setenv("GFOT_LANGUAGE", "italiano.language", 1);
    unsetenv("GFOpenTorrent/Downloads");
    auto env = Environment::Create();
    check("database in the drawer", env->GetDatabaseFilePath().string() == d + "/PicoTorrent.sqlite", env->GetDatabaseFilePath().string());
    check("coredb in the drawer", env->GetCoreDbFilePath().string() == d + "/coredb.sqlite");
    check("app data and app path are the drawer", env->GetApplicationDataPath().string() == d + "/" && env->GetApplicationPath().string() == d + "/");
    check("downloads default", env->GetKnownFolderPath(Environment::KnownFolder::UserDownloads).string() == d + "/Downloads");
    setenv("GFOpenTorrent/Downloads", "Work:Downloads\n", 1);   // ENV: files often end in a newline
    check("downloads from ENV:GFOpenTorrent/Downloads", env->GetKnownFolderPath(Environment::KnownFolder::UserDownloads).string() == "Work:Downloads");
    check("locale", env->GetCurrentLocale() == "it");
    check("not installed, no app container, no crash reports", !env->IsInstalled() && !env->IsAppContainerProcess() && env->GetCrashpadReportUrl().empty());

    std::string log = env->GetLogFilePath().string();
    check("log file in logs/", std::regex_match(log, std::regex(d + R"(/logs/PicoTorrent\.\d{14}\.log)")), log);
    std::ifstream f(log);
    std::string first;
    std::getline(f, first);
    check("Create() made logs/ and logs to the file", first.find("] [main] info: PicoTorrent starting up...") != std::string::npos, first);

    std::printf("environment_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
