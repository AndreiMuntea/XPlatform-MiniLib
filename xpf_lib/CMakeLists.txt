﻿# CMakeList.txt : CMake project for xpf, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# We are including "xpf.hpp" in our source files to have all definitions.
# This must be set here so we can reference them.
include_directories( "${CMAKE_CURRENT_LIST_DIR}/.." )


# Add sources to build process.
ADD_LIBRARY( "xpf_lib" STATIC "xpf.hpp"
                              "private/core/PlatformApi.cpp"
                              "private/Memory/LookasideListAllocator.cpp"
                              "private/Memory/SplitAllocator.cpp"
                              "private/Locks/BusyLock.cpp"
                              "private/Locks/ReadWriteLock.cpp"
                              "private/Containers/String.cpp"
                              "private/Containers/TwoLockQueue.cpp"
                              "private/Multithreading/Thread.cpp"
                              "private/Multithreading/Signal.cpp"
                              "private/Multithreading/RundownProtection.cpp"
                              "private/Multithreading/ThreadPool.cpp"
                              "private/Utility/EventFramework.cpp"
                              "private/Utility/ProtobufSerializer.cpp"
                              "private/Utility/PdbSymbolParser.cpp"
                              "private/Communication/Sockets/ServerSocket.cpp"
                              "private/Communication/Sockets/ClientSocket.cpp"
                              "private/Communication/Sockets/BerkeleySocket.cpp"
                              "private/Communication/Sockets/win_km/WskApi.cpp"
                              "private/Communication/http.cpp")


# Run the linter on lib - can be enabled when needed.
# add_custom_command(TARGET xpf_lib
#                    POST_BUILD
#                    COMMAND py -m cpplint --counting=detailed --linelength=130 --filter=-legal/copyright,-whitespace/braces,-whitespace/newline --recursive ${CMAKE_CURRENT_LIST_DIR}
#                    RESULT_VARIABLE lint_result)
# message($lint_result)
