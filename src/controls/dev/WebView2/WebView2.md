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
~~(test\MUXControlsTestApp\MUXControlsTestApp) ‎(Universal Windows)'*~~  
~~2. Run on *Local Machine* with F5 or Control+F5~~  
    ~~F5 sometimes doesn't work due to missing environment variables. To work around this, temporarily change the~~  
    ~~incorrect paths in the error you see to hard-coded real paths.~~  


# Building Anaheim

These instructions are only necessary to follow if you need to make changes to Anaheim Edge code. Otherwise, the public
Edge Beta can be used with WebView2.

## Build Anaheim/Installer
1. Enlist/sync per 
[Anaheim Enlistment Instructions](https://microsoft.visualstudio.com/Edge/_wiki/wikis/Edge.wiki/150/Windows-Instructions)
2. Build Anaheim + Installer:
    * Follow the [instructions here](https://microsoft.visualstudio.com/Edge/_wiki/wikis/Edge.wiki/64/Windows)
    * Use x86 or x64 release flavor (debug flavor takes untractably long to generate PDBs). 
        * The rest of the notes here assume x86 for consistency.
        * x64 Anaheim generally works with either x64 or x86 webview client (however embeddedbrowserwebview.dll arch 
        needs to match that of Xaml App)
    * A good set of modules to build are Chrome, WebView2, and the installer:
    `autoninja chrome embedded_browser_webview installer mini_installer`
    * However you can also build these separately
        * Build Chrome: `autoninja chrome`
        * Build Anaheim WebView: `autoninja embedded_browser_webview`
        * Build installer: `autoninja installer mini_installer`
3. Install the built Microsoft Edge WebView2 Runtime: `mini_installer.exe --msedgewebview --system-level`
    * If you already have Microsoft Edge Beta installed, and just want to update EBWebView binaries:
        * (Note: `<EdgeInstallDir>` is eg `C:\Program Files (x86)\Microsoft\EdgeWebView\Application\86.0.574.0`)
        * Replace `embeddedbrowserwebview.dll` (loaded in app)
            * `copy <AnaheimRepo>\src\out\release_x86\EBWebView\x86\EmbeddedBrowserWebView.dll
            <EdgeInstallDir>\EBWebView\x86\embeddedbrowserwebview.dll"`
        * Replace `msedge.dll` (loaded in browser)
            * `copy <AnaheimRepo>\src\out\release_x86\msedge.dll  <EdgeInstallDir>\msedge.dll"`

## Update Dependencies / Ensure Environment
1. If there are changes to WebView2.idl or WebView2_Staging.idl in Anaheim, the corresponding headers need to be copied
to Xaml WebView2 project as follows:
    * `copy <AnaheimRepo>\src\third_party\win_build_output\midl\edge_embedded_browser\client\win\current\idl_public\<arch>\webview2.h 
       <WebView2Repo>\microsoft-ui-xaml-lift\webview2\microsoft-ui-xaml\dev\WebView2\webview2_core.h`
    * `copy <AnaheimRepo>\src\third_party\win_build_output\midl\edge_embedded_browser\client\win\current\idl_staging\<arch>\webview2staging.h 
       <WebView2Repo>\microsoft-ui-xaml-lift\webview2\microsoft-ui-xaml\dev\WebView2\webview2_core_staging.h`