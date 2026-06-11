// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    [Flags]
    public enum BindStatus
    {
        None = 0x0,
        HasBinding = 0x01,
        TracksSource = 0x02,
        TracksTarget = 0x04,
        HasFallbackValue = 0x08,
        HasTargetNullValue = 0x10,
        HasConverter = 0x20,
        HasEventBinding = 0x40,
    }
}