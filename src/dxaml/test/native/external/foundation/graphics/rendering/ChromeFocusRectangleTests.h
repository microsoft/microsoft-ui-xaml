// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class ChromeFocusRectangleTests : public WEX::TestClass<ChromeFocusRectangleTests>
        {
        private:
            Platform::String^ GetResourcesPath() const;
            void FocusRectangleSuppressionTest(bool shouldSIPShow, bool shouldHandleBringIntoView = false);
        public:
            BEGIN_TEST_CLASS(ChromeFocusRectangleTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;b7d949c9-6779-44c5-b16b-1f51e18f3866")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(HighVisibilityFocusVisualsWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Basic smoke test for high-visibility focus rectangles")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopupUnderPopup)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure the focus rect is drawn for the right popup")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusTargetDescendent)
                TEST_METHOD_PROPERTY(L"Description", L"We should get the focus visual properties from the descendent first if they exist")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NudgeInsideScrollViewer)
                TEST_METHOD_PROPERTY(L"Description", L"Test nudging logic inside a ScollViewer.  Focus rect should be nudged within the ScrollViewer and within the app")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NudgeInsideScrollViewerWithoutScrollbars)
                TEST_METHOD_PROPERTY(L"Description", L"Test nudging logic inside a ScollViewer that's been re-templated to remove scrollbars")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NudgeInsideVisibleBounds)
                TEST_METHOD_PROPERTY(L"Description", L"Focused element should be nudged inside the visible bounds of a window if it's on the edge")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Visual size mismatch
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RaiseExceptionWhenSettingBrushToNonSolidColorBrush)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when we set the brush to something other than SolidColorBrush, we fail in a way an app developer can see")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusOnVariousControls)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies focus on some different controls")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusRectangleOnOpacity)
                TEST_METHOD_PROPERTY(L"Description", L"Focus Rect LTE should update with the effective Opacity of the target")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusOnCommandBar)
                TEST_METHOD_PROPERTY(L"Description", L"Make sure focus rect works well with command bar")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36781575
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimatingFocusedElement)
                TEST_METHOD_PROPERTY(L"Description", L"Focus rect should be drawn for animating elements")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScaledFocusedElement)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(RenderTransformOfAncestorChanged)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HonorLayoutClip)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies Focus Rect rendering honors the layout clip applied to the element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ShowFocusRectOnNewPage)
                TEST_METHOD_PROPERTY(L"Description", L"If the last input device was from keyboard, focus rect should be visible on new pages")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SuppressFocusRectangleOnPhone)
                TEST_METHOD_PROPERTY(L"Description", L"If the last entered key was non navigational while SIP was shown, a focus rectangle should not be drawn")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotSuppressFocusRectangleOnDesktop)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the same conditions that cause focus rectangles to suppress their rendering on phone do not supress them on desktop")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Leftover composition target, offset mismatch
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"True")  // Investigate test instability in ChromeFocusRectangleTests
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SuppressFocusRectangleOnPhoneBringIntoViewHandled)
                TEST_METHOD_PROPERTY(L"Description", L"If the last entered key was non navigational while SIP was shown, a focus rectangle should not be drawn when an app handles BringIntoView responsibilities")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotSuppressFocusRectangleOnDesktopBringIntoViewHandled)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the same conditions that cause focus rectangles to suppress their rendering on phone do not supress them on desktop when an app handles BringIntoView responsibilities")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusRectangleRendersBehindPopup)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure a focus rectangle renders behind a popup when focus is on an item behind the popup")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            // Tests with isolation
            BEGIN_TEST_METHOD(StickyHeaders)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies Focus Rect renders correctly on sticky headers / ListViewItem headers")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // TODO: remove after the associated issue is fixed
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // BuildTreeService drains endlessly in a loop
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckFocusChromeVisuals)
                TEST_METHOD_PROPERTY(L"Description", L"Renders some controls then moves between them with keyboard focus.")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // TODO: remove after the associated issue is fixed
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // ChromeFocusRectangleTests::CheckFocusChromeVisuals fails on WPF
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SimpleHyperlink)
                TEST_METHOD_PROPERTY(L"Description", L"A one-line hyperlink")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // TODO: remove after the associated issue is fixed
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SplitHyperlink)
                TEST_METHOD_PROPERTY(L"Description", L"A multi-line hyperlink requiring multiple focus rectangles")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // TODO: remove after the associated issue is fixed
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HyperlinkTapped)
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // TODO: remove after the associated issue is fixed
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusDeviceLost)
                TEST_METHOD_PROPERTY(L"Description", L"Trigger device lost while a focus rectangle is showing. Don't crash.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusSetOnApp)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies when RevealFocus is set on Application, all controls get it")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NoRevealFocusOnHyperlink)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies when RevealFocus is set, hyperlinks dont get it")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusBreathing)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies correct behavior of the breathing animation")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // MOCK10_REMOVAL
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusPressState)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies correct behavior when focused element is pressed")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // MOCK10_REMOVAL
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusBorderlessNudgingInsideScrollViewer)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we don't nudge a borderless reveal focused element inside scrollviewer")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusBorderlessNudgingInsideScrollViewerNoScrollbar)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we don't nudge a borderless reveal focused element inside scrollviewer with no Scrollbars")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusBorderlessNudgingInsideVisibleBounds)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we don't nudge a borderless reveal focused element inside visible bounds of the window")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RevealFocusBorderlessWithShapeAsFocusTarget)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we use the Shape.Fill property for borderless glow")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            struct NudgeInsideArgs
            {
                ::Windows::UI::Color PrimaryColor = Microsoft::UI::Colors::Yellow;
                ::Windows::UI::Color SecondaryColor = Microsoft::UI::Colors::Red;

                Microsoft::UI::Xaml::Thickness PrimaryThickness = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(2);
                Microsoft::UI::Xaml::Thickness SecondaryThickness = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(2);
                Microsoft::UI::Xaml::Thickness Margin = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(-4);

                static NudgeInsideArgs Borderless(double thickness)
                {
                    NudgeInsideArgs args;
                    MakeBorderless(args, thickness);
                    return args;
                }

            protected:
                static void MakeBorderless(NudgeInsideArgs& args, double thickness)
                {
                    args.PrimaryColor = Microsoft::UI::Colors::Transparent;
                    args.SecondaryThickness = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(0);
                    args.PrimaryThickness = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(thickness);
                    args.Margin = Microsoft::UI::Xaml::ThicknessHelper::FromUniformLength(-thickness);
                }
            };

            struct NudgeInsideScrollViewerArgs : public NudgeInsideArgs
            {
                ::Windows::UI::Color CanvasBackground = Microsoft::UI::Colors::LightBlue;

                static NudgeInsideScrollViewerArgs Borderless(double thickness, ::Windows::UI::Color canvasBackground)
                {
                    NudgeInsideScrollViewerArgs args;
                    MakeBorderless(args, thickness);
                    args.CanvasBackground = canvasBackground;
                    return args;
                }
            };

            void NudgeInsideScrollViewerHelper(const wchar_t* const scrollViewerTemplate, const NudgeInsideScrollViewerArgs& args = NudgeInsideScrollViewerArgs());
            void NudgeInsideVisibleBoundsHelper(const NudgeInsideArgs& args = NudgeInsideArgs());
        };

    } }
} } } }

