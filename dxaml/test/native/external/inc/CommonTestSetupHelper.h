// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <HostingModeOverride.h>
#include <MasterFileNameOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class CommonTestSetupHelper
    {
    public:
        static void CommonTestClassSetup(const WEX::Private::TestPropertyMetadata* classMetadata)
        {
            const wchar_t* mode = PropertyFromMetadata(classMetadata, L"Hosting:Mode");
            WEX::Common::Throw::IfFalse(mode != nullptr, E_INVALIDARG,
                L"Declare a hosting mode with TEST_CLASS_HOSTING_MODE_DEFAULT() or TEST_CLASS_HOSTING_MODE(...).");
            WEX::Common::Throw::IfFailed(::Private::Infrastructure::Hosting::SetDeclaredHostingMode(mode),
                L"Failed to set the class-declared hosting mode.");
            WEX::Common::Throw::IfFailed(::Private::Infrastructure::MasterFiles::SetClassName(
                PropertyFromMetadata(classMetadata, L"MasterFile:ClassName")),
                L"Failed to set the class name used for master files.");

            if (_wcsicmp(mode, L"WPF") == 0 && !_isWpfInitialized)
            {
                ConfigureWin32Host();
                _isWpfInitialized = true;
            }
            test_infra::TestServices::EnsureInitialized();
        }
    private:
        inline static bool _isWpfInitialized = false;

        static const wchar_t* PropertyFromMetadata(const WEX::Private::TestPropertyMetadata* metadata, const wchar_t* name)
        {
            const wchar_t* value = nullptr;
            for (auto property = metadata; property && (property->m_pszPropertyName || property->m_pszPropertyValue); ++property)
            {
                if (property->m_identifier == WEX::Private::TaefAbiStructIdentifier::TestClassMetadata
                    && property->m_pszPropertyName
                    && _wcsicmp(reinterpret_cast<const wchar_t*>(property->m_pszPropertyName), name) == 0)
                {
                    WEX::Common::Throw::IfFalse(value == nullptr, E_INVALIDARG,
                        L"A test class must not declare the same property more than once.");
                    value = reinterpret_cast<const wchar_t*>(property->m_pszPropertyValue);
                }
            }
            return value;
        }

        static void ConfigureWin32Host()
        {
            auto hostingSetupHelper = ref new test_infra::Hosting::HostingHelpers::HostingSetupHelper();
            hostingSetupHelper->InitializeWPFHostFactory();
        }
    };


} } } } }

#define XAML_HOSTING_MODE_CLASS_SETUP() \
    ::Microsoft::UI::Xaml::Tests::Common::CommonTestSetupHelper::CommonTestClassSetup(TAEF_GetClassMetadata())