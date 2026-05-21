// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

    class ScrollViewerInGridViewTests : public WEX::TestClass<ScrollViewerInGridViewTests>
    {
    public:
        BEGIN_TEST_CLASS(ScrollViewerInGridViewTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanScrollToHorizontalOffsetWithScrollBarEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollToHorizontalOffset properly changes the view for a ScrollViewer inside a GridView, with HorizontalScrollBarVisibility==Auto.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollToHorizontalOffsetWithScrollBarDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollToHorizontalOffset properly changes the view for a ScrollViewer inside a GridView, with HorizontalScrollBarVisibility==Disabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollWithPenToHorizontalOffsetWithScrollBarEnabled)
            TEST_METHOD_PROPERTY(L"Disabled", L"Known issue")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that, even when using pen input, the ScrollToHorizontalOffset properly changes the view for a ScrollViewer inside a GridView, with HorizontalScrollBarVisibility==Auto.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollWithPenToHorizontalOffsetWithScrollBarDisabled)
            TEST_METHOD_PROPERTY(L"Disabled", L"Known issue")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that, even when using pen input, the ScrollToHorizontalOffset properly changes the view for a ScrollViewer inside a GridView, with HorizontalScrollBarVisibility==Disabled.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeViewWithScrollBarEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ChangeView properly changes the view for a ScrollViewer inside a GridView, with HorizontalScrollBarVisibility==Auto.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotChangeViewWithScrollBarDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ChangeView does not change the view for a ScrollViewer inside a GridView, with HorizontalScrollBarVisibility==Disabled.")
        END_TEST_METHOD()

    private:
        void AttemptHorizontalOffsetChange(bool isHorizontalScrollBarEnabled, bool useChangeView, bool usePen);
    };

} } } } } }
