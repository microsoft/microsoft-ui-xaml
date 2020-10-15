// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlMetadataProvider.h"
#include "XamlControlsXamlMetaDataProvider.g.h"

class MUXControlsFactory :
    public winrt::factory_implementation::XamlControlsXamlMetaDataProviderT<MUXControlsFactory, XamlMetadataProvider>
{
public:

    static void EnsureInitialized();

    static void VerifyInitialized();

    static bool IsInitialized()
    {
        return s_initialized;
    }

private:
    static bool s_initialized;
};

namespace winrt::Microsoft::UI::Xaml::XamlTypeInfo
{
    namespace factory_implementation { using XamlControlsXamlMetaDataProvider = MUXControlsFactory; }
    namespace implementation { using XamlControlsXamlMetaDataProvider = XamlMetadataProvider; }
}

