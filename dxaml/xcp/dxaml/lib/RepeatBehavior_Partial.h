// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class RepeatBehaviorHelper
    {
        friend class RepeatBehaviorFactory;
        friend class CValueBoxer;

    private:

        static _Check_return_ HRESULT GetForever(_Out_opt_ xaml_animation::RepeatBehavior *pValue);
    };
}
