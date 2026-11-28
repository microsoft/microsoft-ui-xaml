// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Thumb {

    class ThumbControlIntegrationTests : public WEX::TestClass<ThumbControlIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ThumbControlIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CancelDragWhenNotDraggingIsNoOp)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Thumb.CancelDrag does nothing when no drag is in progress.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CancelDragDuringMouseDragRaisesCanceledDragCompleted)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Thumb.CancelDrag ends an active mouse drag and raises a single canceled DragCompleted.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RemovedDragHandlersAreNotInvoked)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that removed DragStarted, DragDelta and DragCompleted handlers are not invoked, and that DragDelta can be handled again after its last handler was removed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RemovingLastDragStartedAndCompletedHandlersSilencesThemUntilReadded)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that after the only DragStarted and DragCompleted handlers are removed, a drag does not invoke them, and that handlers added afterwards are invoked.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragStartedHandlerFailureCancelsDrag)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Thumb cancels the drag it started (DragCompleted with Canceled=true, IsDragging false) when a DragStarted handler fails, and drags normally once that handler is removed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FocusStatesFollowThumbFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a focusable Thumb goes to the Focused visual state when it gets focus and back to Unfocused when it loses focus.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScrollBarThumbDoesNotDragWhenScrollModeDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the vertical ScrollBar thumb of a ScrollViewer does not start a drag while VerticalScrollMode is Disabled, and does once scrolling is enabled.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotStartDragWhenPointerPressedAlreadyHandled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Thumb does not start a drag when the press was handled by an element in its template, but does when the press reaches it unhandled.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GetIidsReportsThumbAndFactoryInterfaces)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that IInspectable::GetIids on a Thumb and on its activation factory reports IThumb and IThumbStatics respectively, and that neither reports the other's interface.")
        END_TEST_METHOD()
    };

} } } } } } }
