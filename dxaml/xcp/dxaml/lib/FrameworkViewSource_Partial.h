// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FrameworkViewSource.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(FrameworkViewSource)
    {
    public:
        IFACEMETHOD(CreateView)(_Outptr_ wac::IFrameworkView** frameworkView) override;
    };
}
