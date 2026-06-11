// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextPointer.g.h"
#include "TextPointerWrapper.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

TextPointer::TextPointer()
    : m_pInternalPointer(NULL)
{
}

TextPointer::~TextPointer()
{
    ctl::release_interface(m_pInternalPointer);
}

_Check_return_ HRESULT TextPointer::CreateInstanceWithInternalPointer(
    _In_ DirectUI::TextPointerWrapper* internalPointer, 
    _Outptr_ xaml_docs::ITextPointer** returnValue
    )
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    xaml_docs::ITextPointer* pInstance = NULL;
    IActivationFactory* pActivationFactory = NULL;

    IFCPTR(internalPointer);

    pActivationFactory = ctl::ActivationFactoryCreator<ctl::ActivationFactory<TextPointer>>::CreateActivationFactory();

    IFC(pActivationFactory->ActivateInstance(&pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC((static_cast<TextPointer *>(pInstance))->put_InternalPointer(internalPointer));

    *returnValue = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    ReleaseInterface(pActivationFactory);
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::get_InternalPointer(_Outptr_ DirectUI::TextPointerWrapper** pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    ctl::addref_interface(m_pInternalPointer);
    *pValue = m_pInternalPointer;
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::put_InternalPointer(_In_ DirectUI::TextPointerWrapper* value)
{
    HRESULT hr = S_OK;
    IFCPTR(value);
    ctl::release_interface(m_pInternalPointer);
    m_pInternalPointer = value;
    IFCPTR(m_pInternalPointer);
    ctl::addref_interface(m_pInternalPointer);
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::get_ParentImpl(
    _Outptr_ xaml::IDependencyObject** pValue
    )
{
    HRESULT hr = S_OK;
    CValue result;
    CDependencyObject* pDO = NULL;
    DependencyObject* pParent = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    // There is a case where the parent should be gone, but it's not yet, because it's still on the
    // release queue.  (See bug 624391.)  To mitigate against that, flush the queue first.
    DXamlCore::GetCurrent()->ReleaseQueuedObjects();

    IFC(CoreImports::TextPointer_GetParent(
        static_cast<CTextPointerWrapper *>(m_pInternalPointer->GetHandle()),
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &pParent));
        *pValue = pParent;
        pParent = NULL;
    }

Cleanup:
    ctl::release_interface(pParent);
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::get_VisualParentImpl(
    _Outptr_ xaml::IFrameworkElement** pValue
    )
{
    HRESULT hr = S_OK;
    CValue result;
    CDependencyObject* pDO = NULL;
    DependencyObject* pParent = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(pValue);
    *pValue = NULL;

    // There is a case where the parent should be gone, but it's not yet, because it's still on the
    // release queue.  (See bug 624391.)  To mitigate against that, flush the queue first.
    DXamlCore::GetCurrent()->ReleaseQueuedObjects();
    
    IFC(CoreImports::TextPointer_GetVisualParent(
        static_cast<CTextPointerWrapper *>(m_pInternalPointer->GetHandle()),
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &pParent));
        IFC(ctl::do_query_interface(*pValue, pParent));
    }

Cleanup:
    ctl::release_interface(pParent);
    RRETURN(hr);
}
        
_Check_return_ HRESULT TextPointer::get_LogicalDirectionImpl(
    _Out_ xaml_docs::LogicalDirection* pValue
    )
{
    HRESULT hr = S_OK;
    CValue resultValue;
    UINT enumValue = 0;

    IFCPTR(pValue);

    IFC(CoreImports::TextPointer_GetLogicalDirection(
        static_cast<CTextPointerWrapper *>(m_pInternalPointer->GetHandle()),
        &resultValue));

    IFC(CValueBoxer::UnboxEnumValue(&resultValue, NULL, &enumValue));
    *pValue = (xaml_docs::LogicalDirection)enumValue;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::get_OffsetImpl(
    _Out_ INT* pValue
    )
{
    HRESULT hr = S_OK;
    XINT32 offset = 0;

    IFC(CoreImports::TextPointer_GetOffset(
        static_cast<CTextPointerWrapper *>(m_pInternalPointer->GetHandle()),
        &offset));
    
    *pValue = offset;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::GetCharacterRectImpl(
    _In_ xaml_docs::LogicalDirection direction, 
    _Out_ wf::Rect* returnValue
    )
{
    HRESULT hr = S_OK;
    
    IFC(CoreImports::TextPointer_GetCharacterRect(
        static_cast<CTextPointerWrapper *>(m_pInternalPointer->GetHandle()),
        direction,
        returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextPointer::GetPositionAtOffsetImpl(
    _In_ INT offset, 
    _In_ xaml_docs::LogicalDirection direction, 
    _Outptr_ xaml_docs::ITextPointer** returnValue
    )
{
    HRESULT hr = S_OK;
    xaml_docs::ITextPointer* pPositionAtOffset = NULL;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject> spPositionAtOffsetInternalPointer;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR(pCore);

    IFCPTR(returnValue);    
    *returnValue = NULL;

    IFC(CoreImports::TextPointer_GetPositionAtOffset(
        static_cast<CTextPointerWrapper *>(m_pInternalPointer->GetHandle()),
        offset,
        direction,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        IFC(pCore->GetPeer(pDO, &spPositionAtOffsetInternalPointer));
        IFC(DirectUI::TextPointer::CreateInstanceWithInternalPointer(
            spPositionAtOffsetInternalPointer.Cast<TextPointerWrapper>(),
            &pPositionAtOffset));
    }

    *returnValue = pPositionAtOffset;
    pPositionAtOffset = NULL;

Cleanup:
    ReleaseInterface(pPositionAtOffset);
    RRETURN(hr);
}

