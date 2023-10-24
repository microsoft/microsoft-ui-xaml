// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UserControl.g.h"

using namespace DirectUI;

_Check_return_ HRESULT UserControl::FindNameInPage(
   _In_ HSTRING strElementName, 
   _In_ bool fIsCalledFromUserControl,
   _Outptr_ IInspectable **ppObj)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spParent;

    if (fIsCalledFromUserControl)
    {
        IFC(FindName(strElementName, ppObj));
        goto Cleanup;
    }

    IFC(GetInheritanceParent(&spParent));
    if (ctl::is<IFrameworkElement>(spParent))
    {
        IFC(spParent.Cast<FrameworkElement>()->FindNameInPage(strElementName, TRUE /* fIsCalledFromUserControl */, ppObj));
    }
    else 
    {
        // No parent means that we can't proceed with the search
        *ppObj = NULL;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UserControl::GetCalculatedDefaultStyleKey(
    _Outptr_result_maybenull_ const CClassInfo** ppType,
    _Outptr_result_maybenull_ IInspectable** ppBoxedKey)
{
    *ppType = NULL;
    *ppBoxedKey = NULL;
    RRETURN(S_OK);
}

_Check_return_ HRESULT UserControl::RegisterAppBarsCallback(_In_ CDependencyObject* nativeDO)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spTargetDO;
    IFC(DXamlCore::GetCurrent()->TryGetPeer(nativeDO, &spTargetDO));
    if (spTargetDO)
    {
        IFC(spTargetDO.Cast<UserControl>()->RegisterAppBars());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UserControl::UnregisterAppBarsCallback(_In_ CDependencyObject* nativeDO)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spTargetDO;
    IFC(DXamlCore::GetCurrent()->TryGetPeer(nativeDO, &spTargetDO));
    if (spTargetDO)
    {
        IFC(spTargetDO.Cast<UserControl>()->UnregisterAppBars());
    }

Cleanup:
    RRETURN(hr);
}
