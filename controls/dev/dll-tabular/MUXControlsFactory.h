// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlMetadataProvider.h"
#include "XamlControlsTabularXamlMetaDataProvider.g.h"

class MUXControlsFactory :
    public winrt::factory_implementation::XamlControlsTabularXamlMetaDataProviderT<MUXControlsFactory, XamlMetadataProvider>
{
public:

    static void EnsureInitialized();

    static void VerifyInitialized();

    static bool IsInitialized()
    {
        return s_initialized;
    }

    static void Deinitialize()
    {
        s_initialized = false;
    }

private:
    static bool s_initialized;
};

namespace winrt::Microsoft::UI::Xaml::XamlTypeInfo
{
    namespace factory_implementation { using XamlControlsTabularXamlMetaDataProvider = MUXControlsFactory; }
    namespace implementation { using XamlControlsTabularXamlMetaDataProvider = XamlMetadataProvider; }
}