// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LayoutTests.h"
#include <TestCleanupWrapper.h>

#include <XamlTailored.h>
#include <WUCRenderingScopeGuard.h>

using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::Windows::UI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layout {

bool LayoutTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool LayoutTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool LayoutTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void LayoutTests::LayoutRoundingIncludesMarginsCommon()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    wh->SetWindowSizeOverrideWithScale(wf::Size(400, 700), 2.25f);

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ content = ref new xaml_shapes::Rectangle();
        content->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        content->Height = 555.555542;
        content->Width = 100;

        Grid^ withMargin = ref new Grid();
        withMargin->Margin = xaml::Thickness({ 0, 7, 0, 7 });
        withMargin->HorizontalAlignment = HorizontalAlignment::Left;
        withMargin->Children->Append(content);

        StackPanel^ stackPanel = ref new StackPanel();
        stackPanel->Children->Append(withMargin);

        Canvas^ root = ref new Canvas();
        root->Children->Append(stackPanel);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    //
    // Measure runs first.
    //
    // - content's desired size is 555.555542. This value is already layout rounded:
    //      555.555542 * 2.25 = 1249.9999695
    //
    // - withMargins's desired size will also be 555.555542.
    //
    // - withMargins then adds 14 for the margins to get 569.555542, which then gets rounded down to 569.333313.
    //      569.555542 * 2.25 = 1281.4999695 => 1281
    //      569.333313 * 2.25 = 1280.99995425 => 1281
    //   This desired size gets set on stackPanel.
    //
    //   Note that the unrounded margins get added. If it were rounded, the margins would be 14.222:
    //      14 * 2.25 = 31.5
    //      14.222 * 2.25 = 31.9995
    //
    // Now arrange runs.
    //
    // - stackPanel runs arrange at its desired height of 569.333313.
    //
    // - 569.333313 gets passed down to withMargin, which subtracts 14.222 for the rounded margins to get 555.111.
    //   That's smaller than withMargin's desired size of 555.555542, so withMargin applies a layout clip on itself.
    //
    // This layout clip is a problem. In the more complicated repro, withMargins is a CCarouselPanel in a
    // ComboBox, and contains lots of items. Its 9 visible items have a combined rounded size of 555.555542,
    // and it has lots of items beyond the visible area that can be scrolled into view. This bug means the
    // CCarouselPanel puts a layout clip on itself, which then clips out all those other items, causing the
    // ComboBox's menu to render mostly blank.
    //
    // ComboBox at 225% scale factor + non-default item height = broken ComboBox
    //
    // The fix is for measure to round the margins before adding them, just like how arrange rounds them before
    // subtracting them. Margins of 14 get rounded to 14.222.
    //      14.222 * 2.25 = 32
    //
    // - withMargin now adds 14.222 for the margins, giving 569.778, which is already layout rounded:
    //      569.778 * 2.25 = 1282
    //   this desired size gets set on stackPanel.
    //
    // Now arrange runs.
    //
    // - stackPanel runs arrange at its desired height of 569.778.
    //
    // - 569.778 gets passed down to withMargin, which subtracts 14.222 for the margins to get 555.556. That
    //   matches withMargin's desired size of 555.555542, so no layout clip gets applied.
    //
}

void LayoutTests::LayoutRoundingIncludesMargins()
{
    LayoutRoundingIncludesMarginsCommon();
}

// Helper to set a tracing level and reset it at the end of the test.
class LayoutCycleTracingHelper
{
public:
    LayoutCycleTracingHelper(LayoutCycleTracingLevel level)
    {
        RunOnUIThread([&]
        {
            m_debugSettings = Application::Current->DebugSettings;
            m_oldLevel = m_debugSettings->LayoutCycleTracingLevel;
            m_debugSettings->LayoutCycleTracingLevel = level;
        });
    }

    ~LayoutCycleTracingHelper()
    {
        if (m_debugSettings != nullptr)
        {
            RunOnUIThread([&]
            {
                m_debugSettings->LayoutCycleTracingLevel = m_oldLevel;
            });
        }
    }

private:
    DebugSettings^ m_debugSettings;
    LayoutCycleTracingLevel m_oldLevel;
};

void LayoutTests::SliderWithMarginNoLayoutCycle()
{
    TestCleanupWrapper cleanup;

    auto tracingHelper = LayoutCycleTracingHelper(LayoutCycleTracingLevel::High);

    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(400, 400), 1.25f);

    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <StackPanel Width='895'>"
            L"    <Slider x:Name='slider' Value='99.9' Margin='0,0,10,10'/>"
            L"  </StackPanel>"
            L"</StackPanel>"));

        auto slider = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));
        VERIFY_IS_NOT_NULL(slider);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    // If we made it here without crashing, success!
}

void LayoutTests::ProgressBarWithBorderNoLayoutCycle()
{
    TestCleanupWrapper cleanup;

    auto tracingHelper = LayoutCycleTracingHelper(LayoutCycleTracingLevel::High);

    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(400, 400), 1.5f);

    LOG_OUTPUT(L"Testing <ProgressBar> with BorderThickness.");
    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <StackPanel Orientation='Horizontal'>" // Horizontal orientation is required to hit this crash!
            L"    <ProgressBar x:Name='progressBar' MinWidth='200' Value='100' BorderThickness='3'/>"
            L"  </StackPanel>"
            L"</StackPanel>"));

        auto progressBar = safe_cast<xaml_controls::ProgressBar^>(rootPanel->FindName(L"progressBar"));
        VERIFY_IS_NOT_NULL(progressBar);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    // If we made it here without crashing, success!

    LOG_OUTPUT(L"Testing Paused (or Error) indeterminate <ProgressBar> with Padding.");
    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <StackPanel Orientation='Horizontal'>" // Horizontal orientation is required to hit this crash!
            L"    <ProgressBar x:Name='progressBar' MinWidth='200' Value='100' IsIndeterminate='True' ShowPaused='True' Padding='3'/>"
            L"  </StackPanel>"
            L"</StackPanel>"));

        auto progressBar = safe_cast<xaml_controls::ProgressBar^>(rootPanel->FindName(L"progressBar"));
        VERIFY_IS_NOT_NULL(progressBar);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    // If we made it here without crashing, success!

    LOG_OUTPUT(L"Testing Error (or Paused) indeterminate <ProgressBar> with BorderThickness.");
    // This case should be the same as paused-with-Padding above.
    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <StackPanel Orientation='Horizontal'>" // Horizontal orientation is required to hit this crash!
            L"    <ProgressBar x:Name='progressBar' MinWidth='200' Value='100' IsIndeterminate='True' ShowError='True' BorderThickness='3'/>"
            L"  </StackPanel>"
            L"</StackPanel>"));

        auto progressBar = safe_cast<xaml_controls::ProgressBar^>(rootPanel->FindName(L"progressBar"));
        VERIFY_IS_NOT_NULL(progressBar);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    // If we made it here without crashing, success!
}

void LayoutTests::ScrollBarMarginRoundingNoLayoutCycle()
{
    TestCleanupWrapper cleanup;

    auto tracingHelper = LayoutCycleTracingHelper(LayoutCycleTracingLevel::High);

    TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(400, 400), 2.25f);

    xaml_controls::ScrollViewer^ scrollViewer;

    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <ScrollViewer x:Name='scrollViewer' Height='654.2222290039062' Width='400'>"
            L"    <StackPanel Height='2267.111023'>"
            L"      <StackPanel.Background>"
            L"        <LinearGradientBrush>"
            L"          <LinearGradientBrush.GradientStops>"
            L"            <GradientStop Color='Blue' Offset='0'/>"
            L"            <GradientStop Color='Red' Offset='1'/>"
            L"          </LinearGradientBrush.GradientStops>"
            L"        </LinearGradientBrush>"
            L"      </StackPanel.Background>"
            L"    </StackPanel>"
            L"  </ScrollViewer>"
            L"</StackPanel>"));

        scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));
        VERIFY_IS_NOT_NULL(scrollViewer);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scrollViewer->ScrollToVerticalOffset(2267.111023);
    });
    TestServices::WindowHelper->WaitForIdle();

    // If we made it here without crashing, success!
}

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout
