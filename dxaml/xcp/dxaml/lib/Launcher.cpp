// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HStringUtil.h"
#include "Launcher.h"

using namespace Microsoft::WRL;

static const WCHAR c_szRes[] = L"res";
static const WCHAR c_szFile[] = L"file";
static const WCHAR c_szHttp[] = L"http";
static const WCHAR c_szHttps[] = L"https";

//------------------------------------------------------------------------
//
//  Method: FindScheme
//
//  Synopsis:
//     Is scheme found in list of schemes?
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
FindScheme(
    _In_ HSTRING hstrScheme,
    _In_reads_(cSchemes) _Deref_pre_z_ const WCHAR* pSchemes[],
    size_t cSchemes,
    _Out_ BOOL* pFound)
{
    LPCWSTR pszScheme = NULL;

    IFC_RETURN(HStringUtil::GetLpcwstr(hstrScheme, &pszScheme));

    *pFound = FALSE;

    // Look for scheme
    for (size_t i = 0; i < cSchemes; i++)
    {
        if (!wcscmp(pszScheme, pSchemes[i]))
        {
            *pFound = TRUE;
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: IsUriSchemeAllowedToLaunch
//
//  Synopsis:
//     Is scheme allowed to be launched in another app?
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
IsUriSchemeAllowedToLaunch(
    _In_ HSTRING hstrScheme,
    _Out_ BOOL* pAllowed)
{
    BOOL found = FALSE;

    // Schemes which cannot be opened in new window
    static const WCHAR* unallowedSchemes[] =
    {
        c_szFile,
        c_szRes
    };

    IFC_RETURN(FindScheme(hstrScheme, unallowedSchemes, ARRAYSIZE(unallowedSchemes), &found));

    *pAllowed = (found ? FALSE : TRUE);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: IsUriSchemeTrustedForLaunch
//
//  Synopsis:
//     Is scheme trusted to be launched in another app without a warning message?
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
IsUriSchemeTrustedForLaunch(
    _In_ HSTRING hstrScheme,
    _Out_ BOOL* pTrusted)
{
    BOOL found = FALSE;

    static const WCHAR* safeSchemes[] =
    {
        c_szHttp,
        c_szHttps
    };

    IFC_RETURN(FindScheme(hstrScheme, safeSchemes, ARRAYSIZE(safeSchemes), &found));

    *pTrusted = (found ? TRUE : FALSE);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: InvokeLauncher
//
//  Synopsis:
//     Invoke default protocol handler app using Windows.System.Launcher
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
InvokeLauncher(
    _In_ wf::IUriRuntimeClass* pUri,
    BOOL trusted)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wsy::ILauncherStatics> spLauncher;
    ComPtr<wf::IAsyncOperation<bool>> spLaunchOperation;
    wrl_wrappers::HStringReference launcherClass(RuntimeClass_Windows_System_Launcher);

    auto spCallback = Microsoft::WRL::Callback<wf::IAsyncOperationCompletedHandler<bool>>(
        [](wf::IAsyncOperation<bool>* operation, wf::AsyncStatus /*status*/)->HRESULT
        {
            ctl::ComPtr<IAsyncInfo> spAsyncInfo;
            if (SUCCEEDED(ctl::do_query_interface(spAsyncInfo, operation)))
            {
                spAsyncInfo->Close();
            }

            // We don't care about errors in this code path.
            return S_OK;
        });
    hr = spCallback != nullptr ? S_OK : E_OUTOFMEMORY;
    IFC(hr);

    IFC(ctl::GetActivationFactory(launcherClass.Get(), &spLauncher));
    if (!trusted)
    {
        ComPtr<wsy::ILauncherOptions> spLauncherOptions;
        IFC(wf::ActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_System_LauncherOptions).Get(),
            &spLauncherOptions));

        IFC(spLauncherOptions->put_TreatAsUntrusted(true));
        IFC(spLauncher->LaunchUriWithOptionsAsync(
            pUri,
            spLauncherOptions.Get(),
            &spLaunchOperation));
    }
    else
    {
        IFC(spLauncher->LaunchUriAsync(pUri, &spLaunchOperation));
    }

    IFC(spLaunchOperation->put_Completed(spCallback.Get()));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: TryInvokeLauncher
//
//  Synopsis:
//     Check scheme and invoke default protocol handler app if check
//     passes.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
Launcher::TryInvokeLauncher(
    _In_ wf::IUriRuntimeClass* pUri)
{
    BOOL trusted = FALSE;
    BOOL allowed = FALSE;
    wrl_wrappers::HString strScheme;

    IFC_RETURN(pUri->get_SchemeName(strScheme.GetAddressOf()));

    // Can scheme be opened in external app?
    IFC_RETURN(IsUriSchemeAllowedToLaunch(strScheme, &allowed));
    if (!allowed)
    {
        return S_OK;
    }

    // Does scheme need a user warning before opening external app?
    IFC_RETURN(IsUriSchemeTrustedForLaunch(strScheme, &trusted));

    // Launch URI in external app
    IFC_RETURN(InvokeLauncher(pUri, trusted));

    return S_OK;
}