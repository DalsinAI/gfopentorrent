// GFOpenTorrent: the second start's half of the single copy (gfot_rexx.hpp).
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "gfot_rexx.hpp"

#if defined __amigaos__ || defined __AMIGA__
#include <exec/ports.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#endif

const char* const gfot::rexx::port_base = "GFOPENTORRENT";
const char* const gfot::rexx::first_port = "GFOPENTORRENT.1";

std::string gfot::rexx::quote(std::string const& s)
{
    std::string q = "\"";
    for (char c : s) {
        switch (c) {
        case '"': q += "*\""; break;
        case '*': q += "**"; break;
        case '\n': q += "*N"; break;
        case '\x1b': q += "*E"; break;
        default: q += c;
        }
    }
    return q + "\"";
}

std::vector<std::string> gfot::rexx::commands(
    std::vector<std::string> const& files,
    std::vector<std::string> const& magnets,
    std::string const& save_path,
    bool silent)
{
    std::string tail;
    if (!save_path.empty()) tail += " SAVEPATH " + quote(save_path);
    if (silent) tail += " SILENT";
    std::vector<std::string> out;
    auto add = [&](const char* name, std::vector<std::string> const& args) {
        if (args.empty()) return;
        std::string c = name;
        for (auto const& a : args) c += " " + quote(a);
        out.push_back(c + tail);
    };
    add("ADD", files);
    add("MAGNET", magnets);
    if (out.empty()) out.push_back("ACTIVATE");
    return out;
}

#if defined __amigaos__ || defined __AMIGA__
// opened only while send() runs (proto/rexxsyslib.h names it this way)
struct RxsLib* RexxSysBase;

bool gfot::rexx::first_running()
{
    Forbid();
    bool there = FindPort((CONST_STRPTR) first_port) != nullptr;
    Permit();
    return there;
}

bool gfot::rexx::send(std::string const& command)
{
    bool sent = false;
    RexxSysBase = (struct RxsLib*) OpenLibrary((CONST_STRPTR) "rexxsyslib.library", 36);
    if (!RexxSysBase) return false;
    if (struct MsgPort* reply = CreateMsgPort()) {
        if (struct RexxMsg* rm = CreateRexxMsg(reply, nullptr, nullptr)) {
            rm->rm_Args[0] = (STRPTR) CreateArgstring((CONST_STRPTR) command.c_str(), command.size());
            rm->rm_Action = RXCOMM;
            if (rm->rm_Args[0]) {
                Forbid();
                struct MsgPort* port = FindPort((CONST_STRPTR) first_port);
                if (port) PutMsg(port, &rm->rm_Node);
                Permit();
                if (port) {
                    // the reply must come back before the message goes
                    WaitPort(reply);
                    GetMsg(reply);
                    sent = rm->rm_Result1 == RC_OK;
                }
                DeleteArgstring((UBYTE*) rm->rm_Args[0]);
            }
            DeleteRexxMsg(rm);
        }
        DeleteMsgPort(reply);
    }
    CloseLibrary((struct Library*) RexxSysBase);
    RexxSysBase = nullptr;
    return sent;
}

void gfot::rexx::wait_gone(long ms)
{
    for (long t = 0; t < ms && first_running(); t += 100) Delay(5);
}
#else
bool gfot::rexx::first_running() { return false; }
bool gfot::rexx::send(std::string const&) { return false; }
void gfot::rexx::wait_gone(long) { }
#endif
