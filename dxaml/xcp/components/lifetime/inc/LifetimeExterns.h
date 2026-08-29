// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

extern IPALDebuggingServices * __stdcall GetPALDebuggingServices();

namespace DirectUI 
{
    struct IDXamlCore;
    struct IPeerTableHost;
    class DependencyObject;

    namespace DXamlServices
    {
        extern __maybenull IDXamlCore* GetDXamlCore();
        extern __maybenull IPeerTableHost* GetPeerTableHost();
        extern bool IsDXamlCoreInitializing();
        extern bool IsDXamlCoreInitialized();
        extern bool IsDXamlCoreShutdown();
    }
}
