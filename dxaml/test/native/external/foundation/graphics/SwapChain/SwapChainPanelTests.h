// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class SwapChainPanelTests : public WEX::TestClass<SwapChainPanelTests>
{
public:
    BEGIN_TEST_CLASS(SwapChainPanelTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\Microsoft.UI.Xaml.media.dxinterop.idl")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(SwapChainBasic)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies basic scenario of setting a SwapChain")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainNull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of clearing a SwapChain by passing in NULL")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainUpdate)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of updating SwapChain content")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainClear)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of clearing SwapChain content")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainMultiple)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of attaching multiple SwapChains")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainBadHandle)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies handling of setting a bas SwapChain Handle")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Disabled until IXP gets fixed. No more synchronous failure when passing in an invalid handle.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainHandleBasic)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies basic scenario of setting a SwapChain Handle")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainHandleNull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of clearing a SwapChain Handle by passing in NULL")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainHandleUpdate)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of updating SwapChain content for SwapChain handle")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainHandleClear)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of clearing SwapChain Handle")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainHandleMultiple)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of attaching multiple SwapChain Handle")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DX12SwapChainDevice)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies immediate relese of DX12 Device used in swap chain")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Re-enable SwapChainPanelTests::DX12SwapChainDevice test and remove old ref-count assumptions
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwapChainAndSwapChainHandle)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies scenario of mix match use of SwapChain and SwapChain Handle")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompositionScaleChanged)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestChildWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Validates DComp hit-test visibility with a standard child Button")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestOverlaySwapChainPanelWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Validates DComp hit-test visibility with a child SwapChainPanel")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestPopup)
        TEST_METHOD_PROPERTY(L"Description", L"Validates DComp hit-test visibility with a Popup")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestTransparentBrushWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Validates DComp hit-test visibility with a transparent brush")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestDisabledParentWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Validates DComp hit-test visibility with a disabled parent")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResizeSwapChainPanel)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that the SwapChainElement resizes as the SwapChainPanel resizes")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CreateCoreIndependentInputSource)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Task 62658820: Re-enable after fixing AV crash (0xC0000005) in Microsoft.UI.Input.dll from the IXP transport bump.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestingOnlySwapChainPanel)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that we use a transparent solid color visual for a SwapChainPanel with CreateCoreIndependentInputSource meant for off-thread input")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Task 62658820: Re-enable after fixing AV crash (0xC0000005) in Microsoft.UI.Input.dll from the IXP transport bump.
    END_TEST_METHOD()

private:
    inline Platform::String^ GetPathToFiles() const;

    void HitTestChildWUCInternal();
    void HitTestTransparentBrushInternal();
    Microsoft::UI::Xaml::Controls::SwapChainPanel^ CreateSizedSwapChainPanel();

    xaml_controls::SwapChainPanel^ m_swapChainPanel;
    xaml_controls::Button^ m_button;
};

} } } } } }



