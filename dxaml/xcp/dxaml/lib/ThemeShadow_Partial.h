// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ThemeShadow.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ThemeShadow)
    {
    public:
        _Check_return_ HRESULT get_MaskImpl(_Outptr_result_maybenull_ WUComp::ICompositionBrush** ppValue);
        _Check_return_ HRESULT put_MaskImpl(_In_opt_ WUComp::ICompositionBrush* pValue);
    };
}
