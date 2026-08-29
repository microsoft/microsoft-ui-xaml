// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using Windows.Foundation.Metadata;

namespace Private.Infrastructure.Hosting.WinForms
{
    // {766BB98B-6143-4BFD-8845-161CE1549157}
    [Guid(0x766bb98b, 0x6143, 0x4bfd, 0x88, 0x45, 0x16, 0x1c, 0xe1, 0x54, 0x91, 0x57)]
#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    public interface IWinFormsHost
    {
    }
}

