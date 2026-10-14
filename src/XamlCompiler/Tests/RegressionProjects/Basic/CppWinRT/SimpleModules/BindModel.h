// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "Models/BindModel.g.h"

namespace winrt::Simple::Models::implementation
{
    struct BindModel : BindModelT<BindModel>
    {
        BindModel();

        Simple::Models::BindItem CurrentItem() const noexcept
        {
            return m_currentItem;
        }

    private:
        Simple::Models::BindItem m_currentItem{ nullptr };
    };
}

namespace winrt::Simple::Models::factory_implementation
{
    struct BindModel : BindModelT<BindModel, implementation::BindModel>
    {
    };
}
