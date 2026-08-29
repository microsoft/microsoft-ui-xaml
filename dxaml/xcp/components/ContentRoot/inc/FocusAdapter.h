// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <enumdefs.g.h>

class CContentRoot;

namespace ContentRootAdapters
{
    class FocusAdapter
    {
    public:
        FocusAdapter(_In_ CContentRoot& contentRoot);
        virtual ~FocusAdapter() = default;

        virtual void SetFocus() = 0;
        virtual bool ShouldDepartFocus(_In_ DirectUI::FocusNavigationDirection direction) const;

    protected:
        CContentRoot& m_contentRoot;
    };
}