# Debugging Crashes

This document covers the following topics:
1. [How to get a crash dump](debugging_crashes.md#How-to-get-a-crash-dump)
2. [How to get a good callstack for crashes](debugging_crashes.md#How-to-get-a-good-callstack-for-crashes)

## How to get a crash dump

### Using Visual Studio

If the crash is in your app which you can launch from Visual Studio, or if you can attach to the app with Visual Studio prior to the crash, then you can use these steps:
1. If launching the app from Visual Studio, set Visual Studio to debug either as "Native Only" or as "Mixed (Managed and Native)" before launching the app.
If attaching to a running app, in the "Attach to Process" dialog be sure to change the "Attach to:" settings to include "Native" if it isn't shown for the process, and then attach.
2. Run the repro.
3. When the crash occurs and Visual Studio breaks in, select "Save Dump As..." from the Debug menu to save the dump.

Note: In some cases, Visual Studio's Diagnostic Tools may cause a crash which only occurs when debugging in Visual Studio with the Diagnostic Tools enabled.
This type of crash might manifest as a crash in your app, usually with a function name containing `Diagnostics` somewhere on the stack, or it may include a crash in a separate ScriptedSandbox64.exe process. If this situation appears to be happening, try turning off Tools | Options | Debugging | "Enable Diagnostic Tools while debugging" in Visual Studio settings and then run your scenario.

### Using WinDbg

In [WinDbg](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/debugger-download-tools), use `.dump /ma <filename>` to save a full memory dump to the specified file when the crash happens.
* If the crash happens sometime after app launch, then you can launch the app, attach to the app with WinDbg, and then do the repro steps to hit the crash.
* If the crash happens on launch, then [WinDbg Preview](https://www.microsoft.com/store/p/windbg/9pgjgd53tn86) is recommended, since it contains a "Launch app package" option in
"Start Debugging" which can launch and debug UWP and packaged apps from launch.

### Other options

Another option is to set some regkeys to tell Windows to keep some crash dumps locally (see https://docs.microsoft.com/en-us/windows/win32/wer/collecting-user-mode-dumps).
For example, running these commands in an administrative command prompt will set regkeys to tell Windows to save full (type=2) dumps into `C:\dumps`:

    reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps" /v DumpFolder /d "C:\dumps"
    reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps" /v DumpType /t REG_DWORD /d 2

After you repro the crash, you should find a dump in the specified folder.

You can automate crash dump collection and filing bugs by following these steps:
1. Download the script at https://aka.ms/RNW/analyze-crash.ps1, for example to `C:\temp`
2. Open an admin PowerShell. If you haven't enabled running unsigned scripts yet, do that by running: `Set-ExecutionPolicy Unrestricted`
3. Run the script and pass it the name of your app's exe: `C:\temp\analyze-crash.ps1 -ExeName MyApp -Repo microsoft/microsoft-ui-xaml`
The script will set up automatic crash dump collection for your app, download the native debugging tools (including the command-line debugger cdb), and ask you to reproduce the crash.

At this point you can launch the app (e.g. from Start menu if you've already deployed it to the local device). When the app crashes, it will generate a crash dump (`.dmp` file). You can then press enter to resume execution of the script, and the script will use `cdb` to automatically analyze the crash dump, and output the results to a file `analyze.log`.

The script will then copy the contents of the log to the clipboard, open the log file in notepad, and launch the browser to file an issue in the WinUI repo, where you can paste the stack trace into the bug template.

## How to get a good callstack for crashes

For some crashes, such as access violations, the direct crash callstack is usually the right stack to use.
But if it is a stowed exception crash, which has exception code: 0xc000027b, then more work is required to get the right callstack.

### Stowed Exception crashes (exception code: 0xc000027b)

Stowed exception crashes save away a possible error, which gets used later if no one handles the exception.
XAML sometimes decides the error is fatal immediately, in which case the direct crash stack may be good, but more frequently the stack has unwound before it was determined to be fatal.
This channel9 talk describes stowed exceptions in more detail: https://channel9.msdn.com/Shows/Inside/C000027B?term=stowed%20exception&lang-en=true.

For stowed exception crashes, you can get more info on the crash by loading a crash dump in WinDbg and then using !pde.dse to dump the stowed exceptions.
The !pde debugger extension is available [here](https://onedrive.live.com/?authkey=%21AJeSzeiu8SQ7T4w&id=DAE128BD454CF957%217152&cid=DAE128BD454CF957) in the PDE*.zip.
(It is linked off the channel9 page above.)
Put the appropiate x64 or x86 dll in that zip in the winext directory of a WinDbg install, and then "!pde.dse" should work on stowed exception crash dumps.

Frequently there will be multiple stowed exceptions, with some at the end which got handled/ignored.
Most commonly, the first stowed exception is the interesting one.
In some cases, the first stowed exception may be a re-throw of the second, so if the second stowed exception shows deeper into the same stack as the first, then the second exception may be the origination of the error.
The error code shown with each stowed exception is also valuable, since that provides the HRESULT associated with that exception.
