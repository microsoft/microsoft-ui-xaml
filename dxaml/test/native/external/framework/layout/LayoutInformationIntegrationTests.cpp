// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LayoutInformationIntegrationTests.h"

#include <Layout.CustomTypes.h>
#include <XamlTailored.h>
#include <DisableErrorReportingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

using namespace test_infra;
using namespace ::Tests::Native::External::Framework::Layout;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layout {

    bool LayoutInformationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool LayoutInformationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool LayoutInformationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    private ref class MeasureOverrideExceptionControl sealed: public Microsoft::UI::Xaml::Controls::Control
    {
    protected:
        ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size) override
        {
            throw ref new Platform::NotImplementedException();
        }
    };

    private ref class ArrangeOverrideExceptionControl sealed: public Microsoft::UI::Xaml::Controls::Control
    {
    protected:
        ::Windows::Foundation::Size ArrangeOverride(::Windows::Foundation::Size) override
        {
            throw ref new Platform::NotImplementedException();
        }
    };

    //
    // Test Cases
    //
    void LayoutInformationIntegrationTests::VerifyGetLayoutExceptionElement()
    {
        TestCleanupWrapper cleanup;

        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verify GetLayoutExceptionElement returns NULL when no exception.");
            VERIFY_IS_NULL(xaml_primitives::LayoutInformation::GetLayoutExceptionElement(Window::Current->CoreWindow->Dispatcher));
        });

        RunOnUIThread([&]()
        {
            auto measureExceptionControl = ref new MeasureOverrideExceptionControl();
            TestServices::WindowHelper->WindowContent = measureExceptionControl;

            LOG_OUTPUT(L"Make control throw exception.");
            VERIFY_THROWS_WINRT(measureExceptionControl->Measure(wf::Size(400, 400)), Platform::NotImplementedException^);

            UIElement^ measureExceptionUIElement = safe_cast<UIElement^>(measureExceptionControl);
            LOG_OUTPUT(L"Verify that control linked to exception matches original control.");
            UIElement^ measureExceptionResult = xaml_primitives::LayoutInformation::GetLayoutExceptionElement(Window::Current->CoreWindow->Dispatcher);
            VERIFY_ARE_EQUAL(measureExceptionUIElement, measureExceptionResult);
            TestServices::WindowHelper->WindowContent = nullptr;

            auto arrangeExceptionControl = ref new ArrangeOverrideExceptionControl();
            TestServices::WindowHelper->WindowContent = arrangeExceptionControl;

            LOG_OUTPUT(L"Make control throw exception.");
            VERIFY_THROWS_WINRT(arrangeExceptionControl->Arrange(::Windows::Foundation::Rect(0, 0, 40, 40)), Platform::NotImplementedException^);

            UIElement^ arrangeExceptionUIElement = safe_cast<UIElement^>(arrangeExceptionControl);
            LOG_OUTPUT(L"Verify that control linked to exception matches original control.");
            UIElement^ arrangeExceptionResult = xaml_primitives::LayoutInformation::GetLayoutExceptionElement(Window::Current->CoreWindow->Dispatcher);
            VERIFY_ARE_EQUAL(arrangeExceptionUIElement, arrangeExceptionResult);
            TestServices::WindowHelper->WindowContent = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto measureExceptionControl = ref new MeasureOverrideExceptionControl();
            auto arrangeExceptionControl = ref new ArrangeOverrideExceptionControl();
            auto stackPanel = ref new xaml_controls::StackPanel();
            TestServices::WindowHelper->WindowContent = stackPanel;

            stackPanel->Children->Append(measureExceptionControl);
            stackPanel->Children->Append(arrangeExceptionControl);

            LOG_OUTPUT(L"Make controls throw exception.");
            VERIFY_THROWS_WINRT(measureExceptionControl->Measure(wf::Size(400, 400)), Platform::NotImplementedException^);
            VERIFY_THROWS_WINRT(arrangeExceptionControl->Arrange(::Windows::Foundation::Rect(0, 0, 40, 40)), Platform::NotImplementedException^);

            UIElement^ measureExceptionUIElement = safe_cast<UIElement^>(measureExceptionControl);
            UIElement^ arrangeExceptionUIElement = safe_cast<UIElement^>(arrangeExceptionControl);

            UIElement^ exceptionResult = xaml_primitives::LayoutInformation::GetLayoutExceptionElement(Window::Current->CoreWindow->Dispatcher);

            LOG_OUTPUT(L"Verify GetLayoutExceptionElement returns last control that generated an exception.");
            VERIFY_ARE_NOT_EQUAL(measureExceptionUIElement, exceptionResult);
            VERIFY_ARE_EQUAL(arrangeExceptionUIElement, exceptionResult);
            TestServices::WindowHelper->WindowContent = nullptr;
        });

        // Clear exception element
        TestServices::WindowHelper->SetLastLayoutExceptionElement(nullptr);
    }

    void LayoutInformationIntegrationTests::ValidateLayoutRoundingMargin()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 300));
        xaml_controls::Grid^ grid = nullptr;
        xaml_controls::StackPanel^ stackPanel = nullptr;

        RunOnUIThread([&]()
        {
            grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid.RowDefinitions>"
                L"      <RowDefinition Height='Auto'/>"
                L"  </Grid.RowDefinitions>"
                L"  <Grid.ColumnDefinitions>"
                L"      <ColumnDefinition Width='Auto'/>"
                L"  </Grid.ColumnDefinitions>"
                L"  <StackPanel Grid.Row='0' Grid.Column='0' x:Name='Stack' Margin = '7,7,7,7'>"
                L"      <Rectangle Width='44' Height='40'/>"
                L"  </StackPanel>"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        float h = 0;
        float w = 0;
        RunOnUIThread([&]()
        {
            stackPanel = TreeHelper::GetVisualChildByType<xaml_controls::StackPanel>(grid);
            w = (float)stackPanel->ActualWidth;
            h = (float)stackPanel->ActualHeight;
        });

        VERIFY_ARE_EQUAL(w, 44.0f);
        VERIFY_ARE_EQUAL(h, 40.0f);

        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(wf::Size(300, 300), 1.25f);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            w = (float)stackPanel->ActualWidth;
            h = (float)stackPanel->ActualHeight;
        });

        VERIFY_ARE_EQUAL(w, 44.0f);
        VERIFY_ARE_EQUAL(h, 40.0f);
    }

    void LayoutInformationIntegrationTests::ValidateDirtyPathWhenDirty()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        StackPanel^ sp0 = nullptr;
        StackPanel^ sp1 = nullptr;
        StackPanel^ sp2 = nullptr;
        StackPanel^ sp3 = nullptr;
        CustomLayoutUserControl^ cluc = nullptr;
        TextBlock^ tb = nullptr;
        xaml_shapes::Rectangle^ r = nullptr;

        // Build the tree and let layout run through it once.
        // After this is done, all elements are clean.
        RunOnUIThread([&]()
        {
            sp0 = ref new StackPanel;
            sp0->HorizontalAlignment = HorizontalAlignment::Left;
            sp0->Background = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Red);

            sp1 = ref new StackPanel;
            sp1->HorizontalAlignment = HorizontalAlignment::Left;
            sp1->Background = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Green);

            sp2 = ref new StackPanel;
            sp2->HorizontalAlignment = HorizontalAlignment::Left;
            sp2->Background = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Blue);

            sp3 = ref new StackPanel;
            sp3->HorizontalAlignment = HorizontalAlignment::Left;
            sp3->Background = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Yellow);

            cluc = ref new CustomLayoutUserControl;

            tb = ref new TextBlock;
            tb->Foreground = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Magenta);
            tb->Text = "lorem ipsum dolor sit amet, consectetur adipiscing elit";
            tb->Width = 300.0;

            r = ref new xaml_shapes::Rectangle;
            r->Fill = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Cyan);

            cluc->Content = tb;
            sp3->Children->Append(cluc);
            sp2->Children->Append(sp3);
            sp1->Children->Append(sp2);
            sp0->Children->Append(sp1);
            sp0->Children->Append(r);
            TestServices::WindowHelper->WindowContent = sp0;
        });
        TestServices::WindowHelper->WaitForIdle();

        // In order to repro the original problem, we need to trigger an Arrange pass with
        // very specific requirements. The custom user control needs to be dirty for arrange
        // and it should invalidate its ancestor (something other than its parent; in this case
        // its great-grandparent) during ArrangeOverride. The ancestors in between the user
        // control and the invalidated parent as well as the Content of the user control must
        // all be dirty for arrange too. Finally, the ancestor that will be invalidated must be
        // originally clean, but Arrange should still be called on it. Now, the only way this can
        // happen is if the parent of this element (i.e. the great-great-grandparent of the user
        // control in this case) passes a final rect to its child that is different than the final
        // rect it passed during the previous arrange pass. We can achieve this by changing the Width
        // of the rectangle, which is a sibling of the ancestor that will be invalidated. Making
        // the rectangle wide enough will result in a new desired size for the parent of the ancestor
        // that will be invalidated, which will then result in a new final rect for its children.
        RunOnUIThread([&]()
        {
            cluc->ActivateCustomLayout();
            sp3->InvalidateArrange();
            sp2->InvalidateArrange();
            r->Width = 350.0;
            tb->InvalidateArrange();
            cluc->InvalidateArrange();
        });
        TestServices::WindowHelper->WaitForIdle();

        // With the pre-RS2 behavior, there is now a section of elements marked as OnArrangeDirtyPath,
        // but there are no elements marked as ArrangeDirty below them. Even worse, the dirty path
        // is actually disconnected, because the ancestor that got invalidated during the arrange pass
        // by the user control is now clean given that we called Arrange on it. The disconnected dirty
        // path includes the user control plus its ancestors all the way up till we reach the one that
        // had been invalidated (excluding it). This means that if we invalidate the child of the user
        // control for arrange, it will be stuck forever as ArrangeDirty since the propagation of the
        // OnArrangeDirtyPath flag will stop immediately after realizing that the user control is already
        // on the dirty path, but then the child is unreachable because the path is actually disconnected.
        // In the case of the TextBlock, having it being ArrangeDirty and then triggering a redraw
        // (e.g. by changing the Foreground color) will cause it to disappear, since we don't render
        // elements that are dirty for layout.
        RunOnUIThread([&]()
        {
            tb->InvalidateArrange();
            tb->Foreground = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Black);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void LayoutInformationIntegrationTests::ValidateGetAvailableSize()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(200, 300));

        StackPanel^ panel = nullptr;
        RunOnUIThread([&]()
        {
            auto button = ref new Button();
            button->Content = L"Hello";

            auto lastSize = xaml_primitives::LayoutInformation::GetAvailableSize(button);
            VERIFY_ARE_EQUAL(0, lastSize.Width);
            VERIFY_ARE_EQUAL(0, lastSize.Height);

            button->Measure(wf::Size(300, 400));
            lastSize = xaml_primitives::LayoutInformation::GetAvailableSize(button);
            VERIFY_ARE_EQUAL(300, lastSize.Width);
            VERIFY_ARE_EQUAL(400, lastSize.Height);

            panel = ref new StackPanel();
            auto button2 = ref new Button();
            button->Content = L"World";
            panel->Children->Append(button2);
            TestServices::WindowHelper->WindowContent = panel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto lastSize = xaml_primitives::LayoutInformation::GetAvailableSize(panel);
            VERIFY_ARE_EQUAL(200, lastSize.Width);
            VERIFY_ARE_EQUAL(300, lastSize.Height);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout
