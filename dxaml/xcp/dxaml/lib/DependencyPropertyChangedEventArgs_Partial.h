// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DependencyPropertyChangedEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DependencyPropertyChangedEventArgs)
    {
    public:
        DependencyPropertyChangedEventArgs();

        KnownPropertyIndex GetPropertyIndex();

        static _Check_return_ HRESULT Create(
            _In_ KnownPropertyIndex nPropertyIndex,
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue,
            _Outptr_ DependencyPropertyChangedEventArgs** ppArgs);

        _Check_return_ HRESULT get_PropertyImpl(_Outptr_ xaml::IDependencyProperty** pValue);

    protected:
        _Check_return_ HRESULT Initialize() override;

    private:
        KnownPropertyIndex m_nPropertyIndex;
    };
}
