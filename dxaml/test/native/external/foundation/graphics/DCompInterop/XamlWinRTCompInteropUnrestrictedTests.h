// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RegKeyHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class XamlWinRTCompInteropUnrestrictedTests : public WEX::TestClass<XamlWinRTCompInteropUnrestrictedTests>
{
public:
    BEGIN_TEST_CLASS(XamlWinRTCompInteropUnrestrictedTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(HitTestHandOffVisualScaleRotate)
        TEST_METHOD_PROPERTY(L"Description", L"Hit-testing on hand-off visual using both non-uniform scale and rotation")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CustomLayout)
        TEST_METHOD_PROPERTY(L"Description", L"Requests the HandOff visual from a XAML element that does custom layout and verifies TransformToVisual.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandoffVisualLTEWUC)
        TEST_METHOD_PROPERTY(L"Description", L"An element with a handoff visual shouldn't crash when render in multiple places with an LTE.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    // TODO: This can't be enabled now, since focing Close fails with a FailFast. Instead we should raise an exception
    //       containg the reason that calling IClosable::Close() is blocked for handoff visuals.
    //       Need to work with COMP to change this behavior, then enable this test.
    //BEGIN_TEST_METHOD(HandOffClosedByApp)
    //    TEST_METHOD_PROPERTY(L"Description", L"Gets handoff visual for a XAML element, then forces Close via explicit delete of visual. App should get an exception as Close on Handoff visuals is not allowed.")
    //    TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    //END_TEST_METHOD()

    BEGIN_TEST_METHOD(BasicGetElementVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementVisual method to access a WinRT Composition Visual for a UIElement.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetElementVisualAfterLeavingTreeWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementVisual method to access a WinRT Composition Visual for a UIElement which left the visual tree.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetElementVisualWithNullParentCheckWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementVisual method to access a WinRT Composition Visual for a UIElement and verifies its parent is null.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetElementVisualWithNullContainerCheckWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementVisual method to access a WinRT Composition Visual for a UIElement and verifies it cannot be casted into a ContainerVisual.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetElementVisualWithDeviceLossWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementVisual method to access a WinRT Composition Visual for a UIElement before a simulated device loss.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetElementVisualWithCommitRequestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Modifies a WinRT Composition Visual for a UIElement and ensures the DComp device commits the changes.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResetNullElementChildVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method to reset the child that is already null.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetElementChildVisualForBorderWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for an empty Border to set a WinRT child that is currently null.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetElementChildVisualForUnparentedElementWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for an unparent Button to set a WinRT child and then adds it to the visual tree.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetElementChildVisualForPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a populated Panel to set a WinRT child that is currently null.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetElementChildVisualForComplexPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a Panel populated with composition nodes to set a WinRT child that is currently null.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetElementChildVisualAfterLeavingTreeWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementChildVisual method to access a WinRT Composition Visual for a UIElement which left the visual tree.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResetElementChildVisualForPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementChildVisual/SetElementChildVisual methods for a populated Panel to reset the child that was previously set.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResetElementChildVisualForComplexPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the GetElementChildVisual/SetElementChildVisual methods for a Panel populated with composition nodes to reset the child that was previously set.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetChildAndAddChildToPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a populated Panel to set a WinRT child and then add a XAML child to the Panel.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetChildAndAddChildToComplexPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a Panel populated with composition nodes to set a WinRT child and then add a XAML child to the Panel.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetChildAndRemoveChildrenFromPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a populated Panel to set a WinRT child and then removes all XAML children from the Panel.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetChildAndRemoveChildrenFromComplexPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a Panel populated with composition nodes to set a WinRT child and then removes all XAML children from the Panel.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetChildAndClearChildrenFromPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a populated Panel to set a WinRT child and then clears all XAML children from the Panel.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SetChildAndClearChildrenFromComplexPanelWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises the SetElementChildVisual method for a Panel populated with composition nodes to set a WinRT child and then clears all XAML children from the Panel.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisualWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Transforms the HandOff visual and hit tests it.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestFacadesWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Transforms the UIElement via facades and hit tests it.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestFacadesScaleRotate)
        TEST_METHOD_PROPERTY(L"Description", L"Hit-testing with facades using both non-uniform scale and rotation")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisual2WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Requests the HandOff visual from a XAML element with XAML transforms and hit tests it.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisual3WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Additional tests involving AnchorPoint")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisualClipWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Changes the hand off visual's clip via WUC. The changes should be reflected in that Xaml element's hit testing.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisualClipWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Changes the hand off visual's clip via WUC. The changes should be reflected in that Xaml element's hit testing.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisualTransform3DWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Changes the hand off visual's TransformMatrix via WUC to something with 3D. The changes should be reflected in that Xaml element's hit testing.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisualTranslateWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies XAML hit-testing when using the Translation property")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Projection1WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Requests the HandOff visual from a XAML element with Projection set and verifies TransformToVisual.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestHandOffVisualProjection)
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderCompScaledElementWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Scales the HandOff visual with comp, then renders the element in Xaml. Xaml shouldn't include the comp scale in its realization.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompAnimationOnLoadWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Setting comp properties & expressions mixed with setting Xaml values. Verifies that Xaml stomps over the app values or avoids stomping over them as appropriate.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandInClosedByXamlWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Sets handin visual for a XAML element, then causes the Xaml element's destruction. Xaml should force close the visual via IClosable::Close()")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    // TODO: This can't be enabled now, since focing Close fails with a FailFast. Instead we should raise an exception
    //       containg the reason that calling IClosable::Close() is blocked for handoff visuals.
    //       Need to work with COMP to change this behavior, then enable this test.
    //BEGIN_TEST_METHOD(HandOffClosedByAppWUC)
    //    TEST_METHOD_PROPERTY(L"Description", L"Gets handoff visual for a XAML element, then forces Close via explicit delete of visual. App should get an exception as Close on Handoff visuals is not allowed.")
    //    TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    //END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandInClosedByApp1WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Sets handin visual for a XAML element, then deletes it while holding UIElement reference. Xaml should not crash and manage closed handin visual as any other.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandInClosedByApp2WUC)
        TEST_METHOD_PROPERTY(L"Description", L"Test replacing a handin visual after it's been disposed, as well as with a disposed visual. Xaml should not crash and manage closed handin visuals as any other")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandInReusedByAppWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Sets handin visual for a XAML element, then removes it and destroys the Xaml element. Handin should survive and get used as handin for another Xaml elemen.t")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandInVisualCausesDManipHitTestVisual)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ImplicitAnimationDisablerWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies Implicit Animations don't run when they aren't supposed to")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ElementCullingWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies we don't cull out elements that have WUC visuals on them")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompositorTest)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies the app has access to the Compositor")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Test requires Window::Current that is not available in Win32
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Translation1WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies basic usage of Translation property")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Translation2WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies basic usage of Translation property, 2nd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Translation3WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies basic usage of Translation property, 3rd variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Translation4WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies basic usage of Translation property, 4th variation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Translation4BWUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies combination of Translation property and implicit Show animation")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Translation5WUCFull)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies prepend clip translation expression")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GetHandOffVisualForAnimatingClip)
        TEST_METHOD_PROPERTY(L"Description", L"Gets a hand off visual for an element with an animating clip. Don't crash.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
    Microsoft::UI::Composition::Visual^ CreateVisual() const;
    void SetElementChildVisualForPanelPrivate(_In_ bool panelChildrenUseCompositionNodes);
    void ResetElementChildVisualForPanelPrivate(_In_ bool panelChildrenUseCompositionNodes);
    void SetElementChildVisualBeforeAddingChildToPanelPrivate(_In_ bool panelChildrenUseCompositionNodes);
    void SetElementChildVisualBeforeRemovingChildrenFromPanelPrivate(_In_ bool panelChildrenUseCompositionNodes);
    void SetElementChildVisualBeforeClearingChildrenFromPanelPrivate(_In_ bool panelChildrenUseCompositionNodes);

    void BasicGetElementVisualInternal();
    void GetElementVisualAfterLeavingTreeInternal();
    void GetElementVisualWithNullParentCheckInternal();
    void GetElementVisualWithNullContainerCheckInternal();
    void GetElementVisualWithDeviceLossInternal();
    void GetElementVisualWithCommitRequestInternal();
    void ResetNullElementChildVisualInternal();
    void SetElementChildVisualForBorderInternal();
    void SetElementChildVisualForUnparentedElementInternal();
    void SetElementChildVisualForPanelInternal();
    void SetElementChildVisualForComplexPanelInternal();
    void GetElementChildVisualAfterLeavingTreeInternal();
    void ResetElementChildVisualForPanelInternal();
    void ResetElementChildVisualForComplexPanelInternal();
    void SetChildAndAddChildToPanelInternal();
    void SetChildAndAddChildToComplexPanelInternal();
    void SetChildAndRemoveChildrenFromPanelInternal();
    void SetChildAndRemoveChildrenFromComplexPanelInternal();
    void SetChildAndClearChildrenFromPanelInternal();
    void SetChildAndClearChildrenFromComplexPanelInternal();
    void HitTestHandOffVisualInternal();
    void HitTestHandOffVisual2Internal();
    void HitTestHandOffVisual3Internal(bool inWUCMode);
    void HitTestHandOffVisualClip();
    void HitTestHandOffVisualTransform3D();
    void Projection1Internal();
    void HandoffVisualLTEInternal();
    void HandInClosedByXamlInternal();
    void HandOffClosedByAppInternal();
    void HandInClosedByApp1Internal();
    void HandInClosedByApp2Internal();
    void HandInReusedByAppInternal();

    Microsoft::UI::Xaml::Controls::Border^ ElementCulling_MakeBorder(::Windows::UI::Color color);
    void ElementCulling_DontCullHandOffVisuals();
    void ElementCulling_UncullAfterGettingHandOffVisuals();
    void ElementCulling_UncullAfterAddingElementsWithHandOffVisuals();
    void ElementCulling_Clip();
    void ElementCulling_LayoutClip();
    void ElementCulling_Transform();
};
} } } } } }

