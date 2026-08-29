// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus { namespace XYFocus {

        public enum class FocusElementType : int
        {
            Button,
            StackPanel,
            TextBlock,
            RichTextBlock
        };

        class XYFocusTests : public WEX::TestClass<XYFocusTests>
        {
        public:
            BEGIN_TEST_CLASS(XYFocusTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a77e56a5-6336-4de8-840a-6750d0e0239c")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(MediaTransportControlsTest)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus when Media Transport controls are showing")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Te.ProcessHost.exe crash
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // _MEDIA_REMOVED_
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LargeDistanceBetweenFocusableElements)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus transition betweeen distant elements")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClippedElementsTest)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus transition in non-visible (clipped) elements")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PageNavigationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus transition works correctly after page transition")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrollViewerScrollTest)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus properly scrolls a scrollviewer")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NarrowScrollViewerScrollTest)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus properly scrolls a narrow listview")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HyperlinkFocusableTest)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies automatic focus when a hyperlink is the next focusable element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusUsesDirectionOverride)
                TEST_METHOD_PROPERTY(L"Description", L"When an element specifies which direction to go, we should use that element instead of running the algorithm")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusUsesDirectionOverrideForStackPanel)
                TEST_METHOD_PROPERTY(L"Description", L"When an element specifies which direction to go, we should use that element instead of running the algorithm, StackPanel variant")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusUsesDirectionOverrideForTextBlock)
                TEST_METHOD_PROPERTY(L"Description", L"When an element specifies which direction to go, we should use that element instead of running the algorithm, TextBlock variant")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusUsesDirectionOverrideForRichTextBlock)
                TEST_METHOD_PROPERTY(L"Description", L"When an element specifies which direction to go, we should use that element instead of running the algorithm, RichTextBlock variant")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AutoFocusRunsWhenDirectionOverrideSetToNull)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when null has been given as a direction, we still run the algorithm")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DirectionOverrideCanSetFocusToSelf)
                TEST_METHOD_PROPERTY(L"Description", L"When the element itself is used as a direction, focus should not move")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AutoFocusConeEliminatesCandidates)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that elements not in the vision cone are ignored although they are the best candidate")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNestedScopeImplviaFindNextFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we are able to find the next focusable element within a given scope when a Hint Rect is specified by calling FindNextFocusWithSearchRootIgnoreEngagement")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusBubbles)
                TEST_METHOD_PROPERTY(L"Description", L"If we have an XYFocus set on a parent and we are leaving the 'scope' of that parent, we should use it's XYFocus properties")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusSetLocallyTakesPriorityOverParent)
                TEST_METHOD_PROPERTY(L"Description", L"If XYFocus is set locally, we should use that value and not it's parent")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusBubblingDoesntHappenWhenStillInsideScope)
                TEST_METHOD_PROPERTY(L"Description", L"If XYFocus is set on a parent, but focus change happens within that element, we should not use the XYFocus of the parent")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotSelectElementThatIsNotTabStoppable)
                TEST_METHOD_PROPERTY(L"Description", L"When an element is not a tab stop, it should not be a considered a candidate")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockShouldNotBeACandidate)
                TEST_METHOD_PROPERTY(L"Description", L"A TextBlock/RichTextBlock should not be considered a candidate")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ToggleSwitchWithLongHeaderStillGainsFocus)
                TEST_METHOD_PROPERTY(L"Description", L"A Toggle button is special because it's focusable area does not scale with it's header, meaning that our hittesting logic needs to be special cased.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")  // DCPP Test: XYFocusTests::ToggleSwitchWithLongHeaderStillGainsFocus is unreliable in WPF hosting mode
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateScopedSearch)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we always honor the search scope passed into focusmgr irrespective of concepts of engagement and multiple visual roots")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in test dll
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagedElementCanStillNavigateThroughPopupsOpenedDuringEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when a popup is opened during engagement, we include it in the candidate list when finding the next focusable element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event timed out
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagedElementDoesNotNavigateThroughPopupsOpenedBeforeEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when a popup is opened before engagement occurred, we do not include it in the candidate list when finding the next focusable element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagedElementCanDistinguishBetweenPopupsOpenedBeforeAndAfterEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can distinguish between when to consider a popup as a candidate based on when it was opened")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in test dll
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagedElementCanNavigateToPopupOpenedByAnotherPopup)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that popups opened from other popups are still considered if it happened during engagement")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event timed out
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnsureClipBoundsBeingUsedWhenScoringElementsInsideSplitView)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that SplitView uses clipped bounds when navigating using gamepad")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrolledOutOccludedElementsTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we scroll the occluded elements that are part of a scrollviewer into view only when we are actually Scrolling")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HyperlinkScrollTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we are able to scroll to and from hyperlinks in a ScrollViewer using gamepad")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CandidatesThatAreFullyContainedWithinElementShouldBeIgnored)
                TEST_METHOD_PROPERTY(L"Description", L"When an element is fully within the focused element, we should ignore it due to the focus cone")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in WUX.dll
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UsingFocusHintRectShouldIncludeFocusedElement)
                TEST_METHOD_PROPERTY(L"Description", L"When we pass in a focus hint rect, the focused element should be part of the candidate list")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(XYFocusIgnoresOcclusivity)
                TEST_METHOD_PROPERTY(L"Description", L"When using private api: IgnoreOcclusivity, we should choose a candidate even when occluded")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

        private:
            Microsoft::UI::Xaml::UIElement^ SetupTest(Platform::String^ xamlFile, Platform::String^ rootPanelName);
            inline Platform::String^ GetResourcesPath() const;
            xaml_primitives::Popup^ GetContainingPopup(FrameworkElement^ element);
            void XYFocusUsesDirectionOverrideInternal(FocusElementType elementType);
        };

    } }
} } } }
