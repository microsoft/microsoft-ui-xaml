// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    class JumpListHelper
    {
        public:
            _Check_return_ static HRESULT HasItems(_In_opt_ IInspectable *pGroup, _Out_ BOOLEAN *pResult);
    };
}
