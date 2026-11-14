# CMake toolchain for GFOpenTorrent's AmigaOS build (68040, soft float with
# the stoves' run-time FPU check), after gfamigaamule's.
#   -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-m68k-amigaos.cmake
#   -DAMIGA_TOOLCHAIN=<stove>/prefix -DGFOPENBOOST_SDK=<sdk>
# Copyright (c) 2026 AmiCradle Team. MIT licence.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)
set(CMAKE_C_COMPILER "${AMIGA_TOOLCHAIN}/bin/m68k-amigaos-gcc")
set(CMAKE_CXX_COMPILER "${AMIGA_TOOLCHAIN}/bin/m68k-amigaos-g++")
set(CMAKE_AR "${AMIGA_TOOLCHAIN}/bin/m68k-amigaos-ar" CACHE FILEPATH "")
set(CMAKE_RANLIB "${AMIGA_TOOLCHAIN}/bin/m68k-amigaos-ranlib" CACHE FILEPATH "")
set(CMAKE_STRIP "${AMIGA_TOOLCHAIN}/bin/m68k-amigaos-strip" CACHE FILEPATH "")
# Asio on one task (GFOpenBoost's default; a bsdsocket.library base belongs
# to its task), its code from GFOpenBoost's libboost_asio.a
set(AMIGA_ASIO_DEFS "-DBOOST_ASIO_DISABLE_THREADS -DBOOST_ASIO_SEPARATE_COMPILATION")
set(CMAKE_C_FLAGS_INIT "-noixemul -m68040")
set(CMAKE_CXX_FLAGS_INIT "-noixemul -m68040 ${AMIGA_ASIO_DEFS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-noixemul -m68040")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH "${GFOPENBOOST_SDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# the stove's pthreads, for libtorrent's find_package(Threads)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(Threads_FOUND TRUE)
