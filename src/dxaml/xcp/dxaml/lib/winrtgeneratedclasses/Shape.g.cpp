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

#include "Shape.g.h"
#include "Brush.g.h"
#include "DoubleCollection.g.h"
#include "Transform.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::Shape::Shape()
{
}

DirectUI::Shape::~Shape()
{
}

HRESULT DirectUI::Shape::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::Shape)))
    {
        *ppObject = static_cast<DirectUI::Shape*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Shapes::IShape)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Shapes::IShape*>(this);
    }
    else
    {
        RRETURN(DirectUI::FrameworkElement::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::Shape::get_Fill(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IBrush** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, ppValue));
}
IFACEMETHODIMP DirectUI::Shape::put_Fill(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IBrush* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pValue));
}
IFACEMETHODIMP DirectUI::Shape::get_GeometryTransform(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::ITransform** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_GeometryTransform, ppValue));
}
IFACEMETHODIMP DirectUI::Shape::get_Stretch(_Out_ ABI::Microsoft::UI::Xaml::Media::Stretch* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_Stretch, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_Stretch(ABI::Microsoft::UI::Xaml::Media::Stretch value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_Stretch, value));
}
IFACEMETHODIMP DirectUI::Shape::get_Stroke(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IBrush** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, ppValue));
}
IFACEMETHODIMP DirectUI::Shape::put_Stroke(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IBrush* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pValue));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeDashArray(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<DOUBLE>** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeDashArray, ppValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeDashArray(_In_opt_ ABI::Windows::Foundation::Collections::IVector<DOUBLE>* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeDashArray, pValue));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeDashCap(_Out_ ABI::Microsoft::UI::Xaml::Media::PenLineCap* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeDashCap, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeDashCap(ABI::Microsoft::UI::Xaml::Media::PenLineCap value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeDashCap, value));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeDashOffset(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeDashOffset, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeDashOffset(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeDashOffset, value));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeEndLineCap(_Out_ ABI::Microsoft::UI::Xaml::Media::PenLineCap* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeEndLineCap, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeEndLineCap(ABI::Microsoft::UI::Xaml::Media::PenLineCap value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeEndLineCap, value));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeLineJoin(_Out_ ABI::Microsoft::UI::Xaml::Media::PenLineJoin* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeLineJoin, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeLineJoin(ABI::Microsoft::UI::Xaml::Media::PenLineJoin value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeLineJoin, value));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeMiterLimit(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeMiterLimit, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeMiterLimit(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeMiterLimit, value));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeStartLineCap(_Out_ ABI::Microsoft::UI::Xaml::Media::PenLineCap* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeStartLineCap, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeStartLineCap(ABI::Microsoft::UI::Xaml::Media::PenLineCap value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeStartLineCap, value));
}
IFACEMETHODIMP DirectUI::Shape::get_StrokeThickness(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, pValue));
}
IFACEMETHODIMP DirectUI::Shape::put_StrokeThickness(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, value));
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::Shape::GetAlphaMask(_Outptr_ ABI::Microsoft::UI::Composition::ICompositionBrush** ppReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "Shape_GetAlphaMask", 0);
    }
    ARG_VALIDRETURNPOINTER(ppReturnValue);
    *ppReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<Shape*>(this)->GetAlphaMaskImpl(ppReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "Shape_GetAlphaMask", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::ShapeFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Shapes::IShapeFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Shapes::IShapeFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Shapes::IShapeStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Shapes::IShapeStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableAbstractCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::ShapeFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Shapes::IShape** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Shapes::IShape);
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
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableAbstractCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::ShapeFactory::get_FillProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_Fill, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_Stroke, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeMiterLimitProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeMiterLimit, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeThicknessProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeThickness, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeStartLineCapProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeStartLineCap, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeEndLineCapProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeEndLineCap, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeLineJoinProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeLineJoin, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeDashOffsetProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeDashOffset, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeDashCapProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeDashCap, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StrokeDashArrayProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_StrokeDashArray, ppValue));
}
IFACEMETHODIMP DirectUI::ShapeFactory::get_StretchProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Shape_Stretch, ppValue));
}


// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_Shape()
    {
        RRETURN(ctl::ActivationFactoryCreator<ShapeFactory>::CreateActivationFactory());
    }
}
