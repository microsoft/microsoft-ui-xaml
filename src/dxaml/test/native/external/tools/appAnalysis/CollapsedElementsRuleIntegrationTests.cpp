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
#include "CollapsedElementsRuleIntegrationTests.h"
#include "MUX-ETWEvents.h"
#include "RuleTesterHelper.h"
#include "resource.h"
#include "XamlDiagnosticsHelper.h"
#include "CustomUserControl.h"
#include "MainPage.xaml.h"
#include "TestHelpers.h"
#include "customTypes.XamlTypeInfo.g.h"
#include <CustomMetadataRegistrar.h>

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
namespace custom_types = ::Tests::Tools::Shared;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        Platform::String^ CollapsedElementsRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/tools/";
        }

        bool CollapsedElementsRuleIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool CollapsedElementsRuleIntegrationTests::ClassCleanup()
        {
            return true;
        }

        bool CollapsedElementsRuleIntegrationTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<custom_types::CustomUserControl>());

            return true;
        }

        bool CollapsedElementsRuleIntegrationTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        xaml_controls::Grid^ SetupGridWithCollpasedElements()
        {
            xaml_controls::Grid^ grid;

            Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&grid]()
            {
                grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red'>"
                    L"  <Rectangle x:Name='collapsed' Fill='Red' Height='30' Width='30' Visibility='Collapsed'/>"
                    L"</Grid>"));

                grid->Width = 120;
                grid->Height = 120;
            });

            return grid;
        }

        // Verifies the basic condition where an element is collpased locally
        void CollapsedElementsRuleIntegrationTests::CorrectlyCountCollapsedElements()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0002", L"CorrectlyCountCollapsedElements");

            ::Windows::Foundation::Size size(400, 300);

            TestServices::WindowHelper->SetWindowSizeOverride(size);

            Platform::String^ fileName = GetResourcesPath() + L"PageWithCollapsedElements.xaml";
            auto rootPage = aa_help::LoadXaml<Page>(fileName);
            VERIFY_IS_NOT_NULL(rootPage);

            // We should only have one element, even though one element has opacity set to 0
            helper.VerifyRuleTriggered(1u);
            helper.VerifyMeasurement(0, MeasurementUnit::Elements, 1.00);
            helper.VerifyDescription(0, COLLAPSED_ELEMENTS_DESCRIPTION, L"collapsed", L"Microsoft.UI.Xaml.Shapes.Rectangle");
            helper.VerifySourceInfo(0, fileName, 6, 76);
        }

        // Verifies the condition where elements within a data template are correctly
        // counted.
        void CollapsedElementsRuleIntegrationTests::CorrectlyCountCollapsedElementsInTemplate()
        {
            TestCleanupWrapper cleanup;

            xaml_data::CollectionViewSource^ cvs = nullptr;
            RuleTesterHelper helper(L"AA0002", L"CorrectlyCountCollapsedElementsInTemplate");

            ::Windows::Foundation::Size size(400, 300);

            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootPage = aa_help::LoadXaml<Page>(GetResourcesPath() + L"PageWithCollapsedElementsInTemplate.xaml");
            VERIFY_IS_NOT_NULL(rootPage);
            RunOnUIThread([&] {

                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPage->FindName(L"cvs"));
                VERIFY_IS_NOT_NULL(cvs);

                cvs->Source = aa_help::GetGroupedData();
                cvs->IsSourceGrouped = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            helper.VerifyRuleTriggered(1u);
            helper.VerifyMeasurement(0, MeasurementUnit::Elements, 6.00);
        }

        void CollapsedElementsRuleIntegrationTests::VerifyElementNameWithoutSourceInfo()
        {
            TestCleanupWrapper cleanup;

            const bool c_noSourceInfo = false;
            RuleTesterHelper helper(L"AA0002", L"CorrectlyCountCollapsedElements", c_noSourceInfo);

            xaml_controls::Grid^ root = SetupGridWithCollpasedElements();
            VERIFY_IS_NOT_NULL(root);

            Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&root]()
            {
                test_infra::TestServices::WindowHelper->WindowContent = root;
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            helper.VerifyRuleTriggered(1u);
            helper.VerifyDescription(0, COLLAPSED_ELEMENTS_DESCRIPTION, L"collapsed", L"Microsoft.UI.Xaml.Shapes.Rectangle");
        }

        void CollapsedElementsRuleIntegrationTests::VerifyDoesntFlagWhenUsingEnC()
        {
            TestCleanupWrapper cleanup;
            XamlDiagnosticsHelper xamlDiagHelper;
            auto tap = xamlDiagHelper.Connect();
            auto callback = xamlDiagHelper.Advise();
            RuleTesterHelper helper(L"AA0002", L"CorrectlyCountCollapsedElements");

            auto rootPage = aa_help::LoadXaml<custom_types::MainPage>(GetResourcesPath() + L"MainPage.xaml");
            VERIFY_IS_NOT_NULL(rootPage);

            // Create a Slider
            InstanceHandle slider = 0;
            VERIFY_SUCCEEDED(tap->CreateInstance(L"Microsoft.UI.Xaml.Controls.Slider", &slider));

            VisualElement layoutRoot = callback->GetElementByName(L"LayoutRoot");

            wil::unique_propertychainvalue children = tap->GetPropertyChainValue(layoutRoot.Handle, L"Children");
            UINT32 count = 0;
            VERIFY_SUCCEEDED(tap->GetCollectionCount(std::stoll(children.Value), &count));
            VERIFY_IS_GREATER_THAN(count, 0u);

            VERIFY_SUCCEEDED(tap->AddChild(std::stoll(children.Value), slider, count));

            TestServices::WindowHelper->WaitForIdle();
            helper.VerifyRuleNotTriggered();

            xamlDiagHelper.OnTestComplete(callback);
        }
    } }
} } } }
