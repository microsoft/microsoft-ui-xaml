// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Helper to safely grab a framework peer out of a PropertyChangedParams struct.

#pragma once

namespace DirectUI
{
    class PropertyChangedParamsHelper
    {
    public:
        static _Check_return_ HRESULT GetObjects(_In_ const PropertyChangedParams& args, _Out_ IInspectable** ppOldValue, _Out_ IInspectable** ppNewValue);
    };
}
