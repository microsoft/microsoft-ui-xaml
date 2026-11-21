// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wex.common.h>
#include <wextestclass.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Activation {

    class ActivationABITests
    {
    public:
        BEGIN_TEST_CLASS(ActivationABITests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;27f34780-4ef6-4102-929f-29737dfda1b9;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)
        
        BEGIN_TEST_METHOD(PointerReturnedForStyleSelectorIsCorrect)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that StyleSelector returns the right vtable pointer when activated through CreateInstance, as a sample of a type that internally uses AggregableActivationFactory")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewInitializeAndUninitializeManageActivatedRegistration)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that FrameworkView.Initialize on a secondary (non-main) view keeps the view, registers one Activated handler and keeps Application.Current, and that Uninitialize removes that exact registration and releases the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewActivatedDispatchesEachActivationKindToItsArgs)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the Activated handler registered by FrameworkView queries exactly the typed activation args interface that matches each ActivationKind, uses the generic IActivatedEventArgs for LockScreen, LockScreenComponent and other kinds, succeeds, and releases the args.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewInitializeReturnsIsMainFailure)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that FrameworkView.Initialize returns the ICoreApplicationView.IsMain failure unchanged, does not register for Activated and does not keep the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewInitializeReturnsActivatedRegistrationFailure)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that FrameworkView.Initialize returns an Activated registration failure unchanged, and that Uninitialize then releases the view without removing a registration that was never made.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewDoesNotRemoveZeroActivatedToken)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that Uninitialize does not call remove_Activated when the Activated registration returned a zero token, and still releases the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewReinitializeDoesNotRemoveStaleActivatedToken)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that after Initialize/Uninitialize, a second Initialize whose Activated registration fails does not make the next Uninitialize remove the already removed token again.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewUninitializeIsNoOpWithoutView)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that Uninitialize succeeds without touching any view when called before Initialize, and when called a second time after a full Initialize/Uninitialize cycle.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkViewLoadSucceedsWithoutTouchingTheView)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that FrameworkView.Load returns S_OK for an entry point and for a null entry point, and makes no calls on the initialized view.")
        END_TEST_METHOD()
    };
} } } } } }
