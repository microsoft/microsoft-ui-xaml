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

#include "QuadraticEase.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::QuadraticEase::QuadraticEase()
{
}

DirectUI::QuadraticEase::~QuadraticEase()
{
}

HRESULT DirectUI::QuadraticEase::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::QuadraticEase)))
    {
        *ppObject = static_cast<DirectUI::QuadraticEase*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Media::Animation::IQuadraticEase)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Media::Animation::IQuadraticEase*>(this);
    }
    else
    {
        RRETURN(DirectUI::EasingFunctionBase::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.

// Factory methods.

// Dependency properties.

// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_QuadraticEase()
    {
        RRETURN(ctl::ActivationFactoryCreator<QuadraticEaseFactory>::CreateActivationFactory());
    }
}
