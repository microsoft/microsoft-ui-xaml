// Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

#include "ThemeShadow.g.h"

namespace DirectUI
{
    class ThemeShadowFactory: public ThemeShadowFactoryGenerated
    {
    public:
        IFACEMETHOD(get_IsDropShadowModeImpl)(_Out_ BOOLEAN* pValue);
    };
}
