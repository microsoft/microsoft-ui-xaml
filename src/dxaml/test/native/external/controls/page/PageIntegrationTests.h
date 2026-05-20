// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Page {

    class PageIntegrationTests : public WEX::TestClass<PageIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PageIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Page.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a Page from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetAndSetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get public properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetAndSetDesktopOnlyProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get public properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LayoutToLayoutBoundsTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates position of controls for Page.")
            // TODO: Enable this test when Xbox does the work to return TitleSafe area in VisibleBound ()
            //                          Also scope this test to Xbox platform ("Need TEST_METHOD_PROPERTY(L"Platform", L"Xbox") functionality)
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LayoutUpdateTriggeredByCoreWindowResize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the page fires the layout updated event when the core window is resized.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePageStackEntryCollectionBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the collection calls used when navigating between pages are working as intended.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PageRendersBackground)
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

    private:
        void ValidateRectanglePlacement(xaml_controls::Page^ rootPage);
        void ValidateToolTipPlacement(xaml_controls::Page^ rootPage);
        void PageRendersBackgroundCommon();

        static const int TitleSafe_Left = 100;
        static const int TitleSafe_Top = 40;
    };

} } } } } }

