// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationPeer.g.h"
#include "TextAdapter.g.h"
#include "TextRangeAdapter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the TextRangeAdapter class.
TextRangeAdapter::TextRangeAdapter()
{
}

// Destructor
TextRangeAdapter::~TextRangeAdapter()
{
}

// this implementation is hidden from IDL
_Check_return_ HRESULT TextRangeAdapter::QueryInterfaceImpl( _In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ITextRangeAdapter)))
    {
        *ppObject = static_cast<ITextRangeAdapter*>(this);
    }
    else
    {
        return TextRangeAdapterGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}


// Set the owner of Adapter
_Check_return_ HRESULT TextRangeAdapter::put_Owner(_In_ TextAdapter* pOwner)
{
    if (!m_tpTextPattern)
    {
        ctl::ComPtr<xaml_automation::Provider::ITextProvider> spTextPattern;
        IFC_RETURN(ctl::do_query_interface(spTextPattern, pOwner));
        
        SetPtrValue(m_tpTextPattern, spTextPattern.Get());
    }

    return S_OK;
}

// InvokeImpl override, defines behavior of when Invoke is called on an Item through UIA Client
_Check_return_ HRESULT TextRangeAdapter::CloneImpl(_Outptr_ xaml_automation::Provider::ITextRangeProvider** ppReturnValue)
{
    CValue resultValue;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 0, CValue(), &resultValue));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(
        &resultValue,
        MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextRangeAdapter),
        __uuidof(xaml_automation::Provider::ITextRangeProvider),
        reinterpret_cast<void**>(ppReturnValue)));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::CompareImpl(
    _In_ xaml_automation::Provider::ITextRangeProvider* textRangeProvider,
    _Out_ BOOLEAN* bReturnValue)
{
    CValue resultValue;
    CValue argVal;

    IFCPTR_RETURN(bReturnValue);
    IFCPTR_RETURN(textRangeProvider);
    if (ctl::is<ITextRangeAdapter>(textRangeProvider))
    {
        argVal.WrapObjectNoRef((static_cast<TextRangeAdapter*>(textRangeProvider))->GetHandle());
        IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 1, argVal, &resultValue));
        IFC_RETURN(CValueBoxer::UnboxValue(&resultValue, bReturnValue));
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}
_Check_return_ HRESULT TextRangeAdapter::CompareEndpointsImpl(
    _In_ xaml_automation::Text::TextPatternRangeEndpoint endpoint,
    _In_ xaml_automation::Provider::ITextRangeProvider* textRangeProvider,
    _In_ xaml_automation::Text::TextPatternRangeEndpoint targetEndpoint,
    _Out_ INT* pReturnValue)
{
    CTextRangeAdapter* target = nullptr;

    IFCPTR_RETURN(pReturnValue);
    IFCPTR_RETURN(textRangeProvider);
    if (ctl::is<ITextRangeAdapter>(textRangeProvider))
    {
        target = static_cast<CTextRangeAdapter*>((static_cast<TextRangeAdapter*>(textRangeProvider))->GetHandle());
        IFC_RETURN(CoreImports::TextRangeAdapter_CompareEndpoints(
            static_cast<CTextRangeAdapter*>(GetHandle()),
            (UIAXcp::TextPatternRangeEndpoint)endpoint,
            target,
            (UIAXcp::TextPatternRangeEndpoint)targetEndpoint,
            pReturnValue));
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::ExpandToEnclosingUnitImpl(_In_ xaml_automation::Text::TextUnit unit)
{
    CValue resultValue;
    CValue argVal;

    argVal.SetEnum(unit);
    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 3, argVal, &resultValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::FindAttributeImpl(
    _In_ INT attributeId,
    _In_ IInspectable* value,
    _In_ BOOLEAN backward,
    _Outptr_ xaml_automation::Provider::ITextRangeProvider** ppReturnValue)
{
    if (ppReturnValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    else
    {
        *ppReturnValue = nullptr;
        // Return code ignored by SuspendFailFastOnStowedException in CUIATextRangeProviderWrapper::FindAttribute()
        IFC_RETURN(E_NOTIMPL);
    }
    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::FindTextImpl(
    _In_ HSTRING text,
    _In_ BOOLEAN backward,
    _In_ BOOLEAN ignoreCase,
    _Outptr_ xaml_automation::Provider::ITextRangeProvider** ppReturnValue)
{
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;
    // Return code ignored by SuspendFailFastOnStowedException in CUIATextRangeProviderWrapper::FindText()
    IFC_RETURN(E_NOT_SUPPORTED);

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::GetAttributeValueImpl(_In_ INT attributeId, _Outptr_ IInspectable** ppReturnValue)
{
    CValue resultValue;
    CValue argVal;

    IFCPTR_RETURN(ppReturnValue);

    argVal.SetSigned(attributeId);

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 6, argVal, &resultValue));

    // in case range has mixed values, Mixed Attribute IUnknown must be returned to UIA Core
    // as IInspectable doesn't provide public wrapper for IUnkown we use UnSetValue to identify it.
    if (resultValue.GetType() == valueIUnknown)
    {
        IFC_RETURN(DependencyPropertyFactory::GetUnsetValue(ppReturnValue));
    }
    else
    {
        IFC_RETURN(CValueBoxer::UnboxObjectValue(&resultValue, nullptr, ppReturnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::GetBoundingRectanglesImpl(_Out_ UINT* returnValueCount, _Out_ DOUBLE** ppReturnValue)
{
    HRESULT hr = S_OK;
    INT nChildCount = 4;
    DOUBLE* pReturnValue = nullptr;

    IFCPTR(ppReturnValue);
    IFCPTR(returnValueCount);
    *ppReturnValue = nullptr;
    *returnValueCount = 0;

    IFC(CoreImports::TextRangeAdapter_GetBoundingRectangles(static_cast<CTextRangeAdapter*>(GetHandle()), &pReturnValue, &nChildCount));

    *ppReturnValue = static_cast<DOUBLE*>(CoTaskMemAlloc(sizeof(DOUBLE) * (ULONG)nChildCount));
    IFCOOMFAILFAST(ppReturnValue);
    
    for (int current = 0; current < nChildCount; current++)
    {
        (*ppReturnValue)[current] = pReturnValue[current];
    }

    *returnValueCount = nChildCount;

Cleanup:
    delete [] pReturnValue;
    return hr;
}

_Check_return_ HRESULT TextRangeAdapter::GetEnclosingElementImpl(
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue)
{
    CValue resultValue;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spEnclosingElementAsAP;

    *ppReturnValue = nullptr;

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 8, CValue(), &resultValue));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(&resultValue, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::AutomationPeer), __uuidof(xaml_automation_peers::IAutomationPeer), reinterpret_cast<void**>(spEnclosingElementAsAP.ReleaseAndGetAddressOf())));
    IFCPTR_RETURN(spEnclosingElementAsAP.Get());
    IFC_RETURN(AutomationPeer::ProviderFromPeerStatic(spEnclosingElementAsAP.Get(), ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::GetTextImpl(_In_ INT maxLength, _Out_ HSTRING* pReturnValue)
{
    CValue resultValue;
    CValue argVal;

    IFCPTR_RETURN(pReturnValue);
    argVal.SetSigned(maxLength);

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 9, argVal, &resultValue));
    IFC_RETURN(CValueBoxer::UnboxValue(&resultValue, pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::MoveImpl(_In_ xaml_automation::Text::TextUnit unit, _In_ INT count, _Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = 0;
    IFC_RETURN(CoreImports::TextRangeAdapter_Move(static_cast<CTextRangeAdapter*>(GetHandle()),(UIAXcp::TextUnit)unit, count, pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::MoveEndpointByUnitImpl(_In_ xaml_automation::Text::TextPatternRangeEndpoint endpoint, _In_ xaml_automation::Text::TextUnit unit, _In_ INT count, _Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = 0;
    IFC_RETURN(CoreImports::TextRangeAdapter_MoveEndpointByUnit(
        static_cast<CTextRangeAdapter*>(GetHandle()),
        (UIAXcp::TextPatternRangeEndpoint)endpoint,
        (UIAXcp::TextUnit)unit,
        count,
        pReturnValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::MoveEndpointByRangeImpl(
    _In_ xaml_automation::Text::TextPatternRangeEndpoint endpoint,
    _In_ xaml_automation::Provider::ITextRangeProvider* textRangeProvider,
    _In_ xaml_automation::Text::TextPatternRangeEndpoint targetEndpoint)
{
    CTextRangeAdapter* target = nullptr;
    IFCPTR_RETURN(textRangeProvider);
    if (ctl::is<ITextRangeAdapter>(textRangeProvider))
    {
        target = static_cast<CTextRangeAdapter*>((static_cast<TextRangeAdapter*>(textRangeProvider))->GetHandle());
        IFC_RETURN(CoreImports::TextRangeAdapter_MoveEndpointByRange(
            static_cast<CTextRangeAdapter*>(GetHandle()),
            (UIAXcp::TextPatternRangeEndpoint)endpoint,
            target,
            (UIAXcp::TextPatternRangeEndpoint)targetEndpoint));
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::SelectImpl()
{
    CValue resultValue;

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 13, CValue(), &resultValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::AddToSelectionImpl()
{
    CValue resultValue;

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 14, CValue(), &resultValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::RemoveFromSelectionImpl()
{
    CValue resultValue;

    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 15, CValue(), &resultValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::ScrollIntoViewImpl(_In_ BOOLEAN alignToTop)
{
    CValue resultValue;
    CValue argVal;

    argVal.SetBool(!!alignToTop);
    IFC_RETURN(CoreImports::GetTextRangeProviderValue(static_cast<CTextRangeAdapter*>(GetHandle()), 16, argVal, &resultValue));

    return S_OK;
}

_Check_return_ HRESULT TextRangeAdapter::GetChildrenImpl(
    _Out_ UINT* pReturnValueCount,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** pppReturnValue)
{
    HRESULT hr = S_OK;
    XINT32 nChildCount = 0;
    UINT nIndex = 0;
    DependencyObject *pChild = nullptr;
    CAutomationPeer** ppChildren = nullptr;
    xaml_automation_peers::IAutomationPeer *pAP = nullptr;
    xaml_automation::Provider::IIRawElementProviderSimple* pREPS = nullptr;
    xaml_automation::Provider::IIRawElementProviderSimple** pREPSChildren = nullptr;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFCPTR(pppReturnValue);
    IFCPTR(pReturnValueCount);
    *pppReturnValue = nullptr;
    *pReturnValueCount = 0;

    IFC(CoreImports::TextRangeAdapter_GetChildren(static_cast<CTextRangeAdapter*>(GetHandle()), &ppChildren, &nChildCount));

    if (nChildCount > 0)
    {
        pREPSChildren = static_cast<IIRawElementProviderSimple**>(CoTaskMemAlloc(sizeof(IIRawElementProviderSimple*) * (ULONG)nChildCount));
        IFCOOMFAILFAST(pREPSChildren);
        for (int i = 0; i < nChildCount; i++)
        {
            pREPSChildren[i] = nullptr;
        }
        for (int nCurrent = 0; nCurrent < nChildCount; nCurrent++)
        {
            IFC(pCore->GetPeer(ppChildren[nCurrent], &pChild));
    
            pAP = ctl::query_interface<xaml_automation_peers::IAutomationPeer>(pChild);
            if (pAP)
            {
                IFC(AutomationPeer::ProviderFromPeerStatic(pAP, &pREPS));
                pREPSChildren[nIndex] = pREPS;
                nIndex++;
                ctl::release_interface(pAP);
            }
            pAP = nullptr;
            ctl::release_interface(pChild);
        }
    }
    *pppReturnValue = pREPSChildren;
    pREPSChildren = nullptr;
    *pReturnValueCount = nIndex;

Cleanup:
    if (ppChildren)
    {
        for (int32_t current = 0; current < nChildCount; current++)
        {
            ReleaseInterface(ppChildren[current]);
        }
    }
    delete[] ppChildren;    // we can just delete this since it is a common heap
    if (hr != S_OK && pREPSChildren)
    {
        for (int32_t current = 0; current < nChildCount; current++)
        {
            ReleaseInterface(pREPSChildren[current]);
        }
    }
    ctl::release_interface(pChild);
    ReleaseInterface(pAP);
    return hr;
}
