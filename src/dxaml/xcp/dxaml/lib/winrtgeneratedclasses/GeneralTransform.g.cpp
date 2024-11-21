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

#include "GeneralTransform.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::GeneralTransform::GeneralTransform()
{
}

DirectUI::GeneralTransform::~GeneralTransform()
{
}

HRESULT DirectUI::GeneralTransform::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::GeneralTransform)))
    {
        *ppObject = static_cast<DirectUI::GeneralTransform*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IGeneralTransform)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IGeneralTransform*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IGeneralTransformOverrides)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IGeneralTransformOverrides*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::GeneralTransform::get_Inverse(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IGeneralTransform** ppValue)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Media::IGeneralTransformOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->get_InverseCore(ppValue));
    }
    else
    {
        IFC(get_InverseCore(ppValue));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::GeneralTransform::get_InverseCore(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IGeneralTransform** ppValue)
{
    // This method is abstract.
    RRETURN(E_NOTIMPL);
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::GeneralTransform::TransformBounds(ABI::Windows::Foundation::Rect rect, _Out_ ABI::Windows::Foundation::Rect* pReturnValue)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Media::IGeneralTransformOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->TransformBoundsCore(rect, pReturnValue));
    }
    else
    {
        IFC(TransformBoundsCore(rect, pReturnValue));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::GeneralTransform::TransformBoundsCore(ABI::Windows::Foundation::Rect rect, _Out_ ABI::Windows::Foundation::Rect* pReturnValue)
{
    // This method is abstract.
    RRETURN(E_NOTIMPL);
}
IFACEMETHODIMP DirectUI::GeneralTransform::TransformPoint(ABI::Windows::Foundation::Point point, _Out_ ABI::Windows::Foundation::Point* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "GeneralTransform_TransformPoint", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    *pReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<GeneralTransform*>(this)->TransformPointImpl(point, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "GeneralTransform_TransformPoint", hr);
    }
    RRETURN(hr);
}
_Check_return_ HRESULT DirectUI::GeneralTransform::TransformXY(DOUBLE x, DOUBLE y, _Out_ ABI::Windows::Foundation::Point* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "GeneralTransform_TransformXY", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    *pReturnValue={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "GeneralTransform_TransformXY", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::GeneralTransform::TryTransform(ABI::Windows::Foundation::Point inPoint, _Out_ ABI::Windows::Foundation::Point* pOutPoint, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Media::IGeneralTransformOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->TryTransformCore(inPoint, pOutPoint, pReturnValue));
    }
    else
    {
        IFC(TryTransformCore(inPoint, pOutPoint, pReturnValue));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::GeneralTransform::TryTransformCore(ABI::Windows::Foundation::Point inPoint, _Out_ ABI::Windows::Foundation::Point* pOutPoint, _Out_ BOOLEAN* pReturnValue)
{
    // This method is abstract.
    RRETURN(E_NOTIMPL);
}

HRESULT DirectUI::GeneralTransformFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::IGeneralTransformFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::IGeneralTransformFactory*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableAbstractCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::GeneralTransformFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Media::IGeneralTransform** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Media::IGeneralTransform);
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

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_GeneralTransform()
    {
        RRETURN(ctl::ActivationFactoryCreator<GeneralTransformFactory>::CreateActivationFactory());
    }
}
