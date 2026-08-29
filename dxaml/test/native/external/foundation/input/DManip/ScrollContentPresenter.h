// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class ScrollContentPresenter : public WEX::TestClass<ScrollContentPresenter>
        {
        public:
            BEGIN_TEST_CLASS(ScrollContentPresenter)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(StretchAlignmentSwitchedOffAndOn)
                TEST_METHOD_PROPERTY(L"Description", L"Tests alignment of ScrollContentPresenter content with a Stretch alignment switched off and on within a plain ScrollViewer.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StretchAlignmentInListView)
                TEST_METHOD_PROPERTY(L"Description", L"Tests alignment of ScrollContentPresenter content with a Stretch alignment switched off and on within a ListView.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
#ifndef MUX_PRERELEASE
                // Test disabled in release
                // Investigate and enable StretchAlignmentInListView test in MUXFinalRelease
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") 
#endif
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StretchAlignmentInFlyout)
                TEST_METHOD_PROPERTY(L"Description", L"Tests alignment of ScrollContentPresenter content inside an unconstrained Flyout.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop,WindowsCore") // MockDComp doesn't like content in windowed popups on desktop.
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event timed out
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StretchAlignmentWithTextControl)
                TEST_METHOD_PROPERTY(L"Description", L"Tests alignment of ScrollContentPresenter content with a Stretch alignment for text controls.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ViewportInteraction)
                TEST_METHOD_PROPERTY(L"Description", L"Tests assignment of viewport interactions to scroll viewers")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;

            void AddScrollViewerAndRichTextBlock(
                xaml_controls::StackPanel^ stackPanel,
                bool smallText,
                bool wrap,
                bool canScrollHorizontally,
                bool canScrollVertically,
                int margin,
                double width,
                double height) const;

        };

    } } }
} } } }

