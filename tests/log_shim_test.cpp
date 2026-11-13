// GFOpenTorrent host test: compat/boostlog's BOOST_LOG_TRIVIAL, the way
// PicoTorrent's sources use it (every use is BOOST_LOG_TRIVIAL(info|warning|
// error|fatal) << ...; run-host-tests.sh checks the tree for others). One
// line a check, "log_shim_test: N failed" at the end.
// Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
#include <boost/log/trivial.hpp>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <system_error>
#include <vector>

static int failed = 0;
static void check(const char *what, bool ok, const std::string &detail = std::string())
{
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
    std::fflush(stdout);
    if (!ok) failed++;
}

int main()
{
    std::vector<std::string> lines;
    gfot::log::set_sink([&](const std::string &l) { lines.push_back(l); });

    BOOST_LOG_TRIVIAL(info) << "PicoTorrent starting up...";
    check("one line a statement", lines.size() == 1);
    const std::regex fmt(R"(^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{6} \[\d\d:\d\d:\d\d\.\d{6}\] \[main\] info: PicoTorrent starting up\.\.\.$)");
    check("PicoTorrent's line format", !lines.empty() && std::regex_match(lines[0], fmt), lines.empty() ? "" : lines[0]);

    lines.clear();
    int peers = 3;
    std::error_code ec = std::make_error_code(std::errc::no_such_file_or_directory);
    BOOST_LOG_TRIVIAL(warning) << "peers: " << peers << ", " << 1.5;
    BOOST_LOG_TRIVIAL(error) << "Failed to check if database path exists: " << ec;
    BOOST_LOG_TRIVIAL(fatal) << "hex " << std::hex << 255;
    auto tail = [&](size_t i) { return i < lines.size() ? lines[i].substr(lines[i].find("] [main] ") + 9) : std::string(); };
    check("warning, numbers", tail(0) == "warning: peers: 3, 1.5", tail(0));
    check("error_code as std::ostream prints it", tail(1).rfind("error: Failed to check if database path exists: generic:", 0) == 0, tail(1));
    check("fatal, manipulator", tail(2) == "fatal: hex ff", tail(2));

    lines.clear();
    std::wstring w = L"Caf\u00e9 \u2603";
    BOOST_LOG_TRIVIAL(info) << L"wide " << w << " " << std::filesystem::path("Work:Downloads/x.torrent");
    check("wide text as UTF-8, path as its operator<<", tail(0) == "info: wide Caf\xc3\xa9 \xe2\x98\x83 \"Work:Downloads/x.torrent\"", tail(0));

    lines.clear();
    gfot::log::set_min_severity(boost::log::trivial::warning);
    BOOST_LOG_TRIVIAL(info) << "dropped";
    BOOST_LOG_TRIVIAL(error) << "kept";
    check("below the minimum severity: dropped", lines.size() == 1 && tail(0) == "error: kept");
    gfot::log::set_min_severity(boost::log::trivial::trace);

    // the file log appends, one line a record
    char tmpl[] = "/tmp/gfotlog-XXXXXX";
    const char *dir = mkdtemp(tmpl);
    std::string path = std::string(dir ? dir : "/tmp") + "/PicoTorrent.log";
    gfot::log::set_sink(nullptr);
    check("add_file_log opens the file", gfot::log::add_file_log(path));
    BOOST_LOG_TRIVIAL(info) << "one";
    BOOST_LOG_TRIVIAL(info) << "two";
    check("add_file_log on a missing directory fails", !gfot::log::add_file_log("/nonexistent-gfot/x.log"));
    BOOST_LOG_TRIVIAL(info) << "three";
    std::ifstream f(path);
    std::vector<std::string> got;
    for (std::string l; std::getline(f, l);) got.push_back(l.substr(l.find("] [main] ") + 9));
    check("file has the lines in order, a failed reopen keeps it",
          got == std::vector<std::string>{"info: one", "info: two", "info: three"}, std::to_string(got.size()) + " lines");

    std::printf("log_shim_test: %d failed\n", failed);
    return failed ? 1 : 0;
}
