// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyTime.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new KeyTime from the specified TimeSpan.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
KeyTimeFactory::FromTimeSpan(
    _In_ wf::TimeSpan timeSpan,
    _Out_ xaml_animation::KeyTime* pReturnValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(pReturnValue);

    if (timeSpan.Duration < 0)
    {
        RRETURN(E_INVALIDARG);
    }

    pReturnValue->TimeSpan = timeSpan;

Cleanup:
    RRETURN(hr);
}

