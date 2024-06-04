// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class DurationHelper
    {
        friend class DurationFactory;
        friend class CValueBoxer;

    private:

        static _Check_return_ HRESULT GetAutomatic(_Out_opt_ xaml::Duration *pValue);
        static _Check_return_ HRESULT GetForever(_Out_opt_ xaml::Duration *pValue);

        static _Check_return_ HRESULT GreaterThan(_In_ xaml::Duration target, _In_ xaml::Duration duration, _Out_ bool* pReturnValue);
        static _Check_return_ HRESULT LessThan(_In_ xaml::Duration target, _In_ xaml::Duration duration, _Out_ bool* pReturnValue);
    };
}
