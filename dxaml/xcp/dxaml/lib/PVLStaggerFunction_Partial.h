// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PVLStaggerFunction.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(PVLStaggerFunction)
    {
    protected:
        _Check_return_ HRESULT GetTransitionDelays(_In_ wfc::IVector<xaml::DependencyObject*>* staggerItems) override;
    };
}

