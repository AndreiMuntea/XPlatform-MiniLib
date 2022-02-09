
## Purpose
This is intended to be a cross-platform library with minimal STL implementation without any exceptions. It is mainly intended to provide cross-platform windows kernel-user mode code.

MSVC compiler is intended to use for Windows. Compile with /kernel switch for KM.
clang and gcc are supported for linux.

Other compilers will be added at a later date (if needed).

## XPF-Test
XPF-Test project contains unit tests using gtest project to be cross platform.
Create a folder called "lib" and git clone the latest gtest library.

The project structure should looke like:



    XPlatform-MiniLib
      |---- XPF
            |---- XPlatform
      |---- XPF-Tests
      |---- lib
            |--- googletest


## To use:
In order to compile the project, open it in visual studio, or visual studio code.
Can be ran directly from visual studio (after git cloning the googletest).
Open a Developer Command Prompt console in the repo's root and run

> cmake -G "Visual Studio 17" .\CMakeLists.txt

This will generate the X-Platform.sln required and then you can load it in visual studio.
Please note that I am using Visual Studio 2022 (hence the 17). You might need to change that to work with yours.
To list available visual studio versions you can issue
> cmake -help 


Or it can be ran from command line using cmake from XPlatform-MiniLib folder:

> (for clang): CXX=clang++ CC=clang cmake -S . -B out && cd out && make && ctest -T memcheck --verbose

> (for gcc): CXX=g++ CC=gcc cmake -S . -B out && cd out && make && ctest -T memcheck --verbose


## Header-Only
This is intended to be a header-only code and can be used independently in other projects:
Copy the XPlatform folder from XPlatform-MiniLib/XPF and include "XPlatformIncludes.hpp".