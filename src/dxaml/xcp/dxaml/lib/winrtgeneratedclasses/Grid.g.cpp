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

#include "Grid.g.h"
#include "Brush.g.h"
#include "ColumnDefinitionCollection.g.h"
#include "RowDefinitionCollection.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::Grid::Grid()
{
}

DirectUI::Grid::~Grid()
{
}

HRESULT DirectUI::Grid::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::Grid)))
    {
        *ppObject = static_cast<DirectUI::Grid*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IGrid)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IGrid*>(this);
    }
    else
    {
        RRETURN(DirectUI::Panel::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::Grid::get_BackgroundSizing(_Out_ ABI::Microsoft::UI::Xaml::Controls::BackgroundSizing* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_BackgroundSizing, pValue));
}
IFACEMETHODIMP DirectUI::Grid::put_BackgroundSizing(ABI::Microsoft::UI::Xaml::Controls::BackgroundSizing value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_BackgroundSizing, value));
}
IFACEMETHODIMP DirectUI::Grid::get_BorderBrush(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IBrush** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_BorderBrush, ppValue));
}
IFACEMETHODIMP DirectUI::Grid::put_BorderBrush(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IBrush* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_BorderBrush, pValue));
}
IFACEMETHODIMP DirectUI::Grid::get_BorderThickness(_Out_ ABI::Microsoft::UI::Xaml::Thickness* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_BorderThickness, pValue));
}
IFACEMETHODIMP DirectUI::Grid::put_BorderThickness(ABI::Microsoft::UI::Xaml::Thickness value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_BorderThickness, value));
}
IFACEMETHODIMP DirectUI::Grid::get_ColumnDefinitions(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Controls::ColumnDefinition*>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_ColumnDefinitions, ppValue));
}
IFACEMETHODIMP DirectUI::Grid::get_ColumnSpacing(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_ColumnSpacing, pValue));
}
IFACEMETHODIMP DirectUI::Grid::put_ColumnSpacing(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_ColumnSpacing, value));
}
IFACEMETHODIMP DirectUI::Grid::get_CornerRadius(_Out_ ABI::Microsoft::UI::Xaml::CornerRadius* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_CornerRadius, pValue));
}
IFACEMETHODIMP DirectUI::Grid::put_CornerRadius(ABI::Microsoft::UI::Xaml::CornerRadius value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_CornerRadius, value));
}
IFACEMETHODIMP DirectUI::Grid::get_Padding(_Out_ ABI::Microsoft::UI::Xaml::Thickness* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_Padding, pValue));
}
IFACEMETHODIMP DirectUI::Grid::put_Padding(ABI::Microsoft::UI::Xaml::Thickness value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_Padding, value));
}
IFACEMETHODIMP DirectUI::Grid::get_RowDefinitions(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Controls::RowDefinition*>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_RowDefinitions, ppValue));
}
IFACEMETHODIMP DirectUI::Grid::get_RowSpacing(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Grid_RowSpacing, pValue));
}
IFACEMETHODIMP DirectUI::Grid::put_RowSpacing(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Grid_RowSpacing, value));
}

// Events.

// Methods.

HRESULT DirectUI::GridFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IGridFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IGridFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IGridStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IGridStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::GridFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IGrid** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::IGrid);
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


IFACEMETHODIMP DirectUI::GridFactory::get_BackgroundSizingProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_BackgroundSizing, ppValue));
}
IFACEMETHODIMP DirectUI::GridFactory::get_BorderBrushProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_BorderBrush, ppValue));
}
IFACEMETHODIMP DirectUI::GridFactory::get_BorderThicknessProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_BorderThickness, ppValue));
}
IFACEMETHODIMP DirectUI::GridFactory::get_CornerRadiusProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_CornerRadius, ppValue));
}
IFACEMETHODIMP DirectUI::GridFactory::get_PaddingProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_Padding, ppValue));
}
IFACEMETHODIMP DirectUI::GridFactory::get_RowSpacingProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_RowSpacing, ppValue));
}
IFACEMETHODIMP DirectUI::GridFactory::get_ColumnSpacingProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_ColumnSpacing, ppValue));
}

// Attached properties.
_Check_return_ HRESULT DirectUI::GridFactory::GetRowStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_Row, pValue));
}

_Check_return_ HRESULT DirectUI::GridFactory::SetRowStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_Row, value));
}


IFACEMETHODIMP DirectUI::GridFactory::get_RowProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_Row, ppValue));
}


IFACEMETHODIMP DirectUI::GridFactory::GetRow(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(GetRowStatic(pElement, pValue));
}

IFACEMETHODIMP DirectUI::GridFactory::SetRow(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(SetRowStatic(pElement, value));
}
_Check_return_ HRESULT DirectUI::GridFactory::GetColumnStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_Column, pValue));
}

_Check_return_ HRESULT DirectUI::GridFactory::SetColumnStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_Column, value));
}


IFACEMETHODIMP DirectUI::GridFactory::get_ColumnProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_Column, ppValue));
}


IFACEMETHODIMP DirectUI::GridFactory::GetColumn(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(GetColumnStatic(pElement, pValue));
}

IFACEMETHODIMP DirectUI::GridFactory::SetColumn(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(SetColumnStatic(pElement, value));
}
_Check_return_ HRESULT DirectUI::GridFactory::GetRowSpanStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_RowSpan, pValue));
}

_Check_return_ HRESULT DirectUI::GridFactory::SetRowSpanStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_RowSpan, value));
}


IFACEMETHODIMP DirectUI::GridFactory::get_RowSpanProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_RowSpan, ppValue));
}


IFACEMETHODIMP DirectUI::GridFactory::GetRowSpan(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(GetRowSpanStatic(pElement, pValue));
}

IFACEMETHODIMP DirectUI::GridFactory::SetRowSpan(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(SetRowSpanStatic(pElement, value));
}
_Check_return_ HRESULT DirectUI::GridFactory::GetColumnSpanStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_ColumnSpan, pValue));
}

_Check_return_ HRESULT DirectUI::GridFactory::SetColumnSpanStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::FrameworkElement*>(pElement), KnownPropertyIndex::Grid_ColumnSpan, value));
}


IFACEMETHODIMP DirectUI::GridFactory::get_ColumnSpanProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Grid_ColumnSpan, ppValue));
}


IFACEMETHODIMP DirectUI::GridFactory::GetColumnSpan(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, _Out_ INT* pValue)
{
    RRETURN(GetColumnSpanStatic(pElement, pValue));
}

IFACEMETHODIMP DirectUI::GridFactory::SetColumnSpan(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pElement, INT value)
{
    RRETURN(SetColumnSpanStatic(pElement, value));
}

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_Grid()
    {
        RRETURN(ctl::ActivationFactoryCreator<GridFactory>::CreateActivationFactory());
    }
}
