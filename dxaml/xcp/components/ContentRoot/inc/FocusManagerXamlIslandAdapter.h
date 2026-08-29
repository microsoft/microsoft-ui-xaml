// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FocusAdapter.h"

class CContentRoot;

namespace ContentRootAdapters
{
    class FocusManagerXamlIslandAdapter : public FocusAdapter
    {
    public:
        FocusManagerXamlIslandAdapter(_In_ CContentRoot& contentRoot);
        void SetFocus() final;
        bool ShouldDepartFocus(_In_ DirectUI::FocusNavigationDirection direction) const final;
    };
}