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

#include "PathGeometry.g.h"
#include "PathFigureCollection.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PathGeometry::PathGeometry()
{
}

DirectUI::PathGeometry::~PathGeometry()
{
}

HRESULT DirectUI::PathGeometry::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PathGeometry)))
    {
        *ppObject = static_cast<DirectUI::PathGeometry*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IPathGeometry)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IPathGeometry*>(this);
    }
    else
    {
        RRETURN(DirectUI::Geometry::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::PathGeometry::get_Figures(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::PathFigure*>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PathGeometry_Figures, ppValue));
}
IFACEMETHODIMP DirectUI::PathGeometry::put_Figures(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Media::PathFigure*>* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PathGeometry_Figures, pValue));
}
IFACEMETHODIMP DirectUI::PathGeometry::get_FillRule(_Out_ ABI::Microsoft::UI::Xaml::Media::FillRule* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::PathGeometry_FillRule, pValue));
}
IFACEMETHODIMP DirectUI::PathGeometry::put_FillRule(_In_ ABI::Microsoft::UI::Xaml::Media::FillRule value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::PathGeometry_FillRule, value));
}

// Events.

// Methods.

HRESULT DirectUI::PathGeometryFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IPathGeometryStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IPathGeometryStatics*>(this);
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
IFACEMETHODIMP DirectUI::PathGeometryFactory::get_FillRuleProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PathGeometry_FillRule, ppValue));
}
IFACEMETHODIMP DirectUI::PathGeometryFactory::get_FiguresProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::PathGeometry_Figures, ppValue));
}

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PathGeometry()
    {
        RRETURN(ctl::ActivationFactoryCreator<PathGeometryFactory>::CreateActivationFactory());
    }
}
