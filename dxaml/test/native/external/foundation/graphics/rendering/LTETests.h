// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class LTETests : public WEX::TestClass<LTETests>
{
public:
    BEGIN_TEST_CLASS(LTETests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\Microsoft.UI.Xaml.media.dxinterop.idl")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Renders elements with LTEs.")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(PortalingMediaElement)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PortalingSwapChainPanel)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PortalingRectangleWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Intermittent crash in MockDComp
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PortalingCompNodeSubtree)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GridViewEntranceInFlyoutEntranceWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing shadows
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityAnimationInEntranceTransitionWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ExplicitLTE)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopupLTE)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash when opening popup
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DeviceLost_Culled_LostTargetCompNode)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DeviceLost_Culled_LostLTE)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResetTreeWithOpenLTE)
        // This test tears down the tree and recovers by shutting down and reinitializing Xaml. ShutdownXaml is skipped for non-desktop SKUs
        // due to a stale DComp tree associated with the window after the DComp device is recreated. See comment
        // in WindowHelper::ShutdownXaml.
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // LTE test hook can't add to transition root of island's popup root
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SecondaryLTELeavesDanglingCompNodePointer)
        // The bug was hit due to timing, and is not actually platform-specific.
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NestedLTELeavesDanglingCompNodePointer)
        // Isolated repro for <Crash when clicking to dismiss a ContextFlyout for a TextBlock on a Tabbed Set tab>
        // The bug was hit due to timing, and is not actually platform-specific.
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BoundsDirtyFlags)
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;

    void Insert3Rectangles(Microsoft::UI::Xaml::Controls::GridView^ gridView);
};

} } } } } }

