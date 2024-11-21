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

#include "PointHelper.g.h"
#include "XamlTelemetry.h"


HRESULT DirectUI::PointHelperFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::IPointHelperStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::IPointHelperStatics*>(this);
    }
    else
    {
        RRETURN(ctl::AbstractActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.
IFACEMETHODIMP DirectUI::PointHelperFactory::FromCoordinates(FLOAT x, FLOAT y, _Out_ ABI::Windows::Foundation::Point* pReturnValue)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, 0, "PointHelper_FromCoordinates", 0);
    }
    ARG_VALIDRETURNPOINTER(pReturnValue);
    *pReturnValue={};
    IFC(CheckActivationAllowed());
    IFC(FromCoordinatesImpl(x, y, pReturnValue));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, 0, "PointHelper_FromCoordinates", hr);
    }
    RRETURN(hr);
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_PointHelper()
    {
        RRETURN(ctl::ActivationFactoryCreator<PointHelperFactory>::CreateActivationFactory());
    }
}
