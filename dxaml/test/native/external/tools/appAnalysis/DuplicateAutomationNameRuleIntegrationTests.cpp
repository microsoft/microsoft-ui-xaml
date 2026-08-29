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
#include "DuplicateAutomationNameRuleIntegrationTests.h"
#include "MUX-ETWEvents.h"
#include "RuleTesterHelper.h"
#include "TestHelpers.h"
#include "CustomUserControl.h"
#include "CustomTypes.XamlTypeInfo.g.h"

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;
using namespace ::Tests::Tools::Shared;
using namespace Microsoft::Diagnostics::AppAnalysis;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        Platform::String^ DuplicateAutomationNameRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/tools/";
        }

        bool DuplicateAutomationNameRuleIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            // Ensure our metadata and custom DPs are registered
            m_provider.reset(new XamlMetadataProviderOverrider(
                ref new MetadataProvider));
            RunOnUIThread([]()
            {
                CustomUserControl::RegisterDependencyProperties();
            });

            return true;
        }

        bool DuplicateAutomationNameRuleIntegrationTests::ClassCleanup()
        {
            RunOnUIThread([]()
            {
                CustomUserControl::ClearDependencyProperties();
            });

            return true;
        }

        bool DuplicateAutomationNameRuleIntegrationTests::TestCleanup()
        {
            return true;
        }

        void DuplicateAutomationNameRuleIntegrationTests::CatchesSameControlsWithSameName()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0007", L"CatchesSameControlsWithSameName");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithDuplicateName_SameParent.xaml";
            auto rootPage= AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);

            helper.VerifyRuleTriggered(1u);

            // We fire the instance for the second element
            helper.VerifySourceInfo(0, fileName, 12u, 112u);
        }

        void DuplicateAutomationNameRuleIntegrationTests::DoesntCatchDifferentControlsWithSameName()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0007", L"DoesntCatchDifferentControlsWithSameName");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithDuplicateName_DiffType.xaml";
            auto rootPage = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);

            helper.VerifyRuleNotTriggered();
        }

        void DuplicateAutomationNameRuleIntegrationTests::DoesntCatchSameControlsWithSameNameWithDifferentParent()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0007", L"DoesntCatchSameControlsWithSameNameWithDifferentParent");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithDuplicateName_DiffParent.xaml";
            auto rootPage = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);

            helper.VerifyRuleNotTriggered();
        }

    } }
} } } }
