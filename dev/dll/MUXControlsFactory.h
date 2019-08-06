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

CppWinRTActivatableClassWithFactory(XamlMetadataProvider, MUXControlsFactory)

// This class is a breadcrumb to help us detect if we're running in a framework package.
// The way this works is that our WinMD does not contain this type so attempting to activate it
// will fail. However our framework package AppX contains an activatable class registration for
// it, so code that tries to activate it will succeed in that context.

struct FrameworkPackageDetectorFactory :
    public winrt::implements<FrameworkPackageDetectorFactory, winrt::IActivationFactory>
{
    static PCWSTR RuntimeClassName()
    {
        return L"Microsoft.UI.Xaml.Controls.Internal.FrameworkPackageDetector";
    }
    hstring GetRuntimeClassName() const
    {
        return RuntimeClassName();
    }

    winrt::IInspectable ActivateInstance() const
    {
        throw winrt::hresult_not_implemented();
    }
};

// Add a registration for this placeholder factory -- not using the helper macros because we don't actually
// list this type in IDL anywhere so the other macros don't work.
CppWinRTInternalWrlCreateCreatorMap(\
    FrameworkPackageDetector, \
    reinterpret_cast<const IID*>(&FrameworkPackageDetectorFactory::RuntimeClassName),\
    &CppWinRTTemp::GetTrustLevel_BaseTrust, \
    CppWinRTCreateActivationFactory<FrameworkPackageDetectorFactory>, \
    "minATL$__r")