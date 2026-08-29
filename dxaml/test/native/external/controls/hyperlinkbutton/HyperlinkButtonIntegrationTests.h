// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace HyperlinkButton {

    class HyperlinkButtonIntegrationTests : public WEX::TestClass<HyperlinkButtonIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(HyperlinkButtonIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a HyperlinkButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a HyperlinkButton from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickUsingTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that HyperlinkButton that has content can launch events upon tapping.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetAndSetNavigationUri)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that HyperlinkButton can successfully get and set the NavigatiouUri.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of HyperlinkButton in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDCompTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree of HyperlinkButton in various visual states. This has been added to test for the underline.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of HyperlinkButton.")
        END_TEST_METHOD()

    private:
        static xaml_controls::Panel^ ValidateTreeTestSetup();
        void ValidateDCompTree(bool underlineInHighContrastOnly);
    };

} } } } } }

