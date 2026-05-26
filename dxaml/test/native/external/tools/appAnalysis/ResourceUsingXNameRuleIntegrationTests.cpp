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
#include "ResourceUsingXNameRuleIntegrationTests.h"
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

        Platform::String^ ResourceUsingXNameRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/tools/";
        }

        bool ResourceUsingXNameRuleIntegrationTests::ClassSetup()
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

        bool ResourceUsingXNameRuleIntegrationTests::ClassCleanup()
        {
            RunOnUIThread([]()
            {
                CustomUserControl::ClearDependencyProperties();
            });

            return true;
        }

        bool ResourceUsingXNameRuleIntegrationTests::TestCleanup()
        {
            return true;
        }

        void ResourceUsingXNameRuleIntegrationTests::CatchesSolidColorBrushWithXName()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0008", L"CatchesSolidColorBrushWithXName");

            Platform::String^ fileName = GetResourcesPath() + L"PageWithXNamedResource.xaml";
            auto page = AppAnalysisTestHelpers::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(page);

            helper.VerifyRuleTriggered(1u);

            helper.VerifySourceInfo(0, fileName, 6u, 44u);
        }

    } }
} } } }
