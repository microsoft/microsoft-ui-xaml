// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BorderIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Border {

    const double BorderIntegrationTests::s_errorMargin = 0.0001;

    bool BorderIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool BorderIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool BorderIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void BorderIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Border>::CanInstantiate();
    }

    void BorderIntegrationTests::CanPositionChildInsideBorder()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Border^ border = nullptr;
        xaml_shapes::Rectangle^ child = nullptr;

        auto spHasLoadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Border, Loaded);

        RunOnUIThread([&]()
        {
            border = ref new xaml_controls::Border();
            border->Height = 149;
            border->Width = 111;
            border->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
            border->BorderBrush = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Orange);
            border->BorderThickness = xaml::Thickness({ 25, 10, 10, 25 });
            border->Margin = xaml::Thickness({ 25, 10, 10, 25 });

            child = ref new xaml_shapes::Rectangle();
            child->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            child->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            child->VerticalAlignment = xaml::VerticalAlignment::Top;

            loadedRegistration.Attach(border, ref new xaml::RoutedEventHandler([spHasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                spHasLoadedEvent->Set();
            }));

            border->Child = child;
            TestServices::WindowHelper->WindowContent = border;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto childOffset = child->TransformToVisual(border)->TransformPoint(wf::Point());

            LOG_OUTPUT(L"CanPositionChildInsideBorder: child offset x=%f y=%f", childOffset.X, childOffset.Y);
            VERIFY_IS_LESS_THAN(border->BorderThickness.Left - childOffset.X, s_errorMargin);
            VERIFY_IS_LESS_THAN(border->BorderThickness.Top - childOffset.Y, s_errorMargin);
            loadedRegistration.Detach();
        });
    }

} } } } } } // namespace Microsoft::UI::Xaml::Tests::Controls::Border
