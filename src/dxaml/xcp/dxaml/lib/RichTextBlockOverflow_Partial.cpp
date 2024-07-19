// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockOverflow.g.h"
#include "RichTextBlockOverflowAutomationPeer.g.h"
#include "TextPointer.g.h"
#include "TextPointerWrapper.g.h"

using namespace DirectUI;

// Initializes a new instance of the RichTextBlockOverflow class.
DirectUI::RichTextBlockOverflow::RichTextBlockOverflow()
{
}

// Deconstructor
DirectUI::RichTextBlockOverflow::~RichTextBlockOverflow()
{
}

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::GetPlainText(_Out_ HSTRING* pStrPlainText)
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

IFACEMETHODIMP
DirectUI::RichTextBlockOverflow::OnCreateAutomationPeer(
    xaml_automation_peers::IAutomationPeer** ppAutomationPeer
    )
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRichTextBlockOverflowAutomationPeer* pRichTextBlockOverflowAutomationPeer = NULL;
    xaml_automation_peers::IRichTextBlockOverflowAutomationPeerFactory* pRichTextBlockOverflowAPFactory = NULL;
    IActivationFactory* pActivationFactory = NULL;
    IInspectable* inner = NULL;

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::RichTextBlockOverflowAutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pRichTextBlockOverflowAPFactory, pActivationFactory));

    IFC(static_cast<RichTextBlockOverflowAutomationPeerFactory*>(pRichTextBlockOverflowAPFactory)->CreateInstanceWithOwner(this,
        NULL,
        &inner,
        &pRichTextBlockOverflowAutomationPeer));
    IFC(ctl::do_query_interface(*ppAutomationPeer, pRichTextBlockOverflowAutomationPeer));

Cleanup:
    ReleaseInterface(pRichTextBlockOverflowAutomationPeer);
    ReleaseInterface(pRichTextBlockOverflowAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::get_ContentSourceImpl(
    _Outptr_ xaml_controls::IRichTextBlock** pValue)
{
    HRESULT hr = S_OK;
    CRichTextBlock *pCoreMaster = NULL;
    DependencyObject *pMaster = NULL;

    IFC(CoreImports::CRichTextBlockOverflow_GetMaster(
        static_cast<CRichTextBlockOverflow *>(GetHandle()),
        &pCoreMaster));

    if (pCoreMaster)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(pCoreMaster, &pMaster));
    }

    IFC(ctl::do_query_interface(*pValue, pMaster));

Cleanup:
    ctl::release_interface(pMaster);
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::get_ContentStartImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetContentEdge(CTextPointerWrapper::ElementEdge::ContentStart, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::get_ContentEndImpl(
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;

    IFC(GetContentEdge(CTextPointerWrapper::ElementEdge::ContentEnd, pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::get_BaselineOffsetImpl(
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

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::GetPositionFromPointImpl(
    _In_ wf::Point point,
    _Outptr_ xaml_docs::ITextPointer** returnValue)
{
    ctl::ComPtr<xaml_docs::ITextPointer> pointer;
    CValue result;
    CDependencyObject* pDO = nullptr;
    ctl::ComPtr<DependencyObject> spInternalPointer;
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
        IFC_RETURN(pCore->GetPeer(pDO, &spInternalPointer));
        IFC_RETURN(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spInternalPointer.Cast<TextPointerWrapper>(),
            &pointer));
    }

    *returnValue = pointer.Detach();

    return S_OK;
}

_Check_return_ HRESULT DirectUI::RichTextBlockOverflow::GetContentEdge(
    _In_ CTextPointerWrapper::ElementEdge::Enum edge,
    _Outptr_ xaml_docs::ITextPointer** pValue)
{
    HRESULT hr = S_OK;
    xaml_docs::ITextPointer* pPointer = NULL;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject > spInternalPointer;

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
        IFC(pCore->GetPeer(pDO, &spInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spInternalPointer.Cast<TextPointerWrapper>(),
            &pPointer));
    }

    *pValue = pPointer;
    pPointer = NULL;

Cleanup:
    ReleaseInterface(pPointer);
    RRETURN(hr);
}
