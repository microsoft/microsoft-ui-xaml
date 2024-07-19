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

#include "ParallelTimeline.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::ParallelTimeline::ParallelTimeline()
{
}

DirectUI::ParallelTimeline::~ParallelTimeline()
{
}

HRESULT DirectUI::ParallelTimeline::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::ParallelTimeline)))
    {
        *ppObject = static_cast<DirectUI::ParallelTimeline*>(this);
    }
    else
    {
        RRETURN(DirectUI::Timeline::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.


