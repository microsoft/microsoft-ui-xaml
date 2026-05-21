// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class XamlResourcePropertyBagOverrider
    {
        typedef void (WINAPI * PfnOverrideXamlResourcePropertyBag)(_In_opt_ std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>>*);

        PfnOverrideXamlResourcePropertyBag pfnOverrideXamlResourcePropertyBag;
        HMODULE hModuleDXaml;

    public:
        XamlResourcePropertyBagOverrider(std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>>* map)
        {
            hModuleDXaml = LoadLibraryEx(
                L"Microsoft.UI.Xaml.dll",
                nullptr, // reserved
                0); // no special flags
            WEX::Common::Throw::IfNull(hModuleDXaml, L"Failed to load Microsoft.UI.Xaml.dll");

            pfnOverrideXamlResourcePropertyBag =
                reinterpret_cast<PfnOverrideXamlResourcePropertyBag>(GetProcAddress(hModuleDXaml, "OverrideXamlResourcePropertyBag"));

            if (pfnOverrideXamlResourcePropertyBag)
            {
                pfnOverrideXamlResourcePropertyBag(map);
            }
            else
            {
                WEX::Common::Throw::LastError(L"GetProcAddress failed for OverrideXamlResourcePropertyBag");
            }
        }

        ~XamlResourcePropertyBagOverrider()
        {
            pfnOverrideXamlResourcePropertyBag(nullptr);
            FreeLibrary(hModuleDXaml);
        }
    };

} } } } }
