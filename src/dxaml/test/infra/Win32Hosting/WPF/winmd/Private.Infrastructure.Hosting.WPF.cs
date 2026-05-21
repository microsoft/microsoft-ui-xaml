// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using Windows.Foundation.Metadata;

namespace Private.Infrastructure.Hosting.WPF
{
#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    [Guid(0x34a62d54, 0x177d, 0x4b1f, 0x84, 0xeb, 0x20, 0xf8, 0x9f, 0x2d, 0xb8, 0xed)]
    public interface IWPFHost
    {
    }
}

