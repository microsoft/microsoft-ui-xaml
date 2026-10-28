// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

public enum XamlProvider
{
    Xaml,
    XamlProfiler,
    // The kernel's GPU memory usage and budget for the process (Microsoft-Windows-DxgKrnl).
    DxgKrnl
}

public static class XamlProviders
{
    public static readonly Guid Xaml = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");
    public static readonly Guid XamlProfiler = new("a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d");
    public static readonly Guid DxgKrnl = new("802ec45a-1e99-4b83-9920-87c98277ba9d");

    // The two XAML providers. DxgKrnl events are matched by ID in XamlEventSubscription.
    public static bool TryGet(Guid providerId, out XamlProvider provider)
    {
        if (providerId == Xaml)
        {
            provider = XamlProvider.Xaml;
            return true;
        }

        if (providerId == XamlProfiler)
        {
            provider = XamlProvider.XamlProfiler;
            return true;
        }

        provider = default;
        return false;
    }
}