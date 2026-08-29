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
#include "LVDevirtualizedRuleIntegrationTests.h"
#include "MUX-ETWEvents.h"
#include "RuleTesterHelper.h"
#include "TestHelpers.h"

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
using namespace Microsoft::Diagnostics::AppAnalysis;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        Platform::String^ LVDevirtualizedRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/tools/";
        }

        bool LVDevirtualizedRuleIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool LVDevirtualizedRuleIntegrationTests::ClassCleanup()
        {
            return true;
        }

        bool LVDevirtualizedRuleIntegrationTests::TestCleanup()
        {
            return true;
        }

        // Verifies the condition where a ListView is devirtualized due to not having a constraint
        // in the direction that the ItemsPanel is trying to layout it's items
        void LVDevirtualizedRuleIntegrationTests::CatchesDevirtualizedDueToLayout()
        {
            TestCleanupWrapper cleanup;

            xaml_data::CollectionViewSource^ cvs = nullptr;
            RuleTesterHelper helper(L"AA0003", L"CatchesDevirtualizedDueToLayout");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithDevirtualizedListView_Layout.xaml";
            auto rootPage = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);
            RunOnUIThread([&] {
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPage->FindName(L"cvs"));
                VERIFY_IS_NOT_NULL(cvs);

                cvs->Source = AppAnalysisTestHelpers::GetGroupedData();
                cvs->IsSourceGrouped = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            helper.VerifyRuleTriggered(1u);
            helper.VerifySourceInfo(0, fileName, 11u, 13u);
        }

        // Verifies that even if the LV is inside a ScrollViewer, that as long as the scroll direction
        // is disabled in the same direction the ItemsPanel is laying out it's items that we don't
        // incorrectly raise a flag because we are able to virtualize in this scenario.
        void LVDevirtualizedRuleIntegrationTests::DoesntFireFalsePositives()
        {
            TestCleanupWrapper cleanup;

            xaml_data::CollectionViewSource^ cvs = nullptr;
            RuleTesterHelper helper(L"AA0003", L"LV_DoesntFireFalsePositives");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithVirtualizedListView.xaml";
            auto rootPage = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);
            RunOnUIThread([&] {
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPage->FindName(L"cvs"));
                VERIFY_IS_NOT_NULL(cvs);

                cvs->Source = AppAnalysisTestHelpers::GetGroupedData();
            });

            TestServices::WindowHelper->WaitForIdle();
            helper.VerifyRuleNotTriggered();
        }

    } }
} } } }
