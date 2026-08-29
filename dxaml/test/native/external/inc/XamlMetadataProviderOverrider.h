// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <MetadataCleanup.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class XamlMetadataProviderOverrider
    {
        typedef void (WINAPI * PfnOverrideXamlMetadataProvider)(_In_opt_ Microsoft::UI::Xaml::Markup::IXamlMetadataProvider^);

        PfnOverrideXamlMetadataProvider pfnOverrideXamlMetadataProvider;
        HMODULE hModuleDXaml;

    public:
        XamlMetadataProviderOverrider(Microsoft::UI::Xaml::Markup::IXamlMetadataProvider^ pProvider)
        {
            hModuleDXaml = LoadLibraryEx(
                L"Microsoft.UI.Xaml.dll",
                nullptr, // reserved
                0); // no special flags
            WEX::Common::Throw::IfNull(hModuleDXaml, L"Failed to load Microsoft.UI.Xaml.dll");

            pfnOverrideXamlMetadataProvider =
                reinterpret_cast<PfnOverrideXamlMetadataProvider>(GetProcAddress(hModuleDXaml, "OverrideXamlMetadataProvider"));

            if (pfnOverrideXamlMetadataProvider)
            {
                _provider = pProvider;
                pfnOverrideXamlMetadataProvider(pProvider);
            }
            else
            {
                WEX::Common::Throw::LastError(L"GetProcAddress failed for OverrideXamlMetadataProvider");
            }
        }

        void Override()
        {
            pfnOverrideXamlMetadataProvider(_provider);
        }

        ~XamlMetadataProviderOverrider()
        {
            pfnOverrideXamlMetadataProvider(nullptr);
            FreeLibrary(hModuleDXaml);
        }

    private:
        MetadataCleanup _metadataCleanup;
        Microsoft::UI::Xaml::Markup::IXamlMetadataProvider^ _provider;
    };

} } } } }
