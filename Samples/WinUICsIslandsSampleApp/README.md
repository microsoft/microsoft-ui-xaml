# WinUI C# Islands shutdown and restart sample

This sample uses a WinForms window as a process-lifetime host for WinUI content. Unlike a
normal C# WinUI application, the native host remains alive after the WinUI DispatcherQueue
shuts down, allowing WinUI to be initialized again in the same process.

The hosted content is a compiled C# XAML `UserControl` containing a MUXC `NumberBox` and a
Windows Community Toolkit `SettingsCard`. This exercises managed XAML metadata, MUXC
dependency-property caches, and Toolkit control state across WinUI generations.

Use the buttons to:

1. **Start WinUI** - create a `DispatcherQueueController`, `WindowsXamlManager`,
   `DesktopWindowXamlSource`, managed `Application`, and hosted XAML content.
2. **Shut down WinUI** - release the hosted content and manager, then shut down the
   DispatcherQueue.
3. **Restart WinUI** - perform a complete shutdown followed by another initialization.

The event log records `XamlShutdownCompletedOnThread`,
`WinUIProcessShutdownStarting`, `WinUIProcessShutdownCompleted`, and the DispatcherQueue
shutdown events. It also records whether the process-event sender and arguments are null.

Open `WinUICsIslandsSampleApp.sln`, select an architecture such as x64, and run the
`WinUICsIslandsSampleApp` project.
