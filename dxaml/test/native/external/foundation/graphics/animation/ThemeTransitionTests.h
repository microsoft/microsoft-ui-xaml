// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <CustomMetadataRegistrar.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ThemeTransitionTests : public WEX::TestClass<ThemeTransitionTests>
{
public:
    BEGIN_TEST_CLASS(ThemeTransitionTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ValidateStaggeringWorks)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSlideThemeTransitionEffect)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateNavigationThemeTransitionWorksWhenNotPresent)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateEntranceNavigationThemeTransition)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSlideNavigationThemeTransition)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in test process
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateDrillInNavigationThemeTransition)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateCommonNavigationThemeTransition)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateContinuumNavigationThemeTransition)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateSuppressNavigationThemeTransition)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EdgeUIThemeTransition)
        TEST_METHOD_PROPERTY(L"Description", L"Tests the EdgeUIThemeTransition using multiple Edge properties.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PaneThemeTransition)
        TEST_METHOD_PROPERTY(L"Description", L"Tests the PaneThemeTransition using multiple Edge properties.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ContentThemeTransition)
        TEST_METHOD_PROPERTY(L"Description", L"Tests the ContentThemeTransition using multiple horizontal/vertical offsets.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateContentOverride)
        TEST_METHOD_PROPERTY(L"Description", L"Validate Frame's content theme will override the page default")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp crash
    END_TEST_METHOD()
private:
    void TestThemeTransitionXaml(Platform::String^ path);

    inline Platform::String^ GetResourcesPath() const;
};

} } } } } }

