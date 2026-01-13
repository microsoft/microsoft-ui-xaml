# Building & Running

## Build Controls in WinUI 3.0
1. Follow the instructions to [clone the WinUI repo](../../../docs/developer-guide.md#clone-the-winui-repo).
2. Initialize your environment
    * Running `init` with no additional parameters should be fine for WebView2 purposes.
3. Build entire WinUI repo
    * Only needed the first time you clone or sync to a new commit
    * Run `build` at repo root
4. Build Controls (if not building entire WinUI repo)
     * `msbuild <repo>\controls\MUXControls.sln`

Other build options:
* Rebuild MUXC without using incremental build:
    * `msbuild <repo>\controls\MUXControls.sln /t:Rebuild`
* For additional scoped builds, see the options in [controls\build.cmd](../../build.cmd).

Additional notes:
* Multi-proc builds (default for `msb.cmd`) are not yet supported for MUXC. 
Therefore, use `msbuild.exe` (defaults to single-proc) as the build command (or `controls\build.cmd`). 
* If running in VS and getting an error asking to attach another debugger, make sure you are running with "mixed" 
debugging.

## Run WebView2 sample (MUXControlsTestApp)
1. Using the instructions in the [Testing FAQ](../../../docs/testing-faq.md), run `testmachine-prerun.cmd` if it has not
already been run.
2. Navigate to `<repo>\BuildOutput\bin\x86chk\Test` and run `MuxControlsTestApp.appx` to install.  
Run using the option in the install dialog, or as you would any other installed application.
    * Alternatively, run a test to install (a WebView2 test will ensure you have the correct Anaheim installed as well)
3. After app launches, click *WebView2* button, then choose a test page (usually *WebView2 Basic Tests*)
    * Invoke the *Load URI* button to navigate to Bing
    * Not providing fully qualified URI (like "www.bing.com" instead of "https://www.bing.com") will crash the app (for
    now).

### Run WebView2 sample in Visual Studio
Not working at this time  
~~1. In Visual Studio, set *Startup Project* dropdown menu to *'MUXControlsTestApp~~  
~~(test\MUXControlsTestApp\MUXControlsTestApp) (Universal Windows)'*~~  
~~2. Run on *Local Machine* with F5 or Control+F5~~  
    ~~F5 sometimes doesn't work due to missing environment variables. To work around this, temporarily change the~~  
    ~~incorrect paths in the error you see to hard-coded real paths.~~  

