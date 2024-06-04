// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FocusAdapter.h"

class CContentRoot;
class CCoreServices;

namespace ContentRootAdapters
{
    class FocusManagerCoreWindowAdapter : public FocusAdapter
    {
    public:
        FocusManagerCoreWindowAdapter(_In_ CContentRoot& contentRoot);
        void SetFocus() final;
    };
}