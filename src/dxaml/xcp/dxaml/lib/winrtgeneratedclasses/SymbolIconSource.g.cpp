// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "SymbolIconSource.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::SymbolIconSourceGenerated::SymbolIconSourceGenerated()
{
}

DirectUI::SymbolIconSourceGenerated::~SymbolIconSourceGenerated()
{
}

HRESULT DirectUI::SymbolIconSourceGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::SymbolIconSource)))
    {
        *ppObject = static_cast<DirectUI::SymbolIconSource*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSource)))
    {
        *ppObject = ctl::interface_cast<ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSource>(this);
    }
    else
    {
        RRETURN(DirectUI::IconSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
_Check_return_ HRESULT STDMETHODCALLTYPE DirectUI::SymbolIconSourceGenerated::get_Symbol(_Out_ ABI::Microsoft::UI::Xaml::Controls::Symbol* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::SymbolIconSource_Symbol, pValue));
}
_Check_return_ HRESULT STDMETHODCALLTYPE DirectUI::SymbolIconSourceGenerated::put_Symbol(ABI::Microsoft::UI::Xaml::Controls::Symbol value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::SymbolIconSource_Symbol, value));
}

// Events.

// Methods.

HRESULT DirectUI::SymbolIconSourceFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSourceFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSourceFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSourceStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSourceStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::SymbolIconSourceFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSource** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISymbolIconSource);
    const GUID metadataAPIGUID = MetadataAPI::GetClassInfoByIndex(GetTypeIndex())->GetGuid();
    const KnownTypeIndex typeIndex = GetTypeIndex();

    if(uuidofGUID != metadataAPIGUID)
    {
        XAML_FAIL_FAST();
    }
#endif

    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::SymbolIconSourceFactory::get_SymbolProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SymbolIconSource_Symbol, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_SymbolIconSource()
    {
        RRETURN(ctl::ActivationFactoryCreator<SymbolIconSourceFactory>::CreateActivationFactory());
    }
}
