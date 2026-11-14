// GFOpenTorrent: no crash reporter on AmigaOS. PicoTorrent's Windows build
// starts crashpad here; the Amiga build has none, so this does nothing.
// Copyright (c) 2026 AmiCradle Team. MIT licence.
#include "picotorrent/crashpadinitializer.hpp"

void pt::CrashpadInitializer::Initialize(std::shared_ptr<Core::Environment>) {}
