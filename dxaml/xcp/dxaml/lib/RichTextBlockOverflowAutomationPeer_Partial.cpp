// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockOverflowAutomationPeer.g.h"
#include "RichTextBlockOverflow.g.h"
#include "TextAdapter.g.h"
#include "RichTextBlock.g.h"
#include "Block.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT RichTextBlockOverflowAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IRichTextBlockOverflow* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IRichTextBlockOverflowAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRichTextBlockOverflowAutomationPeer* pInstance = nullptr;
    IInspectable* pInner = nullptr;
    xaml::IUIElement* ownerAsUIE = nullptr;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == nullptr || ppInner != nullptr);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<RichTextBlockOverflow*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<RichTextBlockOverflowAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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
    return hr;
}

RichTextBlockOverflowAutomationPeer::RichTextBlockOverflowAutomationPeer() : m_pTextPattern(nullptr)
{
}

RichTextBlockOverflowAutomationPeer::~RichTextBlockOverflowAutomationPeer()
{
    if (m_pTextPattern)
    {
        m_pTextPattern->InvalidateOwner();
    }
    ctl::release_interface(m_pTextPattern);
}

IFACEMETHODIMP RichTextBlockOverflowAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Text)
    {
        if (m_pTextPattern == nullptr)
        {
            ctl::ComPtr<IUIElement> spOwner;
            ctl::ComPtr<TextAdapter> spTextAdapter;
            IFC(get_Owner(&spOwner));
            IFCPTR(spOwner.Get());

            // RichTextBlockOverflows that don't have a master RichTextBlock don't have a text pattern, and should return nullptr.
            if (static_cast<CRichTextBlockOverflow*>((static_cast<RichTextBlockOverflow*>(spOwner.Get())->GetHandle()))->m_pMaster != nullptr)
            {
                IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::TextAdapter, static_cast<RichTextBlockOverflow*>(spOwner.Get())->GetHandle(), spTextAdapter.GetAddressOf()));

                IFCPTR(spTextAdapter.Get());

                m_pTextPattern = spTextAdapter.Detach();
                IFC(m_pTextPattern->put_Owner(spOwner.Get()));
            }
        }
        *ppReturnValue = ctl::as_iinspectable((m_pTextPattern));
        ctl::addref_interface(m_pTextPattern);
    }
    else
    {
        IFC(RichTextBlockOverflowAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    return hr;
}

IFACEMETHODIMP RichTextBlockOverflowAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"RichTextBlockOverflow")).CopyTo(returnValue));
    return S_OK;
}

IFACEMETHODIMP RichTextBlockOverflowAutomationPeer::GetAutomationControlTypeCore(
    _Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Text;
    return S_OK;
}

// We populate automation peer childrens from its block collection recursively
// Here we need to eliminate all text elements which are
// are present in the previous RichTextBlock/RichTextBlockOverflow
// overflowing to next RichTextOverflow if any
IFACEMETHODIMP RichTextBlockOverflowAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    UINT count = 0;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<wfc::IVector<xaml_docs::Block*>> spBlocks;
    ctl::ComPtr<IBlock> spBlock;
    ctl::ComPtr<xaml_docs::ITextPointer> spBlockStart;
    ctl::ComPtr<xaml_controls::IRichTextBlock> spSourceControl;
    ctl::ComPtr<xaml_controls::IRichTextBlockOverflow> spOverflowControl;
    ctl::ComPtr<xaml_docs::ITextPointer> spOverflowStart;
    ctl::ComPtr<xaml_docs::ITextPointer> spContentStart;

    INT posBlockStart = 0;
    INT posContentStart = 0;
    BOOLEAN hasOverflow = FALSE;
    INT posOverflowStart = INT_MAX;

    IFCPTR(ppReturnValue);
    *ppReturnValue = nullptr;

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());

    RichTextBlockOverflowAutomationPeerGenerated::GetChildrenCore(ppReturnValue);
    IFCPTR(*ppReturnValue);

    IFC(spOwner.Cast<RichTextBlockOverflow>()->get_ContentStart(&spContentStart));
    if (spContentStart.Get())
    {
        IFC(spContentStart->get_Offset(&posContentStart));

        IFC(spOwner.Cast<RichTextBlockOverflow>()->get_HasOverflowContent(&hasOverflow));
        if (hasOverflow)
        {
            IFC(spOwner.Cast<RichTextBlockOverflow>()->get_OverflowContentTarget(&spOverflowControl));
            if (spOverflowControl)
            {
                IFC(spOverflowControl.Cast<RichTextBlockOverflow>()->get_ContentStart(&spOverflowStart));
                if (spOverflowStart)
                {
                    IFC(spOverflowStart->get_Offset(&posOverflowStart));
                }
            }
        }

        IFC(spOwner.Cast<RichTextBlockOverflow>()->get_ContentSource(&spSourceControl));
        IFC(spSourceControl.Cast<RichTextBlock>()->get_Blocks(&spBlocks));
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
            IFC(spBlock.Cast<Block>()->AppendAutomationPeerChildren(*ppReturnValue, posContentStart, posOverflowStart));
        }
    }

Cleanup:
    if (FAILED(hr) && ppReturnValue && *ppReturnValue)
    {
        ReleaseInterface(*ppReturnValue);
    }
    return hr;
}
