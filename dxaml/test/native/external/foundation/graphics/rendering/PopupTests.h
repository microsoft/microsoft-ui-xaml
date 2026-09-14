// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <HostingModeTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class PopupTests : public WEX::TestClass<PopupTests>
{
public:
    BEGIN_TEST_CLASS(PopupTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        TEST_CLASS_HOSTING_MODE_DEFAULT()
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(WindowedPopupHasForcedBackground)
        // Crash
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ParentedPopup_ValidateRequestedThemePropagation)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"False")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResetNestedPopups)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(WindowedPopupsMoveWithAncestorChain)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InlinePopupInWindowedPopup)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    // This test is verifying incorrect behavior. It should be updated after the Xaml bug is fixed.
    BEGIN_TEST_METHOD(InlinePopupTabOrder)
    END_TEST_METHOD()

    // This test is verifying incorrect behavior. It needs the inline popup bug being fixed first.
    BEGIN_TEST_METHOD(WindowedPopupTabOrder)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SystemBackdropRoundedCorners)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(WindowedPopupDeviceLost)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(WindowedPopupAncestorScale)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NoXamlRoot)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CloseWindowWithPopupOpen)
        TEST_METHOD_PROPERTY(L"Description", L"Closes the window when it has a popup open to confirm it doesn't crash.")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
    Microsoft::UI::Xaml::Controls::Primitives::Popup^ MakeRTLPopupInLTRTree();
    Microsoft::UI::Xaml::Controls::Primitives::Popup^ MakePopup(Microsoft::UI::Xaml::UIElement^ child);

    void PopupTabOrder(bool isWindowed);
};

    class PopupTestsUap : public WEX::TestClass<PopupTestsUap>
    {
    public:
        BEGIN_TEST_CLASS(PopupTestsUap)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
        TEST_CLASS_PROPERTY(L"MasterFile:ClassName", L"PopupTests")
        TEST_CLASS_HOSTING_MODE(UAP)
    END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

    private:
        Microsoft::UI::Xaml::Controls::Primitives::Popup^ MakeRTLPopupInLTRTree();

    public:
        BEGIN_TEST_METHOD(ParentedPopup_RTLSubtreeInLTRTreeWUC)
        // MockDComp shows an extra RTL transform
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParentedPopup_RTLSubtreeInLTRTreeWUC_AnimatedOffset)
        // MockDComp shows an extra RTL transform
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ParentedPopup_RTLSubtreeInLTRTreeWUC_HandoffVisual)
        // MockDComp shows an extra RTL transform
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()
    };

} } } } } }

