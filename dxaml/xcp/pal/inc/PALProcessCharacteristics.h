// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//-----------------------------------------------------------------------------
//  Abstract:
//      Defines IPALProcessCharacteristics, a PAL interface for obtaining
//      information about a process.
//
// Exposes information about a process through the PAL.
//
// Currently IPlatformServices derives from IPALProcessCharacteristics, so
// the PAL describes the current process. If a scenario arises where we
// need to examine other processes, the PAL could hand out an instance of
// IPALProcessCharacteristics to describe a different process.
//-----------------------------------------------------------------------------

struct IPALProcessCharacteristics
{
    virtual bool IsProcessPackaged() = 0;
    virtual bool IsProcessAppContainer() = 0;
    virtual _Check_return_ HRESULT GetProcessPackagePath(_Out_ xstring_ptr* pstrPackagePath) = 0;
    virtual _Check_return_ HRESULT GetProcessPackageFullName(_Out_ xstring_ptr* pstrPackageFullName) = 0;
    virtual _Check_return_ HRESULT GetProcessModernAppId(_Out_ xstring_ptr* pstrAppId) = 0;
    virtual _Check_return_ HRESULT GetProcessImageName(_Out_ xstring_ptr* pstrImageName) = 0;
};


