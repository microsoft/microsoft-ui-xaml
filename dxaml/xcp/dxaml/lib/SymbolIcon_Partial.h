// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SymbolIcon.g.h"

namespace DirectUI
{
    // Represents the SymbolIcon control
    PARTIAL_CLASS(SymbolIcon)
    {
        public:
            // Initializes a new instance of the SymbolIcon class.
            SymbolIcon();
            ~SymbolIcon() override;

            // Apply a template to the icon.
            IFACEMETHOD(OnApplyTemplate)() override;

            IFACEMETHOD(put_Symbol)(_In_ xaml_controls::Symbol value) override;

            void SetFontSize(_In_ float fontSize);
    };
}
