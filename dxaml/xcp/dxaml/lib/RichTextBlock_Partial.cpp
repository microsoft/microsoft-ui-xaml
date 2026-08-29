// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlock.g.h"
#include "RichTextBlockAutomationPeer.g.h"
#include "TextPointer.g.h"
#include "TextPointerWrapper.g.h"
#include "BlockCollection.g.h"
#include "TextControlHelper.h"

using namespace DirectUI;

// Initializes a new instance of the RichTextBlock class.
DirectUI::RichTextBlock::RichTextBlock()
{
}

// Deconstructor
DirectUI::RichTextBlock::~RichTextBlock()
{
}

_Check_return_ HRESULT DirectUI::RichTextBlock::GetPlainText(_Out_ HSTRING* pStrPlainText)
{
    HRESULT hr = S_OK;

    xruntime_string_ptr strPlainText;
    xstring_ptr strLocalPlainText;

    IFCPTR(pStrPlainText);

    IFC(CoreImports::CRichTextBlock_GetPlainText(
        static_cast<CFrameworkElement*>(GetHandle()),
        &strLocalPlainText));

    IFC(strLocalPlainText.Promote(&strPlainText));

    *pStrPlainText = strPlainText.DetachHSTRING();

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::RichTextBlock::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IRichTextBlockAutomationPeer> spRichTextBlockAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IRichTextBlockAutomationPeerFactory> spRichTextBlockAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::RichTextBlockAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spRichTextBlockAPFactory));

    IFC(spRichTextBlockAPFactory.Cast<RichTextBlockAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spRichTextBlockAutomationPeer));
    IFC(spRichTextBlockAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::get_ContentStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetContentEdge(CTextPointerWrapper::ElementEdge::ContentStart, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::get_ContentEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetContentEdge(CTextPointerWrapper::ElementEdge::ContentEnd, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::get_SelectionStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetSelectionEdge(CTextPointerWrapper::ElementEdge::ContentStart, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::get_SelectionEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetSelectionEdge(CTextPointerWrapper::ElementEdge::ContentEnd, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::get_BaselineOffsetImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    XFLOAT baselineOffset = 0.0f;

    IFCPTR(pValue);
    IFC(CoreImports::Text_GetBaselineOffset(
        GetHandle(),
        &baselineOffset));

    *pValue = baselineOffset;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::SelectImpl(
    _In_ xaml_docs::ITextPointer* start,
    _In_ xaml_docs::ITextPointer* end)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TextPointerWrapper> spStartInternalPointer;
    ctl::ComPtr<TextPointerWrapper> spEndInternalPointer;
    ctl::ComPtr<DependencyObject> spDOStart;
    ctl::ComPtr<DependencyObject> spDOEnd;
    BoxerBuffer buffer;
    CValue valueStart;
    CValue valueEnd;

    IFCPTR(start);
    IFCPTR(end);

    IFC((static_cast<TextPointer*>(start))->get_InternalPointer(&spStartInternalPointer));
    IFC((static_cast<TextPointer*>(end))->get_InternalPointer(&spEndInternalPointer));

    IFC(CValueBoxer::BoxObjectValue(&valueStart, /* pSourceType */ NULL, ctl::iinspectable_cast(spStartInternalPointer.Get()), &buffer,  &spDOStart));
    IFC(CValueBoxer::BoxObjectValue(&valueEnd, /* pSourceType */ NULL, ctl::iinspectable_cast(spEndInternalPointer.Get()), &buffer, &spDOEnd));

    IFC(CoreImports::CRichTextBlock_Select(
        static_cast<CRichTextBlock*>(GetHandle()),
        &valueStart,
        &valueEnd));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::GetPositionFromPointImpl(
    _In_ wf::Point point,
    _Outptr_ xaml_docs::ITextPointer** returnValue
    )
{
    ctl::ComPtr<xaml_docs::ITextPointer> spPointer;
    CValue result;
    CDependencyObject* pDO = nullptr;
    XPOINTF pointValue;

    IFCPTR_RETURN(returnValue);
    *returnValue = nullptr;

    pointValue.x = point.X;
    pointValue.y = point.Y;

    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFC_RETURN(CoreImports::CRichTextBlock_GetTextPositionFromPoint(
        static_cast<CFrameworkElement*>(GetHandle()),
        pointValue,
        &result));

    pDO = result.AsObject();
    if (pDO != nullptr)
    {
        ctl::ComPtr<DependencyObject> spInternalPointer;

        IFC_RETURN(pCore->GetPeer(pDO, &spInternalPointer));
        IFC_RETURN(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spInternalPointer.Cast<TextPointerWrapper>(),
            &spPointer));
    }

    IFC_RETURN(spPointer.MoveTo(returnValue));

    return S_OK;
}

_Check_return_ HRESULT RichTextBlock::CopySelectionToClipboardImpl()
{
    IFC_RETURN(static_cast<CRichTextBlock*>(GetHandle())->CopySelectedText());
    return S_OK;
}

_Check_return_ HRESULT DirectUI::RichTextBlock::GetContentEdge(
    _In_ CTextPointerWrapper::ElementEdge::Enum edge,
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_docs::ITextPointer> spPointer;
    CValue result;
    CDependencyObject* pDO = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::CRichTextBlock_GetEdge(
        static_cast<CFrameworkElement*>(GetHandle()),
        edge,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        ctl::ComPtr<DependencyObject> spInternalPointer;

        IFC(pCore->GetPeer(pDO, &spInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spInternalPointer.Cast<TextPointerWrapper>(),
            &spPointer));
    }

    IFC(spPointer.MoveTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlock::GetSelectionEdge(
    _In_ CTextPointerWrapper::ElementEdge::Enum edge,
    _Outptr_ xaml_docs::ITextPointer** pValue
    )
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_docs::ITextPointer> spPointer;
    CValue result;
    CDependencyObject* pDO = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    IFC(CoreImports::CRichTextBlock_GetSelectionEdge(
        static_cast<CRichTextBlock*>(GetHandle()),
        edge,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        ctl::ComPtr<DependencyObject> spInternalPointer;

        IFC(pCore->GetPeer(pDO, &spInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spInternalPointer.Cast<TextPointerWrapper>(),
            &spPointer));
    }

    IFC(spPointer.MoveTo(pValue));

Cleanup:
    RRETURN(hr);
}



//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, clear Blocks property as well.
//
//-----------------------------------------------------------------------------


IFACEMETHODIMP
DirectUI::RichTextBlock::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_docs::Block*>> spBlocks;

    IFC(get_Blocks(&spBlocks));

    IFC(spBlocks.Cast<BlockCollection>()->DisconnectVisualChildrenRecursive());

    IFC(RichTextBlockGenerated::OnDisconnectVisualChildren());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RichTextBlock::OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeRichTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
{
    return (TextControlHelper::OnContextMenuOpeningHandler<IRichTextBlock, RichTextBlock>(pNativeRichTextBlock, cursorLeft, cursorTop, handled));
}

_Check_return_ HRESULT RichTextBlock::QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeRichTextBlock)
{
    return TextControlHelper::QueueUpdateSelectionFlyoutVisibility<RichTextBlock>(pNativeRichTextBlock);
}

_Check_return_ HRESULT RichTextBlock::UpdateSelectionFlyoutVisibility()
{
    return TextControlHelper::UpdateSelectionFlyoutVisibility<CRichTextBlock>(this);
}