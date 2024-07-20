// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "XamlOM.WinUI.Private.h"
#include <functional>

// {28cb4df8-85eb-46ee-8d71-c614c2305f74}
DEFINE_GUID(CLSID_XamlDiagnosticsLauncher,
    0x28cb4df8, 0x85eb, 0x46ee, 0x8d, 0x71, 0xc6, 0x14, 0xc2, 0x30, 0x5f, 0x74);
interface IXamlDiagnosticsTap;
using XamlDiagnosticsLoadedCallback = std::function<void(IXamlDiagnosticsTap*)>;
interface IXamlDiagnosticsLauncher;

interface __declspec(uuid("{a52f9e5f-3633-4ce7-a3ad-5eea9aaee0dd}"))
    IXamlDiagnosticsLauncherFactory
    : public IClassFactory
{
    STDMETHOD(GetLauncher)(
        _COM_Outptr_ IXamlDiagnosticsLauncher** launcher
        ) = 0;
};

interface __declspec(uuid("{334830db-a376-4c80-84ec-064006f85110}"))
    IXamlDiagnosticsLauncher
    : public IUnknown
{
    STDMETHOD(ConnectToVisualTree)(
        _In_z_ PCWSTR xamlDiagDll,
        _In_ XamlDiagnosticsLoadedCallback connectionCallback
        ) = 0;
};
