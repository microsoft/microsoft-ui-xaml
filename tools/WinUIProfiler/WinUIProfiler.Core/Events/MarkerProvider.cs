// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

// A provider named in the marker configuration (Launch.MarkerConfig), such as the one an app writes its own
// "ReadyToInteract" event with. The session file stores its name and ID with each of its event types.
public sealed record MarkerProvider(string Name, Guid Id);

// How a marker provider is enabled: its level (1, critical, to 5, verbose) and keywords. As for the XAML providers, the
// target process's events are read. Launcher is set when a launcher milestone names the provider: then the events of the
// target's launcher, the process that started it, are read too, from LauncherWindow before the target's creation to
// LauncherWindow after it.
public sealed record MarkerProviderSettings(MarkerProvider Provider, byte Level = 5, ulong Keywords = ulong.MaxValue,
    bool Launcher = false)
{
    public static TimeSpan LauncherWindow { get; } = TimeSpan.FromSeconds(10);
}
