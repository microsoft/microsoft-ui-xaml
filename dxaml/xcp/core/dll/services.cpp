// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

IPALDebuggingServices *GetPALDebuggingServices()
{
    return static_cast<IPALDebuggingServices *>(gps.Get());
}

IPALMemoryServices *GetPALMemoryServices()
{
    return static_cast<IPALMemoryServices *>(gps.Get());
}

IPALURIServices *GetPALURIServices()
{
    return static_cast<IPALURIServices *>(gps.Get());
}

IPALMathServices *GetPALMathServices()
{
    return static_cast<IPALMathServices *>(gps.Get());
}

IPALPrintIOServices *GetPALPrintIOServices()
{
    return static_cast<IPALPrintIOServices *>(gps.Get());
}

IPALThreadingServices *GetPALThreadingServices()
{
    return static_cast<IPALThreadingServices *>(gps.Get());
}

IPALTextServices *GetPALTextServices()
{
    return static_cast<IPALTextServices *>(gps.Get());
}

IPALCoreServices *GetPALCoreServices()
{
    return static_cast<IPALCoreServices *>(gps.Get());
}