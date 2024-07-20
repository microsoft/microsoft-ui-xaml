// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBlockAutomationPeer.g.h"
#include "TextBlock.g.h"
#include "TextAdapter.g.h"
#include "Inline.g.h"
#include "UIAEnums.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT TextBlockAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::ITextBlock* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ITextBlockAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ITextBlockAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<TextBlock*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<TextBlockAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Deconstructor
TextBlockAutomationPeer::~TextBlockAutomationPeer()
{
    if (auto peg = m_tpTextPattern.TryMakeAutoPeg())
    {
        ctl::ComPtr<ITextAdapter> spTextAdapter = m_tpTextPattern.AsOrNull<ITextAdapter>();
        if (spTextAdapter)
        {
            spTextAdapter.Cast<TextAdapter>()->InvalidateOwner();
        }
        m_tpTextPattern.Clear();
    }
}

IFACEMETHODIMP TextBlockAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    if (patternInterface == xaml_automation_peers::PatternInterface_Text)
    {
        ctl::ComPtr<IInspectable> spPattern;
        if(!m_tpTextPattern)
        {
            ctl::ComPtr<IUIElement> spOwner;
            ctl::ComPtr<TextAdapter> spTextAdapter;
            ctl::ComPtr<xaml_automation::Provider::ITextProvider> spTextPattern;

            IFC_RETURN(get_Owner(&spOwner));

            IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::TextAdapter, static_cast<TextBlock*>(spOwner.Get())->GetHandle(), spTextAdapter.GetAddressOf()));

            IFCPTR_RETURN(spTextAdapter.Get());
            IFC_RETURN(spTextAdapter.As(&spTextPattern));
            SetPtrValue(m_tpTextPattern, spTextPattern.Get());

            IFC_RETURN(spTextAdapter->put_Owner(spOwner.Get()));
        }
        IFC_RETURN(m_tpTextPattern.As(&spPattern));
        *ppReturnValue = spPattern.Detach();
    }
    else
    {
        IFC_RETURN(TextBlockAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP TextBlockAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"TextBlock")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP TextBlockAutomationPeer::GetAutomationControlTypeCore(
    _Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Text;
    RRETURN(S_OK);
}

IFACEMETHODIMP TextBlockAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    UINT count = 0;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<wfc::IVector<xaml_docs::Inline*>> spInlines;
    ctl::ComPtr<IInline> spInline;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    IFC(ctl::make<TrackerCollection<xaml_automation_peers::AutomationPeer*>>(&spAPChildren));

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());

    IFC(spOwner.Cast<TextBlock>()->get_Inlines(&spInlines));
    IFC(spInlines->get_Size(&count));
    for(UINT i = 0; i < count; i++)
    {
        IFC(spInlines->GetAt(i, &spInline));
        IFC(spInline.Cast<Inline>()->AppendAutomationPeerChildren(spAPChildren.Get(), -1, INT_MAX));
    }

    IFC(spAPChildren.CopyTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}
