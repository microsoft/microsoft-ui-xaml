# Exception Context Lab

A packaged C# diagnostic sample for [issue #11253](https://github.com/microsoft/microsoft-ui-xaml/issues/11253).
It throws a real exception from an `async void` button handler after an `await`.
No catch surrounds the throw. `Application.UnhandledException` logs the event; only the explicit
**Set e.Handled = true** control marks the intentional test exception handled.

The regression still calls the handler. With the pre-fix runtime, loading an Expander leaves
stale native error context that can cause XAML to fail-fast after the handler returns.

## Build and deploy

From an initialized repository root:

```powershell
.\initrun.ps1 .\build.cmd /q product
.\initrun.ps1 msb /q "Samples\ExceptionContextLab\ExceptionContextLab.csproj" /restore /p:Platform=x64 /p:RuntimeIdentifier=win-x64
.\Samples\ExceptionContextLab\Export-Layout.ps1 -OutputDirectory C:\Temp\ExceptionContextLab-AppX
```

Choose a new or empty directory. Register its `AppxManifest.xml` on a development-mode machine
or deploy the layout to a test VM. It includes .NET, the complete local native runtime cohort,
and generated activation registrations.

Build and preserve MUX at the genuine pre-fix and fixed commits. Export the same sample twice
with `-Variant Before` or `-Variant After`, `-RuntimeDll <preserved-dll>`, and an accurate
`-RuntimeLabel "BEFORE - pre-fix WinUI"` or `-RuntimeLabel "AFTER - fixed WinUI"`.
Only substitute compatible runtimes from the same source baseline and build flavor.
The export records SHA-256 hashes. The variants have separate package identities and appear in
Start as **Issue 11253 - Exception Context Lab - BEFORE** and **... - AFTER**.
Close the app before replacing its layout. If its registered installation path changes, uninstall
that sample variant before registering the new location.

For ARM64, use `-Flavor arm64chk`, `/p:Platform=ARM64`, and `/p:RuntimeIdentifier=win-arm64`
for the builds and `-Flavor arm64chk` for export.

## Test each case in a fresh process

1. Close and reopen the selected variant.
2. Leave **Set e.Handled = true** unchecked.
3. Optionally select **Load and expand Expander**. Wait for the case text to say **loaded**.
4. Select **Throw async void exception**.
5. If the app survives, check its exception count and heartbeat, then select **Check UI response**.
6. Repeat in fresh processes for all cases below, including the handled control.

| Case | Pre-fix runtime | Fixed runtime |
|---|---|---|
| No Expander, Handled left false | Survives in the observed desktop C# path | Same |
| Expander loaded, Handled left false | Terminates after the handler runs | Survives |
| Expander loaded, Handled set true | Survives | Same |

An Expander is created only when requested. Unloading it would not reset the native class
constructor or stale thread error state, so closing and reopening is essential.
Each log line includes the process ID. `sessions.log` is appended in the package's LocalState
directory, including the handler call before it returns and a later `UI_ALIVE` marker if the
dispatcher continues. Logging a handler call alone does not prove process survival.

This fix removes XAML's stale-context fail-fast. It does not make unhandled exceptions safe in
general or promise process survival for other exception paths or runtime versions.

## Validation

The x64 Debug sample built successfully and passed all six fresh-process cases above on a
Windows 11 test VM, with 60 explicit UI Automation assertions. Each process loaded the expected
runtime hash and identical sample binary. The pre-fix Expander/unhandled case exited with
`0xC000027B`; the five surviving cases recorded a later dispatcher tick and responded to the
UI check button. The before/after recording shows actual process termination and survival.

The build reported one CS8305 warning in generated code for the experimental tabular metadata
provider. ARM64, Release, and Native AOT were not validated.
