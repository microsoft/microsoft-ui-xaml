// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Frame {

    class FrameIntegrationTests : public WEX::TestClass<FrameIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FrameIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;e7de4cca-1436-4030-80b9-56ef01aa1cae;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Frame.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a Frame from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseNavigationEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Navigated and Navigating events are raised when a navigation is requested.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigateBetweenPages)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Frame can navigate forward and backward between different pages.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigateWithNavigationTransitionInfo)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Frame can navigate forward and backward with the navigation transition info.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReEntrancyPrevention)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Frame silently ignores recursive navigates to prevent compat breaks.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetNavigationStateWithCurrentPageNull)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Frame.GetNavigationState correctly returns the serialized navigation when current page is NULL.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetNavigationStateWithoutNavigatingToCurrent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Frame navigations work when restoring a navigation state without navigating to the current page.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CacheModeDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Jupiter Navigation main test suite (Ported from Legacy:CacheModeDisabled)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CacheModeEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Jupiter Navigation main test suite (Ported from Legacy:CacheModeEnabled)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CacheModeRequired)
            TEST_METHOD_PROPERTY(L"Description", L"Jupiter Navigation main test suite (Ported from Legacy:CacheModeRequired)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanceledNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Jupiter Navigation main test suite (Ported from Legacy:CanceledNavigation)")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDisableNavigationHistoryFromFrame)
            TEST_METHOD_PROPERTY(L"Description", L"Test if setting IsNavigationStackEnabled on Frame to false disables the navigation history.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDisableNavigationHistoryUsingNavigationMethod)
            TEST_METHOD_PROPERTY(L"Description", L"Test if setting IsNavigationStackEnabled on FrameNavigationOptions to false disables the navigation history for the current navigation.") // add desc
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        void ValidateFrameStack(xaml_controls::Frame^ frame, int expectedBackStackDepth, unsigned int expectedForwardStackDepth);

        void ValidateGoBackBehaviorWhenCurrentIsNull(xaml_controls::Frame^ frame, Platform::String^ navigationHistory);
        void ValidateGoForwardBehaviorWhenCurrentIsNull(xaml_controls::Frame^ frame, Platform::String^ navigationHistory);
        void ValidateNavigateBehaviorWhenCurrentIsNull(xaml_controls::Frame^ frame, Platform::String^ navigationHistory);
        void VerifyCachePageNavigationHelper(xaml_navigation::NavigationCacheMode cacheMode, int expectedValues[]);
    };

} } } } } }
