// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI;
using Microsoft.Windows.Storage.Pickers;

namespace WinUIProfiler.App.Services;

// Picks a recorded trace. Listing its processes then shows whether it can be read.
internal static class TracePicker
{
    public static async Task<string?> PickAsync(WindowId owner)
    {
        if (owner.Value == 0)
        {
            throw new ArgumentException("The trace picker requires its owning window.", nameof(owner));
        }

        var picker = new FileOpenPicker(owner);
        picker.FileTypeFilter.Add(".etl");
        var selected = await picker.PickSingleFileAsync();
        return selected?.Path;
    }
}
