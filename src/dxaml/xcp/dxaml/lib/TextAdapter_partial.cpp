// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextAdapter.g.h"
#include "TextRangeAdapter.g.h"
#include "AutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the TextAdapter class.
TextAdapter::TextAdapter()
    : m_pOwnerNoRef(nullptr)
{
}

// Destructor
TextAdapter::~TextAdapter()
{
    m_pOwnerNoRef = nullptr;
}

// this implementation is hidden from IDL
_Check_return_ HRESULT TextAdapter::QueryInterfaceImpl( _In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ITextAdapter)))
    {
        *ppObject = static_cast<ITextAdapter*>(this);
    }
    else
    {
        return TextAdapterGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

// Set the owner of Adapter
_Check_return_ HRESULT TextAdapter::put_Owner(_In_ xaml::IUIElement* pOwner)
{
    if(!m_pOwnerNoRef)
    {
        m_pOwnerNoRef = pOwner;
    }

    return S_OK;
}

// Invalidate owner, this is called by Owner' d'tor
void TextAdapter::InvalidateOwner()
{
    m_pOwnerNoRef = nullptr;
}

_Check_return_ HRESULT TextAdapter::get_DocumentRangeImpl(_Outptr_ xaml_automation::Provider::ITextRangeProvider** ppReturnValue)
{
    CValue resultValue;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;

    IFC_RETURN(CoreImports::GetTextProviderValue(static_cast<CTextAdapter*>(GetHandle()), 0, CValue(), &resultValue));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(
        &resultValue,
        MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextRangeAdapter),
        __uuidof(xaml_automation::Provider::ITextRangeProvider),
        reinterpret_cast<void**>(ppReturnValue)));

    return S_OK;
}

_Check_return_ HRESULT TextAdapter::get_SupportedTextSelectionImpl(_Out_ xaml_automation::SupportedTextSelection* pReturnValue)
{
    UINT enumValue = 0;
    CValue resultValue;

    IFCPTR_RETURN(pReturnValue);

    IFC_RETURN(CoreImports::GetTextProviderValue(static_cast<CTextAdapter*>(GetHandle()), 1, CValue(), &resultValue));
    IFC_RETURN(CValueBoxer::UnboxEnumValue(&resultValue, nullptr, &enumValue));
    *pReturnValue = (xaml_automation::SupportedTextSelection)enumValue;
    
    return S_OK;
}

_Check_return_ HRESULT TextAdapter::GetSelectionImpl(
    _Out_ UINT* pReturnValueCount,
    _Outptr_ xaml_automation::Provider::ITextRangeProvider*** pppReturnValue)
{
    HRESULT hr = S_OK;
    XINT32 nChildCount = 0;
    CTextRangeAdapter** ppChildren = nullptr;
    DependencyObject *pChild = nullptr;
    xaml_automation::Provider::ITextRangeProvider *pTextRangeProvider = nullptr;
    xaml_automation::Provider::ITextRangeProvider** ppReturnValue = nullptr;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFCPTR(pppReturnValue);
    IFCPTR(pReturnValueCount);
    *pppReturnValue = nullptr;
    *pReturnValueCount = 0;

    IFC(CoreImports::GetTextRangeArray(static_cast<CTextAdapter*>(GetHandle()), 2, &ppChildren, &nChildCount));
    if (nChildCount > 0)
    {
        ppReturnValue = static_cast<xaml_automation::Provider::ITextRangeProvider**>(CoTaskMemAlloc(sizeof(xaml_automation::Provider::ITextRangeProvider*) * (ULONG)nChildCount));
        IFCOOMFAILFAST(ppReturnValue);
        for(INT i = 0; i < nChildCount; i++)
        {
            ppReturnValue[i] = nullptr;
        }
        for (INT nCurrent = 0; nCurrent < nChildCount; nCurrent++)
        {
            IFC(pCore->GetPeer(ppChildren[nCurrent], &pChild));
    
            // See if this is a framework element
            // TODO: Check the type internally perhaps? 
            pTextRangeProvider = ctl::query_interface<xaml_automation::Provider::ITextRangeProvider>(pChild);
            ppReturnValue[nCurrent] = pTextRangeProvider;
            ctl::release_interface(pChild);
            pTextRangeProvider = nullptr;
        }
    }
    *pppReturnValue = ppReturnValue;
    ppReturnValue = nullptr;
    *pReturnValueCount = nChildCount;
       
Cleanup:
    if (ppChildren)
    {
        for (XINT32 current = 0; current < nChildCount; current++)
        {
            ReleaseInterface(ppChildren[current]);
        }
    }
    delete[] ppChildren;    // we can just delete this since it is a common heap
    if (hr != S_OK && ppReturnValue)
    {
        for (XINT32 current = 0; current < nChildCount; current++)
        {
            ReleaseInterface(ppReturnValue[current]);
        }
    }
    ctl::release_interface(pChild);
    ReleaseInterface(pTextRangeProvider);
    return hr;
}

_Check_return_ HRESULT TextAdapter::GetVisibleRangesImpl(
    _Out_ UINT* pReturnValueCount,
    _Outptr_ xaml_automation::Provider::ITextRangeProvider*** pppReturnValue)
{
    HRESULT hr = S_OK;
    XINT32 nChildCount = 0;
    CTextRangeAdapter** ppChildren = nullptr;
    DependencyObject *pChild = nullptr;
    xaml_automation::Provider::ITextRangeProvider *pTextRangeProvider = nullptr;
    xaml_automation::Provider::ITextRangeProvider** ppReturnValue = nullptr;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFCPTR(pppReturnValue);
    IFCPTR(pReturnValueCount);
    *pppReturnValue = nullptr;
    *pReturnValueCount = 0;
    
    IFC(CoreImports::GetTextRangeArray(static_cast<CTextAdapter*>(GetHandle()), 3, &ppChildren, &nChildCount));
    if (nChildCount > 0)
    {
        ppReturnValue = static_cast<xaml_automation::Provider::ITextRangeProvider**>(CoTaskMemAlloc(sizeof(xaml_automation::Provider::ITextRangeProvider*) * (ULONG)nChildCount));
        IFCOOMFAILFAST(ppReturnValue);
        for(INT i = 0; i < nChildCount; i++)
        {
            ppReturnValue[i] = nullptr;
        }
        for (INT nCurrent = 0; nCurrent < nChildCount; nCurrent++)
        {
            IFC(pCore->GetPeer(ppChildren[nCurrent], &pChild));
    
            // See if this is a framework element
            // TODO: Check the type internally perhaps? 
            pTextRangeProvider = ctl::query_interface<xaml_automation::Provider::ITextRangeProvider>(pChild);
            ppReturnValue[nCurrent] = pTextRangeProvider;
            pTextRangeProvider = nullptr;
            ctl::release_interface(pChild);
        }
    }
    *pppReturnValue = ppReturnValue;
    ppReturnValue = nullptr;
    *pReturnValueCount = nChildCount;
    
Cleanup:
    if (ppChildren)
    {
        for (XINT32 current = 0; current < nChildCount; current++)
        {
            ReleaseInterface(ppChildren[current]);
        }
    }
    delete[] ppChildren;    // we can just delete this since it is a common heap
    if (hr != S_OK && ppReturnValue)
    {
        for (XINT32 current = 0; current < nChildCount; current++)
        {
            ReleaseInterface(ppReturnValue[current]);
        }
    }
    ctl::release_interface(pChild);
    ReleaseInterface(pTextRangeProvider);
    return hr;
}

_Check_return_ HRESULT TextAdapter::RangeFromChildImpl(
    _In_ xaml_automation::Provider::IIRawElementProviderSimple* childElement,
    _COM_Outptr_ xaml_automation::Provider::ITextRangeProvider** ppReturnValue)
{
    CValue resultValue;
    CValue argVal;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> childAsAP;

    IFCPTR_RETURN(childElement);
    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;
    
    
    IFC_RETURN(AutomationPeer::PeerFromProviderStatic(childElement, &childAsAP));
    argVal.WrapObjectNoRef((static_cast<AutomationPeer*>(childAsAP.Get()))->GetHandle());

    IFC_RETURN(CoreImports::GetTextProviderValue(static_cast<CTextAdapter*>(GetHandle()), 4, argVal, &resultValue));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(
        &resultValue,
        MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextRangeAdapter),
        __uuidof(xaml_automation::Provider::ITextRangeProvider),
        reinterpret_cast<void**>(ppReturnValue)));

    return S_OK;
}

_Check_return_ HRESULT TextAdapter::RangeFromPointImpl(
    _In_ wf::Point screenLocation,
    _Outptr_ xaml_automation::Provider::ITextRangeProvider** ppReturnValue)
{
    CValue resultValue;
    XPOINTF point;
    CValue argVal;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = nullptr;
    point.x = screenLocation.X;
    point.y = screenLocation.Y;      
    argVal.WrapPoint(&point);

    IFC_RETURN(CoreImports::GetTextProviderValue(static_cast<CTextAdapter*>(GetHandle()), 5, argVal, &resultValue));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(
        &resultValue,
        MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextRangeAdapter),
        __uuidof(xaml_automation::Provider::ITextRangeProvider),
        reinterpret_cast<void**>(ppReturnValue)));

    return S_OK;
}   
