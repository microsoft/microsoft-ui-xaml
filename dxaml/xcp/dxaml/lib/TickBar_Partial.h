// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TickBar.g.h"

#define MIN_TICKMARK_GAP 20

namespace DirectUI
{
    // Represents the TickBar
    PARTIAL_CLASS(TickBar)
    {
    public:
        // Initializes a new instance of the TickBar class.
        TickBar();
        
        // Destroys an instance of the TickBar class.
        ~TickBar() override;

        // Arranges the items of a TickBar.
        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size finalSize,
            _Out_ wf::Size* returnValue)
            override;

    protected:
        // Overrides
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

    private:
        // Propagates the Fill brush to existing tick marks.
        _Check_return_ HRESULT PropagateFill();
    };
}
