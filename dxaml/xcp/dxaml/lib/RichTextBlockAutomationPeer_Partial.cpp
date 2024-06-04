// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockAutomationPeer.g.h"
#include "RichTextBlock.g.h"
#include "TextAdapter.g.h"
#include "RichTextBlockOverflow.g.h"
#include "Block.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT RichTextBlockAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IRichTextBlock* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IRichTextBlockAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRichTextBlockAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<RichTextBlock*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<RichTextBlockAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

RichTextBlockAutomationPeer::RichTextBlockAutomationPeer() : m_pTextPattern(NULL)
{
}

RichTextBlockAutomationPeer::~RichTextBlockAutomationPeer()
{
    if(m_pTextPattern)
    {
        m_pTextPattern->InvalidateOwner();
    }
    ctl::release_interface(m_pTextPattern);
}

IFACEMETHODIMP RichTextBlockAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Text)
    {
        if (!m_pTextPattern)
        {
            ctl::ComPtr<IUIElement> spOwner;
            ctl::ComPtr<TextAdapter> spTextAdapter;

            IFC(get_Owner(&spOwner));
            IFCPTR(spOwner.Get());

            IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::TextAdapter, static_cast<RichTextBlock*>(spOwner.Get())->GetHandle(), spTextAdapter.GetAddressOf()));

            m_pTextPattern = spTextAdapter.Detach();
            IFC(m_pTextPattern->put_Owner(spOwner.Get()));
        }
        *ppReturnValue = ctl::as_iinspectable((m_pTextPattern));
        ctl::addref_interface(m_pTextPattern);
    }
    else
    {
        IFC(RichTextBlockAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RichTextBlockAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"RichTextBlock")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RichTextBlockAutomationPeer::GetAutomationControlTypeCore(
    _Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Text;
    RRETURN(S_OK);
}

// We populate automation peer childrens from its block collection recursively
// Here we need to eliminate all text elements which are overflowing to next RichTextBlockOverflow if any
IFACEMETHODIMP RichTextBlockAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    UINT count = 0;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<wfc::IVector<xaml_docs::Block*>> spBlocks;
    ctl::ComPtr<IBlock> spBlock;
    ctl::ComPtr<xaml_docs::ITextPointer> spBlockStart;
    INT posBlockStart = -1;

    BOOLEAN hasOverflow = FALSE;
    ctl::ComPtr<xaml_controls::IRichTextBlockOverflow> spOverflowControl;
    ctl::ComPtr<xaml_docs::ITextPointer> spOverflowStart;
    INT posOverflowStart = INT_MAX;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;


    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());

    RichTextBlockAutomationPeerGenerated::GetChildrenCore(ppReturnValue);
    IFCPTR(*ppReturnValue);

    IFC(spOwner.Cast<RichTextBlock>()->get_HasOverflowContent(&hasOverflow));
    if (hasOverflow)
    {
        IFC(spOwner.Cast<RichTextBlock>()->get_OverflowContentTarget(&spOverflowControl));
        if (spOverflowControl)
        {
            IFC(spOverflowControl.Cast<RichTextBlockOverflow>()->get_ContentStart(&spOverflowStart));
            if (spOverflowStart)
            {
                IFC(spOverflowStart->get_Offset(&posOverflowStart));
            }
        }
    }

    IFC(spOwner.Cast<RichTextBlock>()->get_Blocks(&spBlocks));
    IFC(spBlocks->get_Size(&count));
    for(UINT i = 0; i < count; i++)
    {
        IFC(spBlocks->GetAt(i, &spBlock));
        if (hasOverflow)
        {
            IFC(spBlock.Cast<Block>()->get_ContentStart(&spBlockStart));
            IFC(spBlockStart->get_Offset(&posBlockStart));
            if (posBlockStart >= posOverflowStart)
            {
                break;
            }
        }
        IFC(spBlock.Cast<Block>()->AppendAutomationPeerChildren(*ppReturnValue, 0, posOverflowStart));
    }

Cleanup:
    if (FAILED(hr) && ppReturnValue && *ppReturnValue)
    {
        ReleaseInterface(*ppReturnValue);
    }
    RRETURN(hr);
}
