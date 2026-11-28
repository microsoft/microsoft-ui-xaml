// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Thumb {

    class ThumbIntegrationTests : public WEX::TestClass<ThumbIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ThumbIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(DragCompletedEventArgsConstructorRoundTripsValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the public DragCompletedEventArgs constructor stores HorizontalChange, VerticalChange and Canceled independently.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragCompletedEventArgsConstructorPreservesBoundaryDoubles)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that signed zeros, extreme, denormal, infinite and NaN changes are returned unchanged on each axis.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragCompletedEventArgsSupportsDerivedClass)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a class derived from DragCompletedEventArgs is composed correctly and exposes the constructor values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragCompletedEventArgsFactoryValidatesAbiArguments)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the DragCompletedEventArgs activation factory and getters reject invalid ABI arguments and return a consistent inner object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragCompletedEventArgsRejectsCrossThreadAccess)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that DragCompletedEventArgs getters fail with RPC_E_WRONG_THREAD off the owning UI thread.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragCompletedReportsNetDisplacementOfMouseDrag)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a mouse drag of a Thumb raises one DragCompleted whose changes equal the summed DragDelta changes, in both directions.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CancelDragRaisesCanceledDragCompleted)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Thumb.CancelDrag during a drag raises one canceled DragCompleted with the changes so far, and nothing when not dragging.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisablingThumbDuringDragCompletesWithoutCancel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that disabling a Thumb during a drag raises one DragCompleted that is not canceled and carries the changes so far.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()
    };

} } } } } } }
