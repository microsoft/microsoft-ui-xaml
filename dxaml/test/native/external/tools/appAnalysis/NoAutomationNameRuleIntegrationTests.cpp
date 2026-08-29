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
#include "NoAutomationNameRuleIntegrationTests.h"
#include "MUX-ETWEvents.h"
#include "RuleTesterHelper.h"
#include "TestHelpers.h"
#include "CustomUserControl.h"
#include "CustomTypes.XamlTypeInfo.g.h"
#include "resource.h"

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

        Platform::String^ NoAutomationNameRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/tools/";
        }

        bool NoAutomationNameRuleIntegrationTests::ClassSetup()
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

        bool NoAutomationNameRuleIntegrationTests::ClassCleanup()
        {
            RunOnUIThread([]()
            {
                CustomUserControl::ClearDependencyProperties();
            });

            return true;
        }

        bool NoAutomationNameRuleIntegrationTests::TestCleanup()
        {
            return true;
        }

        void NoAutomationNameRuleIntegrationTests::CatchesCustomUserControlWithNoName()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0006", L"CatchesCustomUserControlWithNoName");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithCustomUserControl.xaml";
            auto rootPage= AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);

            helper.VerifyRuleTriggered(2u);

            helper.VerifySourceInfo(0, fileName, 11u, 119u);
            helper.VerifyDescription(0, ACC_NO_NAME_DESCRIPTION, L"UserControl1", L"Tests.Tools.Shared.CustomUserControl");

            helper.VerifySourceInfo(1, fileName, 12u, 56u);
            helper.VerifyDescription(1, ACC_NO_NAME_DESCRIPTION, L"UserControl1", L"Tests.Tools.Shared.CustomUserControl");
        }

    } }
} } } }
