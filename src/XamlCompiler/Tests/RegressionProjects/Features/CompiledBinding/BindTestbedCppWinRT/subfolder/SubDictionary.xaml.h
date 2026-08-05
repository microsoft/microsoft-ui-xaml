// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "subfolder.SubDictionary.g.h"

namespace winrt::BindTestbed::subfolder::implementation
{
    struct SubDictionary : SubDictionaryT<SubDictionary>
    {
        SubDictionary();
    };
}

namespace winrt::BindTestbed::subfolder::factory_implementation
{
    struct SubDictionary : SubDictionaryT<SubDictionary, implementation::SubDictionary>
    {
    };
}
