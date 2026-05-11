# Controls Developer Guide

This guide provides instructions on how to build, run and debug the controls product and test code. This is an addendum to the [developer-guide](./developer-guide.md). Any common issues/gotchas are called out where possible.

Once you have your environment and build setup per the [developer-guide](./developer-guide.md), you can follow these steps to work in the controls code base.

1. From the same command prompt where you ran init.cmd and build.cmd, navigate to controls folder and launch muxcontrols.sln file. This will open the controls solution which includes the WinUI controls project and tests in visual studio with the appropriate environment variables setup.

2. Switch the build configuration in visual studio to match how you ran init.cmd. By default, init.cmd uses x86 chk, so choose x86 and debug configuration in visual studio.

3. Set MuxControlsTestApp project as the startup project.

4. You can hit F5 to debug or ctrl-F5 to run without the debugger attached.

5. Right click muxcontrolstestapp project, choose properties. This will open the project properties for the test app. Select Debug->General and "Open debug launch profiles UI" Scroll down and check "Enable debugging for managed and native code together, also known as mixed-mode debugging". This will enable debugging product code (native) and test code (C#).

6. In the search (Ctrl+Q) box on top type "exception settings" and open Debug->Windows-Exception Settings. Check "C++ Exceptions", "Common Language Runtime Exceptions" and "Win32 Exceptions". This will ensure that first chance exceptions are caught on the debugger when you debug the app.


## Known issues.

1.You see this build error when building muxcontrols.sln

### Error
Microsoft.NET.TargetFrameworkInference.targets(92,5): error NETSDK1013: The TargetFramework value '-windows10.0.18362.0' was not recognized. It may be misspelled. If not, then the TargetFrameworkIdentifier and/or TargetFrameworkVersion properties must be specified explicitly.
8>Done building project "MUXControlsTestApp.csproj" -- FAILED.

### Try these fixes
a. Ensure that you launched muxcontrols.sln file after running init.cmd which sets up the appropriate environment variables.

b. Ensure that you have visual studio setup correctly using the .vsconfig file. See [developer-guide](./developer-guide.md) for instructions

c. run 'git clean -dfx' to clean the repo from any stale files and start from the beginning again.

