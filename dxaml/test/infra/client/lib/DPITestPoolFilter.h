// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TestPoolFilter.h"
#include <vector>

namespace Private { namespace Infrastructure {

class DPITestPoolFilter : public TestPoolFilter
{
public:
    HRESULT Initialize() override;
    const wchar_t* GetName() const override { return L"DPITestPoolFilter"; }
    bool IsDirty() override;
private:
    std::vector<LUID> m_displayAdapters;
};

} }

