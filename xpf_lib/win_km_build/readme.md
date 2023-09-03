## Windows KM run and setup

This is intended to help building the library as a Windows KM library.
The vcxproj is manually created to import all .cpp files using wildcards.
Do not modify this. It was created this way so it will be automatically adjusted.
For now we're using msbuild to build for KM. The easiest way for this is to have a vcxproj with all settings.
This can be changed to fit your needs. Feel free to use it as you want :) 


To run, just open a visual studio developer command prompt and run the config that you want:
```
   msbuild .\xpf_lib_winkm.vcxproj /p:configuration=WinKmDebug /p:platform=x64 /t:Clean;Rebuild
   msbuild .\xpf_lib_winkm.vcxproj /p:configuration=WinKmDebug /p:platform=Win32 /t:Clean;Rebuild
 
   msbuild .\xpf_lib_winkm.vcxproj /p:configuration=WinKmRelease /p:platform=x64 /t:Clean;Rebuild
   msbuild .\xpf_lib_winkm.vcxproj /p:configuration=WinKmRelease /p:platform=Win32 /t:Clean;Rebuild
```