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

#include "GridLength.g.h"

using namespace DirectUI;

HRESULT DirectUI::GridLengthFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::IGridLengthHelperStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::IGridLengthHelperStatics*>(this);
    }
    else
    {
        RRETURN(ctl::AbstractActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_GridLength()
    {
        RRETURN(ctl::ActivationFactoryCreator<GridLengthFactory>::CreateActivationFactory());
    }
}
