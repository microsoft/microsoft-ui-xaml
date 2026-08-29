// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"
#include "XamlBehaviorMode.h"
#include "RawData.h"
#include <muiload.h>

#include <Richedit.h>
#include "textserv.h"
#include "WindowLessSiteHost.h"
#include "LoadLibraryAbs.h"

void CXcpBrowserHost::UnloadSatelliteDll(_In_ HINSTANCE dll)
{
    FreeMUILibrary(dll);
}

//------------------------------------------------------------------------
//
//  Method: LoadResourceSatelliteDlls
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT CXcpBrowserHost::LoadResourceSatelliteDlls()
{
    HRESULT hr = S_OK;
    IPALResourceManager* pResourceManager = NULL;

    bool isWoW64 = false;
    PVOID oldValue = NULL;

    if (m_hNonLocalizedResource)
    {
        goto Cleanup;
    }

    IFC(m_pcs->GetResourceManager(&pResourceManager));
    IFC(pResourceManager->SetProcessMUILanguages());

    // Locate the WinAppSDK "Microsoft.UI.Xaml.dll" currently executing and load its MUI resources.
    // We can't blindly load a "Microsoft.UI.Xaml.dll" since it may resolve to the WinUI 2 Microsoft.UI.Xaml.dll
    // if both WinUI 2 and WinAppSDK are being used in the same process.

    WCHAR fullModulePath[MAX_PATH];

    // Get the handle to the current WinAppSDK module using an arbitrary static method, CXcpBrowserHost::Create
    HMODULE thisModule;
    IFCW32(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&CXcpBrowserHost::Create, &thisModule));

    // Get the full path of the current WinAppSDK module
    IFCW32(GetModuleFileName(thisModule, fullModulePath, sizeof(fullModulePath)));

    // 'LangId = 0' means that the OS will use its own fallback logic to locate the suitable
    // resource file.
    m_hLocalizedResource = LoadMUILibrary(fullModulePath, MUI_LANGUAGE_NAME, 0);
    if (!m_hLocalizedResource)
    {
        IFC(HRESULT_FROM_WIN32(GetLastError()));
    }

    USHORT Machine;
    IFCW32(IsWow64Process2(GetCurrentProcess(), &Machine, NULL));
    isWoW64 = (Machine != IMAGE_FILE_MACHINE_UNKNOWN);
    if (isWoW64)
    {
        // Since our resource DLL is architecture neutral, we only keep a copy of it in the Windows\System32 directory
        // We need to tell WOW redirect not to redirect into the WOW64 folder
        IFCW32(Wow64DisableWow64FsRedirection(&oldValue));
    }
    m_hNonLocalizedResource = LoadLibraryExWAbs(L"Microsoft.UI.Xaml.resources.common.dll", NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!m_hNonLocalizedResource)
    {
        IFC(HRESULT_FROM_WIN32(GetLastError()));
    }

Cleanup:
    if (isWoW64)
    {
        Wow64RevertWow64FsRedirection(oldValue);
    }

    ReleaseInterface(pResourceManager);
    
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::GetLocalizedResourceString
//
//  Synopsis:
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CXcpBrowserHost::GetLocalizedResourceString(_In_ XUINT32 stringId, _Out_ xstring_ptr* resourceString)
{
    IFC_RETURN(LoadResourceSatelliteDlls());
    IFC_RETURN(GetResourceString(m_hLocalizedResource, stringId, resourceString));
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::GetLocalizedResourceString
//
//  Synopsis:
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CXcpBrowserHost::GetNonLocalizedResourceString(_In_ XUINT32 stringId, _Out_ xstring_ptr* resourceString)
{
    IFC_RETURN(LoadResourceSatelliteDlls());
    IFC_RETURN(GetResourceString(m_hNonLocalizedResource, stringId, resourceString));
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::GetLocalizedResourceString
//
//  Synopsis:
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CXcpBrowserHost::GetResourceString(HINSTANCE hResource, _In_ XUINT32 stringId, _Out_ xstring_ptr* resourceString)
{
    HRESULT hr = S_OK;

    bool foundString = false;
    XStringBuilder strBuilder;

    IFC(strBuilder.Initialize());
        
    //Load localized string from mui:
    IFC_NOTRACE(LoadStringResource(hResource, stringId, strBuilder, &foundString));

    ASSERT(foundString);

    IFC(strBuilder.DetachString(resourceString));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CXcpBrowserHost::GetNonLocalizedErrorString
//
//  Synopsis:
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CXcpBrowserHost::GetNonLocalizedErrorString(
    _In_ XUINT32 stringId,
    _Out_ xstring_ptr* errorString
    )
{
    HRESULT hr = S_OK;
    WCHAR* wszErrorCode = NULL;
    XUINT32 cchErrorCode = 0;
    bool foundString = false;
    XStringBuilder strBuilder;

    IFC(LoadResourceSatelliteDlls());

    IFC(strBuilder.Initialize());

    //Load non-localized string from MUX.resources.common.dll
    IFC_NOTRACE(LoadStringResource(m_hNonLocalizedResource, stringId, strBuilder, &foundString));    

    if (!foundString)
    {
        wszErrorCode = xstritoa(stringId, &cchErrorCode);

        IFC(strBuilder.Append(wszErrorCode, cchErrorCode));
        IFC(strBuilder.Append(L" ", 1));
        IFC(strBuilder.Append(STR_LEN_PAIR(L"An error has occurred.")));
    }

    IFC(strBuilder.DetachString(errorString));

Cleanup:
    delete[] wszErrorCode;

    RRETURN(hr);
}

_Check_return_ HRESULT CXcpBrowserHost::GetResourceData(
    _In_ uint32_t resourceId,
    _In_ const xstring_ptr_view& resourceType,
    _Out_ RawData* resourceData)
{
    IFC_RETURN(LoadResourceSatelliteDlls());
    IFCPTR_RETURN(resourceData);

    auto resInfo = FindResource(m_hNonLocalizedResource, MAKEINTRESOURCE(resourceId), resourceType.GetBuffer());
    IFCW32_RETURN(resInfo != nullptr);

    auto size = SizeofResource(m_hNonLocalizedResource, resInfo);
    IFCW32_RETURN(size > 0u);

    auto resource = LoadResource(m_hNonLocalizedResource, resInfo);
    IFCW32_RETURN(resource != nullptr);

    resourceData->Allocate(size);
    memcpy(resourceData->GetData(), LockResource(resource), resourceData->GetSize());

    return S_OK;
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      IPALWindowlessHost factory method for Windows-less hosts that creates
//      a fresh instance of CWindowlessSiteHost.
//
//------------------------------------------------------------------------------
IPALWindowlessHost *
CXcpBrowserHost::CreateWindowlessHost(_In_ IXcpHostSite* pHostSite, _In_ CDependencyObject* pParentEditBox, _In_ XUINT32 uRuntimeId)
{
    IPALWindowlessHost* pWindowlessHost = NULL;
    pWindowlessHost = new CWindowlessSiteHost();
#if XCP_MONITOR
    ::XcpDebugSetLeakDetectionFlag(pWindowlessHost, true);
#endif
    IFCFAILFAST(pWindowlessHost->Initialize(pHostSite, pParentEditBox, uRuntimeId));

    return pWindowlessHost;
}


