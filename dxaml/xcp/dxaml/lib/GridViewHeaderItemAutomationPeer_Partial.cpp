// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewHeaderItemAutomationPeer.g.h"
#include "GridViewHeaderItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT GridViewHeaderItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IGridViewHeaderItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IGridViewHeaderItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IGridViewHeaderItemAutomationPeer* pInstance = nullptr;
    IInspectable* pInner = nullptr;
    xaml::IUIElement* ownerAsUIE = nullptr;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == nullptr || ppInner != nullptr);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<GridViewHeaderItem*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<GridViewHeaderItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = nullptr;
    }

    *ppInstance = pInstance;
    pInstance = nullptr;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

IFACEMETHODIMP GridViewHeaderItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GridViewHeaderItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}
