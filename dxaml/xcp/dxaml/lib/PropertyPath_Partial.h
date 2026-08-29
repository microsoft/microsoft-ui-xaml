// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PropertyPath.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(PropertyPath)
    {
    public:

        static _Check_return_ HRESULT CreateInstance(_In_ HSTRING hPath, _Outptr_ PropertyPath **ppPropertyPath);
        
    };
}
