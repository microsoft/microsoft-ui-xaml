// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include "TestCleanupWrapper.h"
#include <collection.h>
#include "StickyHeadersHelper.h"
#include "DynamicBindingRuleIntegrationTests.h"
#include "MUX-ETWEvents.h"
#include "RuleTesterHelper.h"
#include "TestHelpers.h"
#include "ValueConverters.h"
#include "CustomUserControl.h"
#include "PageWithTemplatedParentBinding.xaml.h"
#include "CustomTypes.XamlTypeInfo.g.h"
#include <CustomMetadataRegistrar.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace ::Windows::Storage::Streams;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace ::Tests::Tools::Shared;
using namespace Microsoft::Diagnostics::AppAnalysis;
namespace shared_types = ::Tests::Tools::Shared;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        Platform::String^ DynamicBindingRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/";
        }

        bool DynamicBindingRuleIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool DynamicBindingRuleIntegrationTests::ClassCleanup()
        {
            return true;
        }

        bool DynamicBindingRuleIntegrationTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());

            return true;
        }

        bool DynamicBindingRuleIntegrationTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void DynamicBindingRuleIntegrationTests::CorrectlyCatchesBasicBinding()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0005", L"CorrectlyCatchesBasicBinding");

            Platform::String^ fileName = GetResourcesPath() + L"tools/PageWithCustomUserControl.xaml";
            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootGrid);

            helper.VerifyRuleTriggered(1u);
            helper.VerifySourceInfo(0, fileName, 8u, 95u);
        }

        void DynamicBindingRuleIntegrationTests::CorrectlyCatchesBindingInDataTemplate()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0005", L"CorrectlyCatchesBindingInDataTemplate");

            Platform::String^ fileName = GetResourcesPath() + L"tools/PageWithBindingInContentTemplate.xaml";
            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootGrid);

            helper.VerifyRuleTriggered(1u);
            helper.VerifySourceInfo(0, fileName, 12u, 26u);
        }

        void DynamicBindingRuleIntegrationTests::DoesntFlagBindingUsingSource()
        {
            TestCleanupWrapper cleanup;

            xaml_data::CollectionViewSource^ cvs = nullptr;
            RuleTesterHelper helper(L"AA0005", L"DoesntFlagBindingUsingSource");

            Platform::String^ fileName = GetResourcesPath() + L"tools/PageWithCollapsedElementsInTemplate.xaml";
            auto rootPage = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);
            RunOnUIThread([&] {

                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPage->FindName(L"cvs"));
                VERIFY_IS_NOT_NULL(cvs);

                cvs->Source = AppAnalysisTestHelpers::GetGroupedData();
                cvs->IsSourceGrouped = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            helper.VerifyRuleNotTriggered();
        }

        void DynamicBindingRuleIntegrationTests::DoesntFlagUsesOfTemplateBinding()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0005", L"DoesntFlagUsesOfTemplateBinding");

            Platform::String^ fileName = GetResourcesPath() + L"controls/pivot/RetemplatedPivot.xaml";
            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Grid>(fileName);
            VERIFY_IS_NOT_NULL(rootGrid);

            helper.VerifyRuleNotTriggered();
        }

        void DynamicBindingRuleIntegrationTests::DoesntFlagUsesOfTemplatedParent()
        {
            TestCleanupWrapper cleanup;
            RuleTesterHelper helper(L"AA0005", L"DoesntFlagUsesOfTemplatedParent");

            Platform::String^ fileName = GetResourcesPath() + L"tools/PageWithTemplatedParentBinding.xaml";
            auto page = AppAnalysisTestHelpers::LoadXaml<PageWithTemplatedParentBinding>(fileName);
            VERIFY_IS_NOT_NULL(page);

            helper.VerifyRuleNotTriggered();
        }


    } }
} } } }
