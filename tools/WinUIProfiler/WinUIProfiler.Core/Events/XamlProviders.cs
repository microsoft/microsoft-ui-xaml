// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Events;

public enum XamlProvider
{
    Xaml,
    XamlProfiler
}

public static class XamlProviders
{
    public static readonly Guid Xaml = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");
    public static readonly Guid XamlProfiler = new("a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d");

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