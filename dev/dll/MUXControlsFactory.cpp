// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlMetadataProvider.h"
#include "MUXControlsFactory.h"

#ifdef MATERIALS_INCLUDED
#include "RevealBrush.h"
#endif

#include "XamlControlsXamlMetaDataProvider.g.cpp"

#include "FrameworkPackageDetector.g.h"
namespace winrt::Microsoft::UI::Private::Controls
{
    // Provide stub implementations to satisfy the C++/WinRT exports.
    namespace implementation
    {
        struct FrameworkPackageDetector : FrameworkPackageDetectorT<FrameworkPackageDetector>
        {
        };
    }
    namespace factory_implementation
    {
        struct FrameworkPackageDetector : FrameworkPackageDetectorT<FrameworkPackageDetector, winrt::Microsoft::UI::Private::Controls::implementation::FrameworkPackageDetector>
        {
        };
    }
}
#include "FrameworkPackageDetector.g.cpp"


bool MUXControlsFactory::s_initialized{ false };

void MUXControlsFactory::EnsureInitialized()
{
    if (!s_initialized)
    {
        // Need to register here the DPs of types which are not referenced in our XAML but whose attached properties are.
#ifdef MATERIALS_INCLUDED
        if (SharedHelpers::IsXamlCompositionBrushBaseAvailable())
        {
            // These are only needed on RS2+.
            RevealBrush::EnsureProperties();
        }
#endif
        s_initialized = true;
    }
}

void MUXControlsFactory::VerifyInitialized()
{
    if (!s_initialized)
    {
        throw winrt::hresult_error(E_FAIL, L"ERROR: You must put an instance of " MUXCONTROLSROOT_NAMESPACE_STR ".XamlControlsResources in your Application.Resources.MergedDictionaries.");
    }
}
