// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace FrameworkStyles {

    class FrameworkStylesIntegrationTests : public WEX::TestClass<FrameworkStylesIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FrameworkStylesIntegrationTests)
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

        BEGIN_TEST_METHOD(ValidateNavBackBtnUIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree with NavigationBackButton* framework styles applied, in various visual states.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTxtBlkBtnUIETree)
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUseSystemFocusVisualsDefaults)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UseSystemFocusVisuals is the correct default on various controls.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // When test calls "Application::Current->FocusVisualKind = FocusVisualKind::Reveal",
                                                            // some elements don't seem to have an updated UseSystemFocusVisuals as expected.
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

        private:
            static inline Platform::String^ GetResourcesPath();

            void VerifyUseSystemFocusVisualsHelper(
                xaml_controls::Grid^ root, bool expectedUseSystemFocusVisuals, Platform::String^ elementName, Platform::String^ innerChildToo = nullptr);
    };

} } } } } }

