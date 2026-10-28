// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

// In display order. Session files store the name, not the value.
public enum XamlProvider
{
    // Microsoft-Windows-XAML.
    Xaml,
    // Microsoft-Windows-XAML-Diagnostics.
    XamlDiagnostics,
    // Microsoft-Windows-XAML-Profiler.
    XamlProfiler,
    // Microsoft.UI.Xaml, the runtime's telemetry events.
    MicrosoftUIXaml,
    // Microsoft.UI.Xaml.Controls, the controls' telemetry events.
    MicrosoftUIXamlControls,
    // The kernel's GPU memory usage and budget for the process (Microsoft-Windows-DxgKrnl).
    DxgKrnl
}

public static class XamlProviders
{
    public static readonly Guid Xaml = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");
    public static readonly Guid XamlDiagnostics = new("59e7a714-73a4-4147-b47e-0957048c75c4");
    public static readonly Guid XamlProfiler = new("a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d");
    public static readonly Guid MicrosoftUIXaml = new("2dc72f6e-e4d1-5f58-3245-09a4243799dd");
    public static readonly Guid MicrosoftUIXamlControls = new("21e0ae07-56a7-55b5-12f9-011e6bc08cca");
    public static readonly Guid DxgKrnl = new("802ec45a-1e99-4b83-9920-87c98277ba9d");

    // The XAML providers, whose events are all read. DxgKrnl events are matched by ID in XamlEventSubscription.
    private static readonly Dictionary<Guid, XamlProvider> XamlProviderIds = new()
    {
        [Xaml] = XamlProvider.Xaml,
        [XamlDiagnostics] = XamlProvider.XamlDiagnostics,
        [XamlProfiler] = XamlProvider.XamlProfiler,
        [MicrosoftUIXaml] = XamlProvider.MicrosoftUIXaml,
        [MicrosoftUIXamlControls] = XamlProvider.MicrosoftUIXamlControls
    };

    public static IEnumerable<Guid> XamlIds => XamlProviderIds.Keys;

    public static bool TryGet(Guid providerId, out XamlProvider provider) =>
        XamlProviderIds.TryGetValue(providerId, out provider);
}