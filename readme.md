
## Purpose
This is intended to be a cross-platform library with minimal STL implementation without any exceptions.
It is mainly intended to provide cross-platform windows kernel-user mode code.
It is a minimalistic lib containing only the most used parts. It is still a work in progress.
I created this as a separated library to help me with my PHD project which requires a windows KM driver.
Then I decided to extract the code in a different project. More functionality will be added when needed.

Because this was originally intended for Windows, the code may be more Windows-Like (with NTSTATUS, SAL and so on).
I created all files required for compatibility with linux under "public/core/platform_specific" folder.
The same code will compile on all platforms without extra changes.

MSVC compiler is intended to use for Windows. Compile with /kernel switch for KM.
clang and gcc are supported for linux.

Other compilers and platforms can be added at a later date.


The project structure should look like:

    XPlatform-MiniLib
      |---- xpf-lib
      |---- xpf-tests

## xpf-lib
xpf-lib is the project which contains the actual library implementation.
It is a minimalistic library, and is intended to be kept simple.


## xpf-tests
XPF-Test project contains unit tests. Originally it used google test.
But because we can't run unit tests in KM using google test, a minimalistic cross platform
test framework was developed. You can see it under xpf_tests/Framework directory.


## To use:
In order to compile the project, open it in Visual Studio, or Visual Studio Code.
Can be ran directly from Visual Studio.

Or it can be ran from command line using cmake from top level directory:

> (for clang): CXX=clang++ CC=clang cmake -S . -B out && cd out && make && ctest -T memcheck --verbose

> (for gcc): CXX=g++ CC=gcc cmake -S . -B out && cd out && make && ctest -T memcheck --verbose

To link with this library, you need to provide an implementation for placement new and placement delete.
I decided to not include this in the library as it can conflict with the default one.
In user mode you can simply ensure that the header "new" is included.
This header is not available in Windows KM, thus an implementation is required, but that is trivial:
```
    void* operator new(size_t Size, void* Location) noexcept
    {
        (void)Size;
        return Location;
    }

    void operator delete(void*, void*) noexcept
    {
        return;
    }
```

### To build for Windows KM
In order to build for windows kernel mode, please see the xpf_lib/win_km_build directory.
It contains a separate readme file with the instructions.

### Use in your own project
To use the cross platform lib in your project, you just need to include "xpf.hpp" header and link with the library.
Alternatively you can change the cmake to build as a DLL rather than a static lib.
Your choice!


## Linter
The code is compliant with cpplint. You can find in CMakeList.txt a command to run the linter.
It is currently commented out as we don't want to force the user to have a dependency on it.
You can uncomment as you see fit.


## Documentation
Documentation can be autogenerated with Doxygen. The DoxyFile is included in the project.
The actual documentation is not included. It will be dropped inside the "out/Doxygen" directory.


## License
Please see the LICENSE file.
