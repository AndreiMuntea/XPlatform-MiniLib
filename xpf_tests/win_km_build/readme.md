## Windows KM Test run and setup

This is intended to help building the library as a Windows KM library.
The vcxproj is manually created to import all .cpp files using wildcards.
Do not modify this. It was created this way so it will be automatically adjusted.
For now we're using msbuild to build for KM. The easiest way for this is to have a vcxproj with all settings.
This can be changed to fit your needs. Feel free to use it as you want :) 


To run, just open a visual studio developer command prompt and run the config that you want:
```
   msbuild .\xpf_tests_winkm.vcxproj /p:configuration=WinKmDebug /p:platform=x64 /t:Clean;Rebuild
   msbuild .\xpf_tests_winkm.vcxproj /p:configuration=WinKmDebug /p:platform=Win32 /t:Clean;Rebuild
 
   msbuild .\xpf_tests_winkm.vcxproj /p:configuration=WinKmRelease /p:platform=x64 /t:Clean;Rebuild
   msbuild .\xpf_tests_winkm.vcxproj /p:configuration=WinKmRelease /p:platform=Win32 /t:Clean;Rebuild
```
They already have a dependency on the xpf-lib.
To load the driver just ensure you have a virtual machine with testsigning and debug mode on
```
    bcdedit.exe -set TESTSIGNING ON
    bcdedit.exe /debug ON
```
A reboot is required.


The logs require the `"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Debug Print Filter"` key to exist.
If it does not, it should be created and `DEFAULT: REG_DWORD : 0xFFFFFFFF` should be specified.
A reboot is required.

To start the driver you can copy the `.sys` file to virtual machine after doing the aformentioned steps then just install as a simple driver:
```
    sc create xtest binPath= c:\path\to\sys\xpf_tests_winkm.sys
    sc start xtest
```
The test will start running (the start will be blocked until they are finished.)

To uninstall the driver:
```
    sc stop xtest
    sc delete xtest
```


To view the logs you can also use "Debug View" utility from system internals.

```
<snip>

[================================] 
[*] Executing scenario 'void __cdecl TestThreadPool::Create::TestImpl(long *) noexcept'! 
    > 133382188831143943 (100 ns) test start time; 
    > 133382188831323139 (100 ns) test end time; 
[*] [SUCCESS] Test finished with status 0x00000000. Delta 17 (ms). 
[================================] 

[================================] 
[*] Executing scenario 'void __cdecl TestThreadPool::EnqueueRundown::TestImpl(long *) noexcept'! 
    > 133382188831477099 (100 ns) test start time; 
    > 133382188831675273 (100 ns) test end time; 
[*] [SUCCESS] Test finished with status 0x00000000. Delta 19 (ms). 
[================================] 

[================================] 
[*] Executing scenario 'void __cdecl TestThreadPool::Stress::TestImpl(long *) noexcept'! 
    > 133382188831821088 (100 ns) test start time; 
    > 133382188852549520 (100 ns) test end time; 
[*] [SUCCESS] Test finished with status 0x00000000. Delta 2072 (ms). 
[================================] 

Finished execution of 98 tests in 5578 (ms).
Passed tests: 98 out of 98 (100%). 
```