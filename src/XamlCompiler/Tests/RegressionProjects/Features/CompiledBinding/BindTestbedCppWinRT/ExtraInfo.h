// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "ExtraInfo.g.h"

namespace winrt::BindTestbed::implementation
{
    struct ExtraInfo : ExtraInfoT<ExtraInfo>
    {
        ExtraInfo()
        {}

        ExtraInfo(hstring caption)
        {
            Caption(caption);
        }

        hstring Caption() { return caption; }
        void Caption(hstring value) { caption = value; }

    private:
        hstring caption{};
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct ExtraInfo : ExtraInfoT<ExtraInfo, implementation::ExtraInfo>
    {
    };
}
