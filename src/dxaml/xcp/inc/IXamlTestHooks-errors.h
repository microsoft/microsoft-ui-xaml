// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Publicly documented at https://docs.microsoft.com/en-us/windows/win32/winrt/roerrorapi/nf-roerrorapi-rofailfastwitherrorcontextinternal2
void WINAPI RoFailFastWithErrorContextInternal2(
    HRESULT hrError,
    ULONG cStowedExceptions,
    _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[]);

// IXamlErrorTestHooks
DECLARE_INTERFACE_IID_(IXamlErrorTestHooks, IInspectable, "B4F0A992-3DE9-403A-B0DD-E06F3ACBB3DA")
{
    IFACEMETHOD_(void, SetRoFailFastMock)(std::function<decltype(RoFailFastWithErrorContextInternal2)> mock) = 0;
    IFACEMETHOD_(void, ClearStowedExceptions)() = 0;
    
};

HRESULT CreateErrorHandlingTestHooks(_Outptr_opt_ IXamlErrorTestHooks** errorTestHooks);
