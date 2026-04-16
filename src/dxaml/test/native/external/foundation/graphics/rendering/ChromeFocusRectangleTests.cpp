// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ChromeFocusRectangleTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include "SafeEventRegistration.h"
#include <TreeHelper.h>
#include "KeyboardInjectionOverride.h"
#include <SurfaceIdModeScopeGuard.h>
#include "CommonInputHelper.h"
#include <FocusTestHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::System::Power;

class FocusRectSetupHelper
{
public:
    explicit FocusRectSetupHelper(DCompRendering dcompRenderType, FocusVisualKind focusVisualKind, float width = 800.0f, float height = 600.0f)
        : m_requestedFocusVisualKind(focusVisualKind)
        , m_guard(dcompRenderType, false /* resizeWindow */, true /* injectMockDComp */)
        , m_mockDCompAttached(false)
    {
        RunOnUIThread([&]()
        {
            m_previousFocusVisualKind = Microsoft::UI::Xaml::Application::Current->FocusVisualKind;
            Microsoft::UI::Xaml::Application::Current->FocusVisualKind = m_requestedFocusVisualKind;
        });

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(width, height));
    }

    FrameworkElement^ LoadAndSetFocus(Platform::String^ xaml, Platform::String^ elementNameToFocus)
    {
        FrameworkElement^ root;
        Control^ focusedControl;
        RunOnUIThread([&]()
        {
            root = safe_cast<FrameworkElement^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
            focusedControl = safe_cast<Control^>(root->FindName(elementNameToFocus));
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        SetFocus(focusedControl);
        return root;
    }

    Control^ FindAndSetFocus(FrameworkElement^ root, Platform::String^ elementNameToFocus)
    {
        Control^ focusedControl;
        RunOnUIThread([&]()
        {
            focusedControl = safe_cast<Control^>(root->FindName(elementNameToFocus));
        });
        SetFocus(focusedControl);
        return focusedControl;
    }

    void SetFocus(Control^ controlToFocus)
    {
        RunOnUIThread([&]()
        {
            controlToFocus->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    ~FocusRectSetupHelper()
    {
        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Application::Current->FocusVisualKind = m_previousFocusVisualKind;
        });

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

        if (m_mockDCompAttached)
        {
            TestServices::WindowHelper->DetachMockDComp();
        }
    }


    FocusVisualKind m_requestedFocusVisualKind;
    FocusVisualKind m_previousFocusVisualKind;
    bool m_mockDCompAttached;
    WUCRenderingScopeGuard m_guard;
};

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation {
    namespace Graphics {
        Platform::String^ ChromeFocusRectangleTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }

        bool ChromeFocusRectangleTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ChromeFocusRectangleTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ChromeFocusRectangleTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Renders a Button and a CheckBox to check the Focus properties on controls.
        //------------------------------------------------------------------------
        void ChromeFocusRectangleTests::CheckFocusChromeVisuals()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::DottedLine);

            const auto& wh = TestServices::WindowHelper;
            const auto& kh = TestServices::KeyboardHelper;
            const auto& ih = TestServices::InputHelper;
            const auto& u = TestServices::Utilities;

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"FocusChromeRectangleTests.xaml"));
            Button^ noVisualButton = nullptr;
            Button^ treeEnterLeaveButton = nullptr;
            Grid^ contentGrid = nullptr;
            Border^ descendantBorder = nullptr;
            RunOnUIThread([&]()
            {
                wh->WindowContent = root;
                StackPanel^ stackPanel = ref new StackPanel();

                //Checks to see if UseSystemFocusVisuals can be disabled from code behind.
                noVisualButton = ref new Button();
                noVisualButton->Content = "Disable property by code";
                noVisualButton->UseSystemFocusVisuals = false;
                stackPanel->Children->Append(noVisualButton);

                //Checks to see if IsTemplateFocusTarget works on descendants at least 2 elements down.
                //Also Checks the EnterImpl call for when the property is set but the descendant doesn't
                //have an appropriate ancestor.
                treeEnterLeaveButton = ref new Button();
                treeEnterLeaveButton->Width = 60;
                treeEnterLeaveButton->Height = 30;
                contentGrid = ref new Grid();
                descendantBorder = ref new Border();
                contentGrid->Width = 10;
                contentGrid->Height = 10;
                descendantBorder->SetValue(Control::IsTemplateFocusTargetProperty, true);
                contentGrid->Children->Append(descendantBorder);
                treeEnterLeaveButton->Content = contentGrid;
                stackPanel->Children->Append(treeEnterLeaveButton);

                root->Children->Append(stackPanel);
            });
            // Initially there were issues with the Dcomp starting before the scroll viewer finished loading.
            // This would cause the .xml masters to change intermittently and cause failures. This ensures that
            // the scroll viewer is loaded before continuing.
            wh->SynchronouslyTickUIThread(2);
            wh->WaitForIdle();
            ih->Tap(root);
            wh->WaitForIdle();

            // Set the surface ID mode after having rendered a frame, when the MockDComp device has for sure been created.
            SurfaceIdModeScopeGuard crc(MockDComp::SurfaceIdMode::CRC);

            //Button with the UseSystemFocusVisuals set from xaml, old focus state removed in Generic.xaml
            //Should display an orange button with a focus visual around the entire control when focused.
            kh->Tab();
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome1");

            //Button with it's content set to a grid. The grid has the IsTemplateFocusTarget attached property set.
            //Should display a button with the grid inside of it receiving the focus visual when the control is focused.
            kh->Tab();
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome2");

            //CheckBox which does not have it's UseSystemFocusVisuals set in xaml. Should display the Checkbox with
            //it's focus state visuals and no chromed visuals.
            kh->Tab();
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome3");

            //Button with it's UseSystemFocusVisuals disabled from the code behind, should display a button with no visual
            //indication when it is in focus.
            kh->Tab();
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome4");

            //Buttone with a border that has the IsTemplateFocusTarget attached property set before being added to the
            //visual tree. Tests the EnterImpl code to verify that the child will be the target of focus visuals when the
            //property is set but the target does not yet have any ancestors. Should display a button with the focus
            //visual around the border inside the grid set as content.
            kh->Tab();
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome5");

            //Removes the grid from the visual tree, testing the LeaveImpl call to make sure a control will draw focus around
            //itself if it's target descendant leaves the visual tree.
            RunOnUIThread([&]()
            {
                treeEnterLeaveButton->Content = nullptr;
            });
            //Cycles through all the tests once more, and verifies the LeaveImpl case.
            kh->Tab();
            kh->Tab();
            kh->Tab();
            kh->Tab();
            kh->Tab();
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome6");

            //Tests that the focus visual can be called with the Focus() function for tests. Enables the
            //visuals on the button that had them disabled.
            RunOnUIThread([&]()
            {
                noVisualButton->Content = "Enable property by code";
                noVisualButton->UseSystemFocusVisuals = true;
                noVisualButton->Focus(xaml::FocusState::Keyboard);
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"FocusChrome7");
        }

        void ChromeFocusRectangleTests::HighVisibilityFocusVisualsWUCFull()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20'>"
                    L"    <Button>Normal Button</Button>"
                    L"    <Button x:Name='specialButton'>Special Button</Button>"
                    L"    <Button>Normal Button</Button>"
                    L"</StackPanel>"));

                button = safe_cast<Button^>(root->FindName("specialButton"));
                button->FocusVisualMargin = ThicknessHelper::FromUniformLength(0.0);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Navy);
                button->FocusVisualPrimaryThickness = ThicknessHelper::FromUniformLength(2.0);
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                button->FocusVisualSecondaryThickness = ThicknessHelper::FromUniformLength(1.0);
                button->Content = "Button with Focus";

                TestServices::WindowHelper->WindowContent = root;
            });

            // Initially there were issues with the Dcomp starting before the scroll viewer finished loading.
            // This would cause the .xml masters to change intermittently and cause failures. This ensures that
            // the scroll viewer is loaded before continuing.
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"internal_focusrect");

            RunOnUIThread([&]()
            {
                button->FocusVisualMargin = ThicknessHelper::FromUniformLength(-10.0);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Navy);
                button->FocusVisualPrimaryThickness = Thickness({2.0, 3.0, 4.0, 5.0});
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                button->FocusVisualSecondaryThickness = Thickness({5.0, 4.0, 3.0, 2.0});
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"external_focusrect");

            // At one point there was a bug where the focus rect showed an intermediate frame that was cleaned up on the next
            // tick.  Make sure after a couple more ticks the scene is the exact same.
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"external_focusrect");

            RunOnUIThread([&]()
            {
                button->FocusVisualMargin = ThicknessHelper::FromUniformLength(-10.0);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Navy);
                button->FocusVisualPrimaryThickness = Thickness({20.0, 30.0, 40.0, 500.0});
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                button->FocusVisualSecondaryThickness = Thickness({50.0, 400.0, 30.0, 20.0});
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"large_thicknesses");
        }

        void ChromeFocusRectangleTests::PopupUnderPopup()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Panel^ root;
            Button^ button1,^ button2;
            Popup^ popup1;
            Popup^ popup2;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "<Popup x:Name='popup1'>"
                    "    <StackPanel Margin='40'>"
                    "        <Button x:Name='button1'>Obscured button 1</Button>"
                    "        <Button x:Name='button2'>Obscured button 2</Button>"
                    "    </StackPanel>"
                    "</Popup>"
                    "<Popup x:Name='popup2'>"
                    "    <Grid Background='#eeffff55' IsHitTestVisible='False' Width='100' Height='100'/>"
                    "</Popup>"
                    "</Grid>"));
                button1 = safe_cast<Button^>(root->FindName("button1"));
                button2 = safe_cast<Button^>(root->FindName("button2"));
                popup1 = safe_cast<Popup^>(root->FindName("popup1"));
                popup2 = safe_cast<Popup^>(root->FindName("popup2"));
                button2->FocusVisualMargin = ThicknessHelper::FromUniformLength(-10.0);
                button2->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
                button2->FocusVisualPrimaryThickness = ThicknessHelper::FromUniformLength(3.0);
                button2->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Red);
                button2->FocusVisualSecondaryThickness = ThicknessHelper::FromUniformLength(2.0);
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            RunOnUIThread([&]()
            {
                popup1->IsOpen = true;
                popup2->IsOpen = true;
                button2->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }


        void ChromeFocusRectangleTests::FocusTargetDescendent()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20'>"
                    "<Button x:Name='button' FocusVisualPrimaryThickness='8' FocusVisualSecondaryThickness='9' >"
                    "    <Button.Template>"
                    "        <ControlTemplate>"
                    "            <Rectangle Width='200' Height='50' Fill='Blue' Control.IsTemplateFocusTarget='True' "
                    "               FocusVisualMargin='10' FocusVisualPrimaryThickness='4' FocusVisualSecondaryThickness='5' />"
                    "        </ControlTemplate>"
                    "    </Button.Template>"
                    "</Button>"
                    "</StackPanel>"));
                button = safe_cast<Button^>(root->FindName("button"));
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Red);
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void ChromeFocusRectangleTests::NudgeInsideScrollViewer()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);
            NudgeInsideScrollViewerHelper(L"");
        }

        void ChromeFocusRectangleTests::NudgeInsideScrollViewerWithoutScrollbars()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);
            NudgeInsideScrollViewerHelper(L"Template='{StaticResource ScrollViewerScrollBarlessTemplate}'");
        }

        void ChromeFocusRectangleTests::NudgeInsideScrollViewerHelper(
            const wchar_t* const scrollViewerTemplate,
            const NudgeInsideScrollViewerArgs& args)
        {
            Panel^ root;
            Button^ button;
            ScrollViewer^ scrollViewer;
            Canvas^ canvas;
            RunOnUIThread([&]()
            {
                const wchar_t* const formatString =
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<ScrollViewer Width='200' Height='400' Margin='10' x:Name='scrollViewer' %s>"
                    L"    <Canvas x:Name='canvas' Width='200' Height='400'>"
                    L"        <Button x:Name='button'>"
                    L"            <Button.Template>"
                    L"                <ControlTemplate>"
                    L"                    <Rectangle Fill='Navy' Width='50' Height='50' />"
                    L"                </ControlTemplate>"
                    L"            </Button.Template>"
                    L"        </Button>"
                    L"    </Canvas>"
                    L"</ScrollViewer>"
                    L"</StackPanel>";

                const unsigned int xamlStringBufferSize = static_cast<unsigned int>(wcslen(formatString) + wcslen(scrollViewerTemplate) + 2);
                wchar_t* xamlString = new wchar_t[xamlStringBufferSize];
                StringCchPrintf(
                    xamlString,
                    xamlStringBufferSize,
                    formatString,
                    scrollViewerTemplate);

                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(Platform::StringReference(xamlString)));

                delete[] xamlString;

                button = safe_cast<Button^>(root->FindName("button"));
                scrollViewer = safe_cast<ScrollViewer^>(root->FindName("scrollViewer"));
                canvas = safe_cast<Canvas^>(root->FindName("canvas"));
                canvas->Background = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(args.CanvasBackground);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(args.PrimaryColor);
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(args.SecondaryColor);
                button->FocusVisualPrimaryThickness = args.PrimaryThickness;
                button->FocusVisualSecondaryThickness = args.SecondaryThickness;
                button->FocusVisualMargin = args.Margin;
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Place focus on button");
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "NonScrollable");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Shrink ScrollViewer to make it scroll");
                scrollViewer->Height = 200.0;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "TopLeft");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Place Button at top right of ScrollViewer");
                button->SetValue(Canvas::LeftProperty, 150.0);
                button->SetValue(Canvas::TopProperty, 0.0);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "TopRight");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Place Button at bottom right of ScrollViewer, but there's still more content below the ScrollViewer's viewport");
                button->SetValue(Canvas::LeftProperty, 150.0);
                button->SetValue(Canvas::TopProperty, 150.0);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "BottomRightClipped");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Place Button at bottom right of ScrollViewer, and the ScrollViewer is scrolled almost to the bottom");
                button->SetValue(Canvas::LeftProperty, 150.0);
                button->SetValue(Canvas::TopProperty, 350.0);
                scrollViewer->ScrollToVerticalOffset(190.0);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Place Button at bottom right of ScrollViewer, and the ScrollViewer is scrolled to the bottom");
                scrollViewer->ScrollToVerticalOffset(200.0);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2); // Wait for vertical scrollbar to start disappearing
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "BottomRightNudged");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Place Button in middle of ScrollViewer, but at top of window, to ensure that it's nudged by the window");
                scrollViewer->ScrollToVerticalOffset(0.0);
                button->SetValue(Canvas::LeftProperty, 100.0);
                button->SetValue(Canvas::TopProperty, 100.0);
                scrollViewer->Margin = ThicknessHelper::FromLengths(10.0, -100.0, 10.0, 10.0);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2); // Wait for vertical scrollbar to start disappearing
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "WindowTop");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Position scene to nudge button on the top by window's bounds and bottom by canvas's bounds");
                scrollViewer->ScrollToVerticalOffset(200.0);
                button->SetValue(Canvas::LeftProperty, 100.0);
                button->SetValue(Canvas::TopProperty, 350.0);
                scrollViewer->Margin = ThicknessHelper::FromLengths(10.0, -150.0, 10.0, 10.0);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2); // Wait for vertical scrollbar to start disappearing
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "DoubleNudge");
        }

        void ChromeFocusRectangleTests::NudgeInsideVisibleBounds()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);
            NudgeInsideVisibleBoundsHelper();
        }

        void ChromeFocusRectangleTests::NudgeInsideVisibleBoundsHelper(const NudgeInsideArgs& args)
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::Utilities->ShrinkApplicationViewVisibleBounds(false);
            });
            TestServices::Utilities->ShrinkApplicationViewVisibleBounds(true); // Shinks the bounds 100px at left and right, 40px at top and bottom

            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <Button x:Name='button'>"
                    "        <Button.Template>"
                    "            <ControlTemplate>"
                    "                <Rectangle Fill='Navy' Width='50' Height='50' />"
                    "            </ControlTemplate>"
                    "        </Button.Template>"
                    "    </Button>"
                    "</Canvas>"));
                button = safe_cast<Button^>(root->FindName("button"));
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(args.PrimaryColor);
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(args.SecondaryColor);
                button->FocusVisualPrimaryThickness = args.PrimaryThickness;
                button->FocusVisualSecondaryThickness = args.SecondaryThickness;
                button->FocusVisualMargin = args.Margin;
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);

                LOG_OUTPUT(L"Position button at top left corner of visible bounds");
                button->SetValue(Canvas::LeftProperty, 100.0);
                button->SetValue(Canvas::TopProperty, 40.0);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "TopLeft");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Position button 1,1 px just outside of top left corner of visible bounds");
                button->SetValue(Canvas::LeftProperty, 99.0);
                button->SetValue(Canvas::TopProperty, 39.0);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "TopLeftOutside");
        }


        void ChromeFocusRectangleTests::ShowFocusRectOnNewPage()
        {
            XamlRoot^ xamlRoot = nullptr;
            RunOnUIThread([&]()
            {
                xamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
            });

            // Before the page is constructed, make sure the last input device is not keyboard/gamepad/remote
            // so there won't be a focus rect.
            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::Touch, xamlRoot);

            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);
            TestCleanupWrapper cleanup([]()
            {
                // This test messes with last input device, set it back to default when we're done
                TestServices::WindowHelper->SendWindowMessage(WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0, nullptr);
            });

            Platform::String^ xaml =
                "<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "<StackPanel Padding='50'>"
                "    <Button x:Name='button'>"
                "        <Button.Template>"
                "            <ControlTemplate>"
                "                <Rectangle Fill='Navy' Width='50' Height='50' />"
                "            </ControlTemplate>"
                "        </Button.Template>"
                "    </Button>"
                "</StackPanel>"
                "</Page>";

            Page^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Navigate to initial page");
                TestServices::WindowHelper->WindowContent = safe_cast<Page^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "FocusRectNotVisible");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Send window message to clear hide-focus flag");
                TestServices::WindowHelper->SendWindowMessage(WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0, nullptr);

                LOG_OUTPUT(L"Navigate to second page");
                TestServices::WindowHelper->WindowContent = safe_cast<Page^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "FocusRectVisible");

            {
                auto clickRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::Button, Click);
                auto clickEvent = std::make_shared<Event>();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Send window message to set hide-focus flag");
                    TestServices::WindowHelper->SendWindowMessage(WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0, nullptr);

                    root = safe_cast<Page^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                    button = safe_cast<Button^>(root->FindName(L"button"));
                    clickRegistration.Attach(button, [&]() { clickEvent->Set(); });

                    LOG_OUTPUT(L"Navigate to third page");
                    TestServices::WindowHelper->WindowContent = root;
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "FocusRectNotVisible");

                LOG_OUTPUT(L"Press enter to set last input device to keyboard");
                TestServices::KeyboardHelper->Enter();
                clickEvent->WaitForDefault();
            }

            // Force re-creation of the mock device to reset the TransformParent counter.
            TestServices::WindowHelper->ResetDeviceAndVisuals();
            TestServices::WindowHelper->WaitForIdle();

            {
                auto clickRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::Button, Click);
                auto clickEvent = std::make_shared<Event>();

                RunOnUIThread([&]()
                {
                    root = safe_cast<Page^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                    button = safe_cast<Button^>(root->FindName(L"button"));
                    clickRegistration.Attach(button, [&]() { clickEvent->Set(); });

                    LOG_OUTPUT(L"Navigate to fourth page");
                    TestServices::WindowHelper->WindowContent = root;
                });
                TestServices::WindowHelper->WaitForIdle();
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "FocusRectVisible");

                LOG_OUTPUT(L"Click to set last focus input to mouse");
                TestServices::InputHelper->LeftMouseClick(button);
                clickEvent->WaitForDefault();
            }

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Navigate to fifth page");
                TestServices::WindowHelper->WindowContent = safe_cast<Page^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "FocusRectNotVisible");

        }

        void ChromeFocusRectangleTests::SimpleHyperlink()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "<Button x:Name='button'>Button just for focus</Button>"
                "<RichTextBlock x:Name='richTextBlock' TextWrapping='WrapWholeWords'>"
                "    <Paragraph>"
                "        <Hyperlink x:Name='hyperlink' NavigateUri='http://microsoft.com/dontvisit'>microsoft.com</Hyperlink>"
                "    </Paragraph>"
                "</RichTextBlock>"
                "</StackPanel>";
            Panel^ root;
            Button^ button;
            Hyperlink^ hyperlink;
            RichTextBlock^ richTextBlock;

            auto gotFocusRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, GotFocus);
            auto gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                hyperlink = safe_cast<Hyperlink^>(root->FindName(L"hyperlink"));
                button = safe_cast<Button^>(root->FindName(L"button"));
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));

                gotFocusRegistration.Attach(richTextBlock, [gotFocusEvent]() { gotFocusEvent->Set(); });

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShowFocusRect");

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ShowFocusRect");

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->Reset();

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->MouseButtonDown(richTextBlock, 0, 0, MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NoFocusRect");
            TestServices::InputHelper->MoveMouse(button);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->MouseButtonUp(richTextBlock, 0, 0, MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();
        }

        void ChromeFocusRectangleTests::SplitHyperlink()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "<Button x:Name='button'>Button just for focus</Button>"
                "<RichTextBlock  TextWrapping='WrapWholeWords' Width='200'>"
                "    <Paragraph>"
                "        <Hyperlink x:Name='hyperlink' NavigateUri='http://microsoft.com/dontvisit'>this is a hyperlink to a url.  It has a super long name on purpose to test the focus rectangle.</Hyperlink>"
                "    </Paragraph>"
                "</RichTextBlock>"
                "</StackPanel>";

            Panel^ root;
            Button^ button;
            Hyperlink^ hyperlink;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                hyperlink = safe_cast<Hyperlink^>(root->FindName(L"hyperlink"));
                button = safe_cast<Button^>(root->FindName(L"button"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void ChromeFocusRectangleTests::HyperlinkTapped()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::DottedLine);

            auto clickRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Documents::Hyperlink, Click);
            auto clickEvent = std::make_shared<Event>();

            Platform::String^ xaml =
                "<StackPanel Margin='0,25,0,0' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "<RichTextBlock x:Name='richTextBlock'>"
                "    <Paragraph>"
                "        other words <Hyperlink x:Name='hyperlink'>Test with high-visibility</Hyperlink> other words"
                "    </Paragraph>"
                "</RichTextBlock>"
                "</StackPanel>";

            Panel^ root;
            Hyperlink^ hyperlink;
            RichTextBlock^ richTextBlock;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                hyperlink = safe_cast<Hyperlink^>(root->FindName(L"hyperlink"));
                richTextBlock = safe_cast<RichTextBlock^>(root->FindName(L"richTextBlock"));

                clickRegistration.Attach(hyperlink, [&]() { clickEvent->Set(); });

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->Tap(richTextBlock);

            clickEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NoDottedLine");

            RunOnUIThread([&]()
            {
                Microsoft::UI::Xaml::Application::Current->FocusVisualKind = FocusVisualKind::HighVisibility;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"NoHighVisibility");
        }

        void ChromeFocusRectangleTests::AnimatingFocusedElement()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Panel^ root;
            Button^ button;
            TranslateTransform^ buttonTransform;
            Storyboard^ storyboard;
            StackPanel^ stackPanel;
            auto storyboardStartedEvent = std::make_shared<Event>();

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "<ScrollViewer>"
                "  <StackPanel x:Name='stackPanel' Width='350' Height='350'>"
                "    <Button x:Name='button' Margin='10'>"
                "        <Button.RenderTransform><TranslateTransform x:Name='buttonTransform' X='0.0'/></Button.RenderTransform>"
                "        <Button.Template>"
                "            <ControlTemplate>"
                "                <Rectangle Fill='Navy' Width='50' Height='50' />"
                "            </ControlTemplate>"
                "        </Button.Template>"
                "    </Button>"
                "  </StackPanel>"
                "</ScrollViewer>"
                "</StackPanel>";

            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                button = safe_cast<Button^>(root->FindName("button"));
                buttonTransform = safe_cast<TranslateTransform^>(root->FindName("buttonTransform"));
                stackPanel = safe_cast<StackPanel^>(root->FindName("stackPanel"));
                TestServices::WindowHelper->WindowContent = root;

                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            XamlRoot^ xamlRoot = nullptr;
            RunOnUIThread([&]()
            {
                xamlRoot = root->XamlRoot;
            });

            TestServices::WindowHelper->SetLastInputMethod(test_infra::LastInputDeviceType::Keyboard, xamlRoot);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Start animation on button.  The focus rect should disappear because we don't support animations between the focus host and the focus target");

                ::Windows::Foundation::TimeSpan span;

                DoubleAnimation^ da1 = ref new DoubleAnimation();
                da1->From = 10.0; da1->To = 11.0;
                span.Duration = 1000000000L; da1->Duration = DurationHelper::FromTimeSpan(span);
                Storyboard::SetTarget(da1, buttonTransform);
                Storyboard::SetTargetProperty(da1, L"X");

                storyboard = ref new Storyboard();
                storyboard->Children->Append(da1);

                storyboard->Begin();

                storyboardStartedEvent->Set();
            });

            // Wait until the animation is underway
            storyboardStartedEvent->WaitForDefault();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

            RunOnUIThread([&]()
            {
                storyboard->Stop();

                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                button = safe_cast<Button^>(root->FindName("button"));
                buttonTransform = safe_cast<TranslateTransform^>(root->FindName("buttonTransform"));
                stackPanel = safe_cast<StackPanel^>(root->FindName("stackPanel"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
                LOG_OUTPUT(L"Start animation on the StackPanel.  The focus rect should remain visible.");

                ::Windows::Foundation::TimeSpan span;

                TranslateTransform^ translateTransform = ref new TranslateTransform();
                stackPanel->RenderTransform = translateTransform;

                DoubleAnimation^ da1 = ref new DoubleAnimation();
                da1->From = 20.0; da1->To = 21.0;
                span.Duration = 1000000000L; da1->Duration = DurationHelper::FromTimeSpan(span);
                Storyboard::SetTarget(da1, translateTransform);
                Storyboard::SetTargetProperty(da1, L"X");

                storyboard = ref new Storyboard();
                storyboard->Children->Append(da1);

                storyboard->Begin();

                storyboardStartedEvent->Set();
            });

            // Wait until the animation is underway
            storyboardStartedEvent->WaitForDefault();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                storyboard->Stop();
            });
            // Wait until the animation has ended
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
        }

        void ChromeFocusRectangleTests::ScaledFocusedElement()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <StackPanel.RenderTransform><ScaleTransform ScaleX='4' ScaleY='4'/></StackPanel.RenderTransform>"
                    "    <Button x:Name='button' Margin='10'>"
                    "        <Button.Template>"
                    "            <ControlTemplate>"
                    "                <Rectangle Fill='Navy' Width='50' Height='50' />"
                    "            </ControlTemplate>"
                    "        </Button.Template>"
                    "    </Button>"
                    "</StackPanel>"
                    ));
                button = safe_cast<Button^>(root->FindName(L"button"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void ChromeFocusRectangleTests::RenderTransformOfAncestorChanged()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility);

            Panel^ root;
            Button^ button;
            TranslateTransform^ translateTransform;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "  <Grid>"
                    "    <Grid.RenderTransform><TranslateTransform x:Name='translateTransform'/></Grid.RenderTransform>"
                    "    <Button x:Name='button' Margin='10'>"
                    "        <Button.Template>"
                    "            <ControlTemplate>"
                    "                <Rectangle Fill='Navy' Width='50' Height='50' />"
                    "            </ControlTemplate>"
                    "        </Button.Template>"
                    "    </Button>"
                    "  </Grid>"
                    "</StackPanel>"));
                button = safe_cast<Button^>(root->FindName(L"button"));
                translateTransform = safe_cast<TranslateTransform^>(root->FindName(L"translateTransform"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                translateTransform->X = 20.0;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void ChromeFocusRectangleTests::RaiseExceptionWhenSettingBrushToNonSolidColorBrush()
        {
            TestCleanupWrapper testCleanup;

            Button^ btn = nullptr;

            RunOnUIThread([&]()
            {
                StackPanel^ mainStackPanel = ref new StackPanel();

                btn = ref new Button();
                btn->Width = 200;
                btn->Height = 200;
                btn->Content = "Button";
                btn->HorizontalAlignment = HorizontalAlignment::Center;

                mainStackPanel->Children->Append(btn);
                TestServices::WindowHelper->WindowContent = mainStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_THROWS_WINRT(btn->FocusVisualPrimaryBrush = ref new LinearGradientBrush(), Platform::InvalidArgumentException^);
            });
        }

        void ChromeFocusRectangleTests::FocusOnCommandBar()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);

            Panel^ root;
            AppBarButton^ button;
            CommandBar^ commandBar;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <CommandBar x:Name='commandBar'>"
                    "        <AppBarButton x:Name='button' Icon='Setting' Label='Settings' />"
                    "    </CommandBar>"
                    "</StackPanel>"));
                button = safe_cast<AppBarButton^>(root->FindName(L"button"));
                commandBar = safe_cast<CommandBar^>(root->FindName(L"commandBar"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto moreButton = TreeHelper::GetVisualChildByName(commandBar, L"MoreButton");
                xaml_controls::ToolTipService::SetToolTip(moreButton, nullptr);

                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"closed");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Open the CommandBar");
                commandBar->IsOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"open");

            LOG_OUTPUT(L"Press tab to get to ... button");
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"more_button");
        }

        void ChromeFocusRectangleTests::FocusOnVariousControls()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);
            unsigned int controlCount = 0;

            Grid^ grid;
            RunOnUIThread([&]()
            {
                grid = safe_cast<Grid^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='10'>"
                    "    <Button>I am a button</Button>"
                    "    <ComboBox><ComboBoxItem>Item 1</ComboBoxItem></ComboBox>"
                    "    <ToggleButton>ToggleButton</ToggleButton>"
                    "    <ToggleSwitch>ToggleSwitch</ToggleSwitch>"
                    "    <CheckBox>CheckBox</CheckBox>"
                    "    <RadioButton>RadioButton</RadioButton>"
                    "    <Slider IsThumbToolTipEnabled='False' />"
                    "    <HyperlinkButton>HyperlinkButton</HyperlinkButton>"
                    "</Grid>"));
                TestServices::WindowHelper->WindowContent = grid;

                controlCount = grid->Children->Size;
                for (unsigned int i = 0; i < controlCount; ++i)
                {
                    grid->Children->GetAt(i)->Visibility = Visibility::Collapsed;
                }
            });
            TestServices::WindowHelper->WaitForIdle();

            for (unsigned int i = 0; i < controlCount; ++i)
            {
                if (i == 6)
                {
                    // Slider has been difficult to get right in the masters because of its tooltip.  Just skip it.
                    continue;
                }
                RunOnUIThread([&]()
                {
                    grid->Children->GetAt(i)->Visibility = Visibility::Visible;
                    safe_cast<Control^>(grid->Children->GetAt(i))->Focus(FocusState::Keyboard);
                });
                TestServices::WindowHelper->WaitForIdle();

                wchar_t numberString[20] = {};
                ::StringCchPrintf(numberString, ARRAYSIZE(numberString), L"control_%d", i);
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, ref new Platform::String(numberString));

                RunOnUIThread([&]()
                {
                    grid->Children->GetAt(i)->Visibility = Visibility::Collapsed;
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }

        void ChromeFocusRectangleTests::DoNotSuppressFocusRectangleOnDesktop()
        {
            // DoNotSupressFocusRectangleOnDesktop and SupressFocusRectangleOnPhone could actually be the same
            // test method with different masters files.  However, to be explicit about the intended difference
            // of functionality, they are separated.  Same goes for the cases when BringIntoView is handled.
            FocusRectangleSuppressionTest(false);
        }

        void ChromeFocusRectangleTests::SuppressFocusRectangleOnPhone()
        {
            FocusRectangleSuppressionTest(true);
        }

        void ChromeFocusRectangleTests::DoNotSuppressFocusRectangleOnDesktopBringIntoViewHandled()
        {
            FocusRectangleSuppressionTest(false, true);
        }

        void ChromeFocusRectangleTests::SuppressFocusRectangleOnPhoneBringIntoViewHandled()
        {
            FocusRectangleSuppressionTest(true, true);
        }

        void ChromeFocusRectangleTests::FocusRectangleSuppressionTest(bool shouldSIPShow, bool shouldHandleBringIntoView)
        {
            ::Windows::Foundation::EventRegistrationToken inputPaneShowToken = {};
            ::Windows::Foundation::EventRegistrationToken inputPaneHideToken = {};

            FocusVisualKind previousFocusVisualKind;

            ::Windows::UI::ViewManagement::InputPane^ sip = nullptr;

            // TODO_WinRT:  These tests have reliability issues in WUC mode.  Tracked by 8218705
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 400.0f));

            KeyboardInjectionIgnoreEventWaitOverride injectionOverride;

            StackPanel^ sp = nullptr;
            Button^ btn = nullptr;
            TextBox^ tb = nullptr;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto textboxGotFocusEvent = std::make_shared<Event>();
            auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            auto buttonLoadedEvent = std::make_shared<Event>();
            auto buttonLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);

            auto SIPShowingEvent = std::make_shared<Event>();
            auto SIPHidingEvent = std::make_shared<Event>();

            // Since InputPane is not agile, we can't use SafeEventRegistration. We need to manage the SIP events manually.
            // This cleanup object must remain after the SIPShowingEvent/SIPHidingEvent variables so we unhook the SIP event
            // handlers before the SIPShowingEvent/SIPHidingEvent are uninitialized
            TestCleanupWrapper cleanup([&]()
            {
                RunOnUIThread([&inputPaneShowToken, &inputPaneHideToken, &shouldSIPShow, &previousFocusVisualKind, SIPShowingEvent, SIPHidingEvent]()
                {
                    if (shouldSIPShow)
                    {
                        auto inputPane = TestServices::WindowHelper->GetInputPaneForMainView();
                        inputPane->Showing -= inputPaneShowToken;
                        inputPane->Hiding -= inputPaneHideToken;
                        LOG_OUTPUT(L"InputPane event handlers removed");

                        // Make sure the event objects are still alive at this point
                        SIPShowingEvent->Set();
                        SIPHidingEvent->Set();
                    }

                    Microsoft::UI::Xaml::Application::Current->FocusVisualKind = previousFocusVisualKind;
                });
                TestServices::WindowHelper->WaitForIdle();
            });

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
                previousFocusVisualKind = Microsoft::UI::Xaml::Application::Current->FocusVisualKind;
                Microsoft::UI::Xaml::Application::Current->FocusVisualKind = FocusVisualKind::HighVisibility;
            });

            RunOnUIThread([&]()
            {

                sp = safe_cast<StackPanel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <Button x:Name='btn'/>"
                    "    <TextBox x:Name='tb'/>"
                    "</StackPanel>"));
                TestServices::WindowHelper->WindowContent = sp;

                btn = safe_cast<Button^>(sp->FindName(L"btn"));
                tb = safe_cast<TextBox^>(sp->FindName(L"tb"));

                btn->Content = L"Test Button";
                btn->Margin = ThicknessHelper::FromUniformLength(10);

                tb->Width = 250;
                tb->Height = 50;
                tb->Margin = ThicknessHelper::FromUniformLength(10);

                buttonGotFocusRegistration.Attach(btn, [&]() {buttonGotFocusEvent->Set(); });
                textboxGotFocusRegistration.Attach(tb, [&]() {textboxGotFocusEvent->Set(); });
                buttonLoadedRegistration.Attach(btn, [&]() {buttonLoadedEvent->Set(); });

                btn->Focus(FocusState::Pointer);
            });
            buttonLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            if (shouldSIPShow)
            {
                RunOnUIThread([&]()
                {
                    sip = TestServices::WindowHelper->GetInputPaneForMainView();
                    inputPaneShowToken = sip->Showing += ref new wf::TypedEventHandler<wuv::InputPane^, wuv::InputPaneVisibilityEventArgs^>
                        ([&](wuv::InputPane^ pane, wuv::InputPaneVisibilityEventArgs^ e)
                    {
                        e->EnsuredFocusedElementInView = shouldHandleBringIntoView;
                        SIPShowingEvent->Set();
                    });

                    inputPaneHideToken = sip->Hiding += ref new wf::TypedEventHandler<wuv::InputPane^, wuv::InputPaneVisibilityEventArgs^>
                        ([&](wuv::InputPane^ pane, wuv::InputPaneVisibilityEventArgs^ e)
                    {
                        e->EnsuredFocusedElementInView = shouldHandleBringIntoView;
                        SIPHidingEvent->Set();
                    });
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            //Bring Up the SIP
            TestServices::InputHelper->Tap(tb);
            textboxGotFocusEvent->WaitForDefault();
            if (shouldSIPShow)
            {
                SIPShowingEvent->WaitForDefault();
            }

            // Start actual test scenarios:
            // 1: type a letter into the textbox then move to the button.
            TestServices::KeyboardHelper->PressKeySequence(L"A");
            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Programmatic);
            });
            buttonGotFocusEvent->WaitForDefault();
            if (shouldSIPShow)
            {
                SIPHidingEvent->WaitForDefault();
            }
            // Should not see any FocusRectangles on phone, but they should be shown on desktop
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

            TestServices::InputHelper->Tap(tb);
            textboxGotFocusEvent->WaitForDefault();
            if (shouldSIPShow)
            {
                SIPShowingEvent->WaitForDefault();
            }

            TestServices::KeyboardHelper->Tab();
            buttonGotFocusEvent->WaitForDefault();
            // Should see focus rectangle on both phone and desktop
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

            TestServices::InputHelper->Tap(tb);
            textboxGotFocusEvent->WaitForDefault();
            if (shouldSIPShow)
            {
                SIPShowingEvent->WaitForDefault();
            }

            TestServices::KeyboardHelper->Left();
            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Programmatic);
            });
            buttonGotFocusEvent->WaitForDefault();
            if (shouldSIPShow)
            {
                SIPHidingEvent->WaitForDefault();
            }
            // Should see focus rectangle on all platforms - verify with mockDComp.
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

            TestServices::WindowHelper->WaitForIdle();
        }

        void ChromeFocusRectangleTests::HonorLayoutClip()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);

            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel Margin='10' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <Button x:Name='button'>"
                    "        <Button.Template>"
                    "            <ControlTemplate>"
                    "                <Rectangle Fill='Navy' Width='100' Height='100' />"
                    "            </ControlTemplate>"
                    "        </Button.Template>"
                    "    </Button>"
                    "</StackPanel>"
                    ));
                button = safe_cast<Button^>(root->FindName(L"button"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                root->Height = 50.0;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void ChromeFocusRectangleTests::FocusRectangleOnOpacity()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);

            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel Margin='10' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <Button x:Name='button'/>"
                    "</StackPanel>"
                ));
                button = safe_cast<Button^>(root->FindName(L"button"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");
            RunOnUIThread([&]()
            {
                button->Opacity = 0.5;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                button->Opacity = 0.0;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
        }

        void ChromeFocusRectangleTests::StickyHeaders()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);

            Panel^ root = nullptr;
            ListView^ listView = nullptr;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "    <Button>First Focus</Button>"
                    "    <ListView x:Name='listView' Margin='20' Padding='20' Width='300' Height='300'>"
                    "        <ListView.GroupStyle>"
                    "            <GroupStyle>"
                    "                <GroupStyle.HeaderTemplate>"
                    "                    <DataTemplate>"
                    "                        <Grid Background='Red' Margin='2'>"
                    "                            <Button>"
                    "                                <TextBlock Text='Header'/>"
                    "                            </Button>"
                    "                        </Grid>"
                    "                    </DataTemplate>"
                    "                </GroupStyle.HeaderTemplate>"
                    "            </GroupStyle>"
                    "        </ListView.GroupStyle>"
                    "    </ListView>"
                    "</StackPanel>"
                    ));
                listView = safe_cast<ListView^>(root->FindName(L"listView"));

                auto groups = ref new Platform::Collections::Vector<Platform::Object^>;

                for (int i = 0; i < 3; ++i)
                {
                    auto newGroup = ref new Platform::Collections::Vector<Platform::String^>;
                    groups->Append(newGroup);
                    for (int j = 0;j < 5;++j)
                    {
                        newGroup->Append(Platform::StringReference(L"item"));
                    }
                }

                auto cvs = ref new Microsoft::UI::Xaml::Data::CollectionViewSource();
                cvs->IsSourceGrouped = true;
                cvs->Source = groups;

                listView->ItemsSource = cvs->View;

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"header_focus");

            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"button_focus");
        }

        void ChromeFocusRectangleTests::FocusRectangleRendersBehindPopup()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::HighVisibility, 400.0f, 400.0f);

            Panel^ root;
            Button^ button;
            Popup^ popup;

            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    "<StackPanel Margin='10' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "   <Popup IsOpen='False' x:Name='popup'>"
                    "       <StackPanel Width='200' Height='300' Background='Red'>"
                    "           <TextBlock Text='Hello' />"
                    "           <Button Content = 'Just for show' />"
                    "       </StackPanel>"
                    "   </Popup>"
                    "   <Button x:Name='btn' Content='Pointless button with a really good purpose' />"
                    "</StackPanel>"
                ));
                button = safe_cast<Button^>(root->FindName(L"btn"));
                popup = safe_cast<Popup^>(root->FindName(L"popup"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                popup->IsOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(button, FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");
        }

        void ChromeFocusRectangleTests::FocusDeviceLost()
        {
            const auto& wh = TestServices::WindowHelper;
            const Microsoft::UI::Xaml::FocusVisualKind types[] = { Microsoft::UI::Xaml::FocusVisualKind::HighVisibility, Microsoft::UI::Xaml::FocusVisualKind::Reveal};
            for (auto type : types)
            {
                FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, type, 400.0f, 400.0f);

                Grid^ grid;
                RunOnUIThread([&]()
                {
                    grid = safe_cast<Grid^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                        "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='10'>"
                        "    <Button>I am a button</Button>"
                        "</Grid>"));
                    wh->WindowContent = grid;
                });
                wh->WaitForIdle();

                RunOnUIThread([&]()
                {
                    safe_cast<Control^>(grid->Children->GetAt(0))->Focus(FocusState::Keyboard);
                });
                wh->WaitForIdle();

                wh->SimulateDeviceLost();
                wh->WaitForIdle();
            }
        }

        void ChromeFocusRectangleTests::RevealFocusSetOnApp()
        {
            // Disable animations so that tests are consistent. Otherwise DComp validation can fail if animations are enabled
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, true);

            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal);
            Panel^ root;
            Button^ button;
            RunOnUIThread([&]()
            {
                root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20'>"
                    L"    <Button>Normal Button</Button>"
                    L"    <Button x:Name='specialButton'>Special Button</Button>"
                    L"    <Button>Normal Button</Button>"
                    L"</StackPanel>"));

                button = safe_cast<Button^>(root->FindName("specialButton"));
                button->FocusVisualMargin = ThicknessHelper::FromUniformLength(0.0);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Navy);
                button->FocusVisualPrimaryThickness = ThicknessHelper::FromUniformLength(2.0);
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                button->FocusVisualSecondaryThickness = ThicknessHelper::FromUniformLength(1.0);
                button->Content = "Button with Focus";

                TestServices::WindowHelper->WindowContent = root;
            });

            // Initially there were issues with the Dcomp starting before the scroll viewer finished loading.
            // This would cause the .xml masters to change intermittently and cause failures. This ensures that
            // the scroll viewer is loaded before continuing.
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"internal_focusrect");

            RunOnUIThread([&]()
            {
                button->FocusVisualMargin = ThicknessHelper::FromUniformLength(-10.0);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Navy);
                button->FocusVisualPrimaryThickness = Thickness({2.0, 3.0, 4.0, 5.0});
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                button->FocusVisualSecondaryThickness = Thickness({5.0, 4.0, 3.0, 2.0});
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"external_focusrect");

            // At one point there was a bug where the focus rect showed an intermediate frame that was cleaned up on the next
            // tick.  Make sure after a couple more ticks the scene is the exact same.
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"external_focusrect");

            RunOnUIThread([&]()
            {
                button->FocusVisualMargin = ThicknessHelper::FromUniformLength(-10.0);
                button->FocusVisualPrimaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Navy);
                button->FocusVisualPrimaryThickness = Thickness({20.0, 30.0, 40.0, 500.0});
                button->FocusVisualSecondaryBrush = ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange);
                button->FocusVisualSecondaryThickness = Thickness({50.0, 400.0, 30.0, 20.0});
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"large_thicknesses");
        }

        void ChromeFocusRectangleTests::NoRevealFocusOnHyperlink()
        {
            // Disable animations so that tests are consistent. Otherwise DComp validation can fail if animations are enabled
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, true);

            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal);

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "<Button x:Name='button'>Button just for focus</Button>"
                "<RichTextBlock x:Name='richTextBlock' TextWrapping='WrapWholeWords'>"
                "    <Paragraph>"
                "        <Hyperlink x:Name='hyperlink' NavigateUri='http://microsoft.com/dontvisit'>microsoft.com</Hyperlink>"
                "    </Paragraph>"
                "</RichTextBlock>"
                "</StackPanel>";

                Panel^ root;
                Button^ button;
                Hyperlink^ hyperlink;
                RunOnUIThread([&]()
                {
                    root = safe_cast<Panel^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml));
                    hyperlink = safe_cast<Hyperlink^>(root->FindName(L"hyperlink"));
                    button = safe_cast<Button^>(root->FindName(L"button"));
                    TestServices::WindowHelper->WindowContent = root;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    button->Focus(FocusState::Keyboard);
                });
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

        void ChromeFocusRectangleTests::RevealFocusBreathing()
        {
            // Enable animations so that tests are consistent and that we try to breathe
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableGlobalAnimations, true);

            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal);

            LOG_OUTPUT(L"Scenario 1. Enabled via 'Adequate power supply'");
            TestServices::Utilities->SetMockPowerSupplyStatus(PowerSupplyStatus::Adequate);

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "   <Button x:Name='button'>Button just for focus</Button>"
                "   <Button x:Name='button2'>Button also just for focus</Button>"
                "</StackPanel>";

            auto root = helper.LoadAndSetFocus(xaml, L"button");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Enabled");

            LOG_OUTPUT(L"Scenario 2. Disabled via enabling energy saver");
            TestServices::Utilities->SetMockEnergySaverStatus(EnergySaverStatus::On);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Disabled");

            LOG_OUTPUT(L"Scenario 3. Re-enabled via disabling energy saver");
            TestServices::Utilities->SetMockEnergySaverStatus(EnergySaverStatus::Off);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Enabled");

            LOG_OUTPUT(L"Scenario 4. Disabled via 'Inadequate power supply'");
            TestServices::Utilities->SetMockPowerSupplyStatus(PowerSupplyStatus::Inadequate);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Disabled");

            LOG_OUTPUT(L"Scenario 5. Disabled via 'NotPresent power supply'");
            TestServices::Utilities->SetMockPowerSupplyStatus(PowerSupplyStatus::NotPresent);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Disabled");

            LOG_OUTPUT(L"Scenario 6. Focus new element, make sure it also doesn't have the breathing animation");
            helper.FindAndSetFocus(root, L"button2");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Disabled2");

            LOG_OUTPUT(L"Scenario 7. Change power state so that it's enabled.");
            TestServices::Utilities->SetMockPowerSupplyStatus(PowerSupplyStatus::Adequate);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Enabled2");
        }

          void ChromeFocusRectangleTests::RevealFocusPressState()
        {
            // Enable animations so that tests are consistent and that we try to breathe
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableGlobalAnimations, true);

            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal);

            LOG_OUTPUT(L"Scenario 1. Enabled via 'Adequate power supply'");
            TestServices::Utilities->SetMockPowerSupplyStatus(PowerSupplyStatus::Adequate);

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "   <Button x:Name='button'>Hey now</Button>"
                "</StackPanel>";

            auto root = helper.LoadAndSetFocus(xaml, L"button");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Button_NoPress");

            TestServices::KeyboardHelper->Space();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Button_Press");

            TestServices::KeyboardHelper->Enter(); // For buttons, ENTER is also a click
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Button_Press");

        }

        void ChromeFocusRectangleTests::RevealFocusBorderlessNudgingInsideScrollViewer()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal, 400.0f, 400.0f);
            auto args = NudgeInsideScrollViewerArgs::Borderless(12, Microsoft::UI::Colors::Black);
            NudgeInsideScrollViewerHelper(L"", args);
        }

        void ChromeFocusRectangleTests::RevealFocusBorderlessNudgingInsideScrollViewerNoScrollbar()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal, 400.0f, 400.0f);
            auto args = NudgeInsideScrollViewerArgs::Borderless(12, Microsoft::UI::Colors::Black);
            NudgeInsideScrollViewerHelper(L"Template='{StaticResource ScrollViewerScrollBarlessTemplate}'", args);
        }

        void ChromeFocusRectangleTests::RevealFocusBorderlessNudgingInsideVisibleBounds()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal);
            auto args = NudgeInsideArgs::Borderless(12);
            NudgeInsideVisibleBoundsHelper(args);
        }

        void ChromeFocusRectangleTests::RevealFocusBorderlessWithShapeAsFocusTarget()
        {
            FocusRectSetupHelper helper(DCompRendering::WUCCompleteSynchronousCompTree, FocusVisualKind::Reveal);

            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "   <Button x:Name='button' Content='Button just for focus'>"
                "      <Button.Template>"
                "        <ControlTemplate TargetType='Button'>"
                "           <Rectangle Control.IsTemplateFocusTarget='True' Fill='Green'/>"
                "        </ControlTemplate>"
                "      </Button.Template>"
                "   </Button>"
                "</StackPanel>";

            auto root = helper.LoadAndSetFocus(xaml, L"button");
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"BorderlessRect");
        }
    }
} } } } }


