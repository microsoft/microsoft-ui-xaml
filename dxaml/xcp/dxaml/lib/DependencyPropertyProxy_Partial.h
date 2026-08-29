// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DependencyPropertyProxy.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DependencyPropertyProxy)
    {
    public:
        static _Check_return_ HRESULT CreateObject(_In_ xaml::IDependencyProperty* pDP, _Outptr_ DependencyPropertyProxy** ppProxy);

    public:
        _Check_return_ HRESULT GetWrappedDependencyProperty(_Outptr_ const CDependencyProperty** ppDP);
    };
}
