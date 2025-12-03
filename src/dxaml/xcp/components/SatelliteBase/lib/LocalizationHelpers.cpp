// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <LocalizationHelpers.h>
#include <NamespaceAliases.h>
#include <SatMacros.h>
#include <RoErrorApi.h>
#include <muiload.h>
#include "MuiUdk.h"
#include "LoadLibraryAbs.h"

HINSTANCE g_hStringResource = nullptr;

namespace Private
{
    extern const wchar_t * c_moduleName;

    _Check_return_ HRESULT FindStringResource(
        _In_ WORD resourceId,
        _Out_ HSTRING* value)
    {
        LANGID langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        LPCWSTR block = MAKEINTRESOURCE(resourceId / 16 + 1);
        HRSRC resourceLocation = nullptr;
        HGLOBAL resourceHandle = nullptr;
        LPCWSTR resource = nullptr;
        wrl_wrappers::HString outStr;

        if (!g_hStringResource)
        {
            IFC_RETURN(Mui_PropagateApplicationLanguages());

            // LoadLibrary will get a handle to the MUX extension with an association to the appropriate
            // MUI based on the above preferences.
            g_hStringResource = LoadLibraryExWAbs(c_moduleName, nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
            IFCW32_RETURN(g_hStringResource);
        }

        resourceLocation = FindResourceEx(g_hStringResource, RT_STRING, block, langId);
        if (!resourceLocation)
        {
            // fallback to english
            langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            resourceLocation = FindResourceEx(g_hStringResource, RT_STRING, block, langId);
        }
        IFCW32_RETURN(resourceLocation);
        IFCW32_RETURN(resourceHandle = LoadResource(g_hStringResource, resourceLocation));
        IFCW32SLE_RETURN(resource = reinterpret_cast<LPCWSTR>(LockResource(resourceHandle)));

        for (INT i = 0; i < (resourceId & 15); i++)
        {
            resource += 1 + static_cast<UINT>(*resource);
        }

        IFC_RETURN(outStr.Set(resource + 1, static_cast<UINT>(*resource)));
        *value = outStr.Detach();

        return S_OK;
    }

    void XamlTestHookFreeResourceLibrary()
    {
        if (g_hStringResource)
        {
            FreeMUILibrary(g_hStringResource);
            g_hStringResource = nullptr;
        }
    }
}