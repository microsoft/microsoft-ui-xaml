// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlMetadataProvider.h"
#include "MUXControlsFactory.h"
#include "RevealBrush.h"

bool MUXControlsFactory::s_initialized{ false };

void MUXControlsFactory::EnsureInitialized()
{
    if (!s_initialized)
    {
        // Need to register here the DPs of types which are not referenced in our XAML but whose attached properties are.
        if (SharedHelpers::IsXamlCompositionBrushBaseAvailable())
        {
            // These are only needed on RS2+.
            RevealBrush::EnsureProperties();
        }
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
