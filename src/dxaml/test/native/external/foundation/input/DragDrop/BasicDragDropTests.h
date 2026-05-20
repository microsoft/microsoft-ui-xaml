// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DragDrop {
        class BasicDragDropTests : public WEX::TestClass < BasicDragDropTests >
        {
        public:
            BEGIN_TEST_CLASS(BasicDragDropTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(PrepareDragTests)
                TEST_METHOD_PROPERTY(L"Description", L"Runs a drag and drop operation in its own window to isolate a suspected issue with DataExchangeHost.exe")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDragUsingCoreDragOperation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging in a Rectangle with the left mouse button and drag and drop events")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDragUsingStartDragAsync)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging in a Rectangle with the left mouse button and drag and drop events")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  // BasicDragDropTests.CanDragUsingStartDragAsync is unreliable
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDefaultMouseDrag)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging in a Rectangle with the left mouse button and drag and drop events")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDefaultTouchDrag)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging in a Rectangle by press hold and drag with drag and drop events")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 20928844: Re-enable after investigating why software injection causes this to fail.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCancelDrag)
                TEST_METHOD_PROPERTY(L"Description", L"Validates drag operations can be canceled by setting the DragStartingEventArgs")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCancelDragProgrammatically)
                TEST_METHOD_PROPERTY(L"Description", L"Validates drag operations can be canceled by setting the DragStartingEventArgs")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetCustomDragVisualWithBitmapUriSource)
                TEST_METHOD_PROPERTY(L"Description", L"Validate custom DragVisual can be set using BitmapImage.UriSource")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetCustomDragVisualWithBitmapUriSourceSmallerSize)
                TEST_METHOD_PROPERTY(L"Description", L"Validate custom DragVisual can be set using BitmapImage.UriSource with decoded size smaller than the dragging element")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetCustomDragVisualWithBitmapSetSource)
                TEST_METHOD_PROPERTY(L"Description", L"Validate custom DragVisual can be set using BitmapImage.SetSource")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetPreparedCustomDragVisualWithBitmapUriSource)
                TEST_METHOD_PROPERTY(L"Description", L"Validate custom DragVisual created previously using BitmapImage.SetSource can be set")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetPreparedCustomDragVisualWithBitmapSetSource)
                TEST_METHOD_PROPERTY(L"Description", L"Validate custom DragVisual created previously using BitmapImage.SetSource can be set")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateDragOperationDeferral)
                TEST_METHOD_PROPERTY(L"Description", L"Validate DragOperationDeferral working correctly")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
             END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetAllDragVisualSettings)
                TEST_METHOD_PROPERTY(L"Description", L"Validate that all Drag Visual Settings not exerciced in other tests are available")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanUseSoftwareBitmapAndDeferral)
                TEST_METHOD_PROPERTY(L"Description", L"Validate that we can take a deferral and use a SoftwareBitmap as visual")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanTakeDeferralOnDragStarting)
                TEST_METHOD_PROPERTY(L"Description", L"Validates Drag does not really start before deferral is completed")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ProvidesDragUIOverrideOnLeave)
                TEST_METHOD_PROPERTY(L"Description", L"Core Dnd does not provide a ICoreDragVisualOverride on leave but our event does.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotRaiseMultipleDragEnterOnTreeChange)
                TEST_METHOD_PROPERTY(L"Description", L"Check that DragEnter is only raised once when the Tree is changed on Enter.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDoTouchListReordering)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ListReordering with touch")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearUIOverridesWhenSwitchingTarget)
                TEST_METHOD_PROPERTY(L"Description", L"Validates DragUIOverride is correctly cleared when moving beween adjacent targets")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeferralOnEnterShouldNotBreakLeave)
                TEST_METHOD_PROPERTY(L"Description", L"Validates taking a deferral on DragEnter does not break leave")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateThatLightDismissPopupDoesNotDismissWhenStartingDragDrop)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that Drag and Drop operations won't close a light dismiss popup.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetCanDragOnListViewItem)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that CanDrag set on ListViewItem works as expected")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDeleteDraggedElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can delete the dragged element while reordering inside a list.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // TODO: Fix this issue
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanReorderAndDropOnItems)
                TEST_METHOD_PROPERTY(L"Description", L"Validates it is possible to drop an item from a reorderable list onto another item")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCollapseDraggedElement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates it is possible to collapse the Dragged Element without crashing the app")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanReorderWithLegacyPanel)
                TEST_METHOD_PROPERTY(L"Description", L"Basic validation that drag and drop works when using a VirtualizingStackPanel legacy panel.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AllowedOperationsPassThrough)
                TEST_METHOD_PROPERTY(L"Description", L"Validation that the AllowedOperations Set in the DragStartingEventArgs correctly copy into the DragEventArgs for its drag events.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanDragToFromWindowedPopups)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that items can be dragged from and to windowed popups")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

        private:
            static void DndTestCleanup();
            void PerformTouchListReordering(bool useLegacyPanel);
            void SetCustomDragVisualWithBitmapUriSourceHelper(bool smallerDecodeSize = false);
            void EnsureDataExchangeHostStarted();
            void TestDragDropBetweenElements(FrameworkElement^ source, FrameworkElement^ target);
            static wf::Point GetCenterPoint(FrameworkElement^ element);
            static void DragBetweenPoints(wf::Point point1, wf::Point point2);
        };
    } } }
} } } }
