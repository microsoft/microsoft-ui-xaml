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

#include "GeometryGroup.g.h"
#include "GeometryCollection.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::GeometryGroup::GeometryGroup()
{
}

DirectUI::GeometryGroup::~GeometryGroup()
{
}

HRESULT DirectUI::GeometryGroup::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::GeometryGroup)))
    {
        *ppObject = static_cast<DirectUI::GeometryGroup*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IGeometryGroup)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IGeometryGroup*>(this);
    }
    else
    {
        RRETURN(DirectUI::Geometry::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::GeometryGroup::get_Children(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::Geometry*>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::GeometryGroup_Children, ppValue));
}
IFACEMETHODIMP DirectUI::GeometryGroup::put_Children(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::Geometry*>* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::GeometryGroup_Children, pValue));
}
IFACEMETHODIMP DirectUI::GeometryGroup::get_FillRule(_Out_ ABI::Microsoft::UI::Xaml::Media::FillRule* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::GeometryGroup_FillRule, pValue));
}
IFACEMETHODIMP DirectUI::GeometryGroup::put_FillRule(_In_ ABI::Microsoft::UI::Xaml::Media::FillRule value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::GeometryGroup_FillRule, value));
}

// Events.

// Methods.

HRESULT DirectUI::GeometryGroupFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IGeometryGroupStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IGeometryGroupStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.

// Dependency properties.
IFACEMETHODIMP DirectUI::GeometryGroupFactory::get_FillRuleProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::GeometryGroup_FillRule, ppValue));
}
IFACEMETHODIMP DirectUI::GeometryGroupFactory::get_ChildrenProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::GeometryGroup_Children, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_GeometryGroup()
    {
        RRETURN(ctl::ActivationFactoryCreator<GeometryGroupFactory>::CreateActivationFactory());
    }
}
