// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TestPoolFilter.h"

namespace Private { namespace Infrastructure {

    class WuxLoadedTestPoolFilter : public TestPoolFilter
    {
    public:
        const wchar_t* GetName() const override { return L"WuxLoadedTestPoolFilter"; }
        bool IsDirty() override;
    };

} }
