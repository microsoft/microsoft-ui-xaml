// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BindingExpression.g.h"
#include "TextBlock.g.h"
#include "TextBlockAutomationPeer.g.h"
#include "TextPointer.g.h"
#include "TextPointerWrapper.g.h"
#include "InlineCollection.g.h"
#include "TextControlHelper.h"

using namespace DirectUI;

// Initializes a new instance of the TextBlock class.
TextBlock::TextBlock()
{
}

// Deconstructor
TextBlock::~TextBlock()
{
}

IFACEMETHODIMP TextBlock::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ITextBlockAutomationPeer> spTextBlockAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::ITextBlockAutomationPeerFactory> spTextBlockAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::TextBlockAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spTextBlockAPFactory));

    IFC(spTextBlockAPFactory.Cast<TextBlockAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spTextBlockAutomationPeer));
    IFC(spTextBlockAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextBlock::GetPlainText(_Out_ HSTRING* pStrPlainText)
{
    HRESULT hr = S_OK;
    IFCPTR(pStrPlainText);
    IFC(get_Text(pStrPlainText));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::TextBlock::get_ContentStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetContentEdge(CTextPointerWrapper::ElementEdge::ContentStart, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::TextBlock::get_ContentEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetContentEdge(CTextPointerWrapper::ElementEdge::ContentEnd, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::TextBlock::get_SelectionStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetSelectionEdge(CTextPointerWrapper::ElementEdge::ContentStart, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::TextBlock::get_SelectionEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetSelectionEdge(CTextPointerWrapper::ElementEdge::ContentEnd, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextBlock::get_BaselineOffsetImpl(
    _Out_ DOUBLE* pValue
    )
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

_Check_return_ HRESULT DirectUI::TextBlock::SelectImpl(
    _In_ xaml_docs::ITextPointer* pStart,
    _In_ xaml_docs::ITextPointer* pEnd
    )
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::TextPointerWrapper> spStartInternalPointer;
    ctl::ComPtr<DirectUI::TextPointerWrapper> spEndInternalPointer;
    ctl::ComPtr<DependencyObject> spDOStart;
    ctl::ComPtr<DependencyObject> spDOEnd;
    BoxerBuffer buffer;
    CValue valueStart;
    CValue valueEnd;

    IFCPTR(pStart);
    IFCPTR(pEnd);

    IFC((static_cast<TextPointer* >(pStart))->get_InternalPointer(&spStartInternalPointer));
    IFC((static_cast<TextPointer* >(pEnd))->get_InternalPointer(&spEndInternalPointer));

    IFC(CValueBoxer::BoxObjectValue(&valueStart, /* pSourceType*/ NULL, ctl::iinspectable_cast(spStartInternalPointer.Get()), &buffer, &spDOStart));
    IFC(CValueBoxer::BoxObjectValue(&valueEnd, /* pSourceType*/ NULL, ctl::iinspectable_cast(spEndInternalPointer.Get()), &buffer, &spDOEnd));

    IFC(CoreImports::TextBlock_Select(
        static_cast<::CTextBlock*>(GetHandle()),
        &valueStart,
        &valueEnd));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextBlock::GetAlphaMaskImpl(
    _Outptr_ WUComp::ICompositionBrush** ppResult)
{
    CTextBlock* pTextBlock = static_cast<CTextBlock*>(GetHandle());
    IFC_RETURN(pTextBlock->GetAlphaMask(ppResult));
    return S_OK;
}

_Check_return_ HRESULT TextBlock::CopySelectionToClipboardImpl()
{
    IFC_RETURN(static_cast<CTextBlock*>(GetHandle())->CopySelectedText());
    return S_OK;
}

_Check_return_ HRESULT DirectUI::TextBlock::GetContentEdge(
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

    IFC(CoreImports::TextBlock_GetEdge(
        static_cast<CTextBlock*>(GetHandle()),
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

_Check_return_ HRESULT DirectUI::TextBlock::GetSelectionEdge(
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

    IFC(CoreImports::TextBlock_GetSelectionEdge(
        static_cast<CTextBlock*>(GetHandle()),
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
//  During a DisconnectVisualChildrenRecursive tree walk, clear Inlines property as well.
//
//-----------------------------------------------------------------------------

IFACEMETHODIMP
TextBlock::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_docs::Inline*>> spInlines;

    IFC(get_Inlines(&spInlines));
    IFC(spInlines.Cast<InlineCollection>()->DisconnectVisualChildrenRecursive());

    IFC(TextBlockGenerated::OnDisconnectVisualChildren());

Cleanup:
    RRETURN(hr);
}

bool TextBlock::HasDataboundText(_In_ CTextBlock* nativeTextBlock)
{
    ctl::ComPtr<DependencyObject> textBlockAsDO;

    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer(nativeTextBlock, &textBlockAsDO));

    if (textBlockAsDO)
    {
        const CDependencyProperty* textPropertyNoRef = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TextBlock_Text);
        ctl::ComPtr<ITextBlock> textBlock;
        ctl::ComPtr<xaml_data::IBindingExpression> bindingExpression;

        IFCFAILFAST(textBlockAsDO.As(&textBlock));
        IFCFAILFAST(textBlock.Cast<TextBlock>()->GetBindingExpression(textPropertyNoRef, &bindingExpression));
        return bindingExpression != nullptr;
    }

    return false;
}

_Check_return_ HRESULT TextBlock::OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
{
    return (TextControlHelper::OnContextMenuOpeningHandler<ITextBlock, TextBlock>(pNativeTextBlock, cursorLeft, cursorTop, handled));
}

_Check_return_ HRESULT TextBlock::QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeTextBlock)
{
    return TextControlHelper::QueueUpdateSelectionFlyoutVisibility<TextBlock>(pNativeTextBlock);
}

_Check_return_ HRESULT TextBlock::UpdateSelectionFlyoutVisibility()
{
    return TextControlHelper::UpdateSelectionFlyoutVisibility<CTextBlock>(this);
}
