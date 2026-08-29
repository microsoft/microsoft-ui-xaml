// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PanelExTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FeatureFlags.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layering {

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

// TODO:  Dynamically enable velocity instead of skipping test
#define VELOCITY_TESTGUARD_XAML2018 if (!Feature_Xaml2018::IsEnabled()) { LOG_OUTPUT(L"XAML2018 velocity feature disabled, skipping test"); return; }

    bool PanelExTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PanelExTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PanelExTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void PanelExTests::CanAccessChildrenProperty()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            LOG_OUTPUT(L"Validate adding a child programmatically.");
            auto element = ref new xaml::PanelEx();
            element->Children->Append(ref new xaml_shapes::Rectangle());
            VERIFY_ARE_EQUAL(1u, element->Children->Size);

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([]()
        {
            LOG_OUTPUT(L"Validate adding a child via markup.");
            auto element = safe_cast<xaml::PanelEx^>(xaml_markup::XamlReader::Load(
                LR"(<PanelEx xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <Rectangle/>
                    </PanelEx>)"));
            VERIFY_ARE_EQUAL(1u, element->Children->Size);

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void PanelExTests::DoesPerformLayout()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml::PanelEx^ element = nullptr;
        xaml_shapes::Rectangle^ child1 = nullptr;
        xaml_shapes::Rectangle^ child2 = nullptr;
        xaml_shapes::Rectangle^ child3 = nullptr;

        const wf::Size availableSize(500, 400);

        RunOnUIThread([&]()
        {
            element = ref new xaml::PanelEx();
            element->Width = availableSize.Width;
            element->Height = availableSize.Height;

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            child1 = ref new xaml_shapes::Rectangle();
            child1->Width = 250;
            child1->Height = 250;
            child1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Orange);
            element->Children->Append(child1);

            child2 = ref new xaml_shapes::Rectangle();
            child2->Width = 175;
            child2->Height = 175;
            child2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
            element->Children->Append(child2);

            child3 = ref new xaml_shapes::Rectangle();
            child3->Width = 100;
            child3->Height = 100;
            child3->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::YellowGreen);
            element->Children->Append(child3);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto validateOffsets = [element, availableSize](xaml::FrameworkElement^ target)
            {
                auto transform = target->TransformToVisual(element);
                auto transformedOffset = transform->TransformPoint(wf::Point(0, 0));

                VERIFY_IS_GREATER_THAN(target->ActualWidth, 0);
                VERIFY_IS_GREATER_THAN(target->ActualHeight, 0);

                VERIFY_ARE_EQUAL(std::floor((availableSize.Width - target->ActualWidth) * 0.5 + 0.5), transformedOffset.X);
                VERIFY_ARE_EQUAL(std::floor((availableSize.Height - target->ActualHeight) * 0.5 + 0.5), transformedOffset.Y);
            };

            LOG_OUTPUT(L"Validating offset of child1.");
            validateOffsets(child1);

            LOG_OUTPUT(L"Validating offset of child2.");
            validateOffsets(child2);

            LOG_OUTPUT(L"Validating offset of child3.");
            validateOffsets(child3);
        });
    }

    void PanelExTests::DoesRespectMinSize()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;
        xaml::PanelEx^ element = nullptr;

        const wf::Size expectedSize(500, 400);

        RunOnUIThread([&]()
        {
            element = ref new xaml::PanelEx();

            element->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            element->VerticalAlignment = xaml::VerticalAlignment::Center;

            element->MinWidth = expectedSize.Width;
            element->MinHeight = expectedSize.Height;

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedSize.Width, element->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSize.Height, element->ActualHeight);
        });
    }

    void PanelExTests::DoesRespectMaxSize()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;
        xaml::PanelEx^ element = nullptr;

        const wf::Size expectedSize(500, 400);

        RunOnUIThread([&]()
        {
            element = ref new xaml::PanelEx();

            element->HorizontalAlignment = xaml::HorizontalAlignment::Stretch;
            element->VerticalAlignment = xaml::VerticalAlignment::Stretch;

            element->MaxWidth = expectedSize.Width;
            element->MaxHeight = expectedSize.Height;

            TestServices::WindowHelper->WindowContent = element;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedSize.Width, element->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSize.Height, element->ActualHeight);
        });
    }

    void PanelExTests::DoesRespectMargin()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        xaml::PanelEx^ element = nullptr;
        xaml_controls::Border^ host = nullptr;

        const xaml::Thickness margin = { 1, 2, 3, 4 };
        const wf::Size defaultSize(500, 400);

        RunOnUIThread([&]()
        {
            element = ref new xaml::PanelEx();

            element->Width = defaultSize.Width;
            element->Height = defaultSize.Height;
            element->Margin = margin;

            host = ref new xaml_controls::Border();
            host->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            host->VerticalAlignment = xaml::VerticalAlignment::Center;

            host->Child = element;

            TestServices::WindowHelper->WindowContent = host;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(element->Width + margin.Left + margin.Right, host->ActualWidth);
            VERIFY_ARE_EQUAL(element->Height + margin.Top + margin.Bottom, host->ActualHeight);
        });
    }
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layering
