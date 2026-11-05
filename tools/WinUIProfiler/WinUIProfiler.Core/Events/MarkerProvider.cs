// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

// A provider named in the marker configuration (Launch.MarkerConfig), such as the one an app writes its own
// "ReadyToInteract" event with. The session file stores its name and ID with each of its event types.
public sealed record MarkerProvider(string Name, Guid Id);

// How a marker provider is enabled: its level (1, critical, to 5, verbose) and keywords. As for the XAML providers, only
// the target process's events are read.
public sealed record MarkerProviderSettings(MarkerProvider Provider, byte Level = 5, ulong Keywords = ulong.MaxValue);
