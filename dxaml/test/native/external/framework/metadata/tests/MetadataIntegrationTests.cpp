// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <collection.h>
#include <XamlTailored.h>
#include <StringUtilities.h>
#include <TestEvent.h>
#include <Utils.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include <XamlMetadataProviderOverrider.h>
#include "MetadataIntegrationTests.h"
#include "CustomMetadataProvider.h"
#undef max

using namespace Platform;
using namespace Platform::Collections;
using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace std;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Metadata {

        bool MetadataIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool MetadataIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool MetadataIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void MetadataIntegrationTests::ImportTypeKindMetadata()
        {
            RunOnUIThread([&]()
            {
                auto border = ref new Border();
                auto provider = ref new CustomMetadataProvider();

                std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::XamlMetadataProviderOverrider> overrider(new XamlMetadataProviderOverrider(provider));

                // Built-in namespace, but custom type name.
                wxaml_interop::TypeName builtinNamespaceCustomTypeName =
                {
                    L"Windows.Foundation.MyType",
                    wxaml_interop::TypeKind::Metadata
                };

                // Custom namespace, custom type name.
                wxaml_interop::TypeName customNamespaceCustomTypeName =
                {
                    L"MyNamespace.MyType",
                    wxaml_interop::TypeKind::Metadata
                };

                // Verify initial state.
                VERIFY_IS_FALSE(provider->CalledWithTypeName);
                VERIFY_IS_FALSE(provider->CalledWithFullName);

                auto dp1 = DependencyProperty::RegisterAttached(L"ImportTypeKindMetadata_DP_1", builtinNamespaceCustomTypeName, Border::typeid, nullptr);

                // Force the DP to get initialized.
                border->GetValue(dp1);

                // Verify the GetXamlType(TypeName) overload was invoked.
                VERIFY_IS_TRUE(provider->CalledWithTypeName);
                VERIFY_IS_FALSE(provider->CalledWithFullName);

                auto dp2 = DependencyProperty::RegisterAttached(L"ImportTypeKindMetadata_DP_2", customNamespaceCustomTypeName, Border::typeid, nullptr);

                // Force the DP to get initialized.
                border->GetValue(dp2);

                // Verify the GetXamlType(TypeName) overload was invoked.
                VERIFY_IS_FALSE(provider->CalledWithTypeName);
                VERIFY_IS_TRUE(provider->CalledWithFullName);
            });
        }

        void MetadataIntegrationTests::CanRegisterSameDPTwiceOnBuiltinControl()
        {
            RunOnUIThread([&]()
            {
                auto dp1 = DependencyProperty::RegisterAttached(L"CanRegisterSameDPTwiceOnBuiltinControl_DP", Object::typeid, Border::typeid, nullptr);
                auto dp2 = DependencyProperty::RegisterAttached(L"CanRegisterSameDPTwiceOnBuiltinControl_DP", Object::typeid, Border::typeid, nullptr);

                // Force DPs to be fully initialized. We used to crash during the initialization of the 2nd property.
                auto border = ref new Border();
                VERIFY_IS_NULL(border->GetValue(dp1));
                VERIFY_IS_NULL(border->GetValue(dp2));
            });
        }

} } } } } }
