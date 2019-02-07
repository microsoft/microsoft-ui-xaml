# Source code structure

#### /build, /tools

These folders contain scripts and other support machinery that you shouldn't 
need to edit for most changes.

In particular:

* **/build/NuSpecs** enables .nupkg generation
* **/build/FrameworkPackage** enables .appx generation

Note that here and in various parts of the codebase you will see references to 
`BUILD_WINDOWS`. WinUI operates as a standalone package for Xaml apps but is 
also a way that new controls migrate into [Windows.UI.Xaml.Controls](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls) 
as part of the Windows build system. The places where the WinUI source needs to 
differ for this different environment are specified under `BUILD_WINDOWS`. It's 
expected that it is the responsibility of the Microsoft team members to 
maintain this part of WinUI, and other community members should be able to 
ignore it.

#### /dev

Under dev is a separate folder for each of our controls.

Each control is composed of a Shared Item using the Shared Item Template in 
Visual Studio. It is then included into the respective projects. This gives us 
flexibility in the future if we need to decompose our other projects into 
smaller projects, move to a Git submodule model on a per control basis, or 
create different DLLs of our solution. Currently the project is small enough 
that the Shared Item Template gives us enough flexibility to add/remove 
controls to/from the subsequent projects easily.

Also under dev is the actual Microsoft.UI.Xaml project, which is the main DLL 
that contains all the controls and other solutions which will be packaged and 
deployed. At this time we believe the Microsoft.UI.Xaml.dll is 
small enough to include all controls into one DLL. As we increase the number of 
controls we will revisit this decision and may decompose it into different DLLs 
in the future. Also we will adjust based on developer feedback if we start to see 
usage patterns where teams use just a few controls vs. the whole library.

This project also includes the necessary definitions to package the DLL into a 
NuGet package.

#### /docs

This is where the repo documentation lives, including this document.

Note that developer usage documentation can be found separately on docs.microsoft.com.

#### /test

Our test library and test app (the app that the test library interacts with 
when executing the tests) are here.

MUXControls.Test is a [MSTest](https://docs.microsoft.com/dotnet/api/microsoft.visualstudio.testtools.unittesting) 
and [TAEF](https://docs.microsoft.com/en-us/windows-hardware/drivers/taef/) DLL that contains all 
of the test code for the various controls by automating the MUXControlsTestApp.

MUXControlsTestApp is a UWP app that exercises all the controls. This is just a 
manual testing playground which can be driven by the automated tests for 
automated verification as well as [TestMethod] control API verification. Note 
this applications references the MUXControls DLL rather than including the 
Shared Items.
