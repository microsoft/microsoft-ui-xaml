// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ThemeShadowTests : public WEX::TestClass<ThemeShadowTests>
{
public:
    BEGIN_TEST_CLASS(ThemeShadowTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(ThemeShadowLoadTest)

    BEGIN_TEST_METHOD(ThemeShadowBasicDropShadow)
        TEST_METHOD_PROPERTY(L"Description", L"Basic ThemeShadow with drop shadow.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowOpacity)
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, Opacity scenarios")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowHitTest)
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, shadow should not hit-test")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowHitTestWindowedPopup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, on Popup, shadow should not hit-test")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowTargetOfLTE)
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, element is the target of an LTE")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowWindowedPopup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, element is windowed Popup")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowWindowedPopupRTL)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, element is windowed Popup, parented to RTL element")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowWindowedPopup125)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowWindowedParentlessPopup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, element is windowed parentless Popup")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowWindowedParentlessPopupRTL)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, element is windowed parentless Popup, RTL")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowBasicPopup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing shadows
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Basic ThemeShadow usage on Popup element.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowBasicNonPopup)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Basic ThemeShadow usage on non-Popup element.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowBasicNonPopupLTE)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing shadows
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow usage on LayoutTransitionElement target.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowProjectedShadowBasicPopupAndChild)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Popup and Popup.Child both with ThemeShadow in projected shadow mode - expect Child's shadow only.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowBasicPopupAndChild)
        TEST_METHOD_PROPERTY(L"Description", L"Popup and Popup.Child both with ThemeShadow in drop shadow mode - expect Child's shadow only.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDeepCaster)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Caster elements in the Popup's subtree.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowClips)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Caster elements with various clips on them.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReceiversAPI)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReceiversAPI_InvalidArg_AddAncestor)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Build a tree top down. Add an ancestor element to a Receivers collection.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReceiversAPI_InvalidArg_ParentUnderReceiver)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Build a tree bottom up. Parent the subtree under an element in the Receivers collection.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReceiversAPI_InvalidArg_SharedShadow)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Share a ThemeShadow among multiple elements. Add an ancestor element to a Receivers collection.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReceiversAPI_SharedReceivers)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowFallback)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Test ThemeShadow fallback.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EnsureRootCanvasCompNodeBasic)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Ensure canvas root has compNode when ThemeShadow is present, element with shadow is added when entering the tree")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EnsureRootCanvasCompNodeLateAdd)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Ensure canvas root has compNode when ThemeShadow is present, shadow is added to element already in tree")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EnsureRootCanvasCompNodeParentLessPopup)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Ensure canvas root has compNode when ThemeShadow is present, popup is parentless")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowCasterFiltering_TwoCasters)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Tests caster element filtering tests with two caster elements.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowCasterFiltering_ThreeCasters)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Tests caster element filtering tests with three caster elements.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowCasterFiltering_NestedPopups)
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Support Projected Shadows
        TEST_METHOD_PROPERTY(L"Description", L"Tests caster element filtering tests with nested poupups.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowLoadTest)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Basic test loading a ThemeShadow with Drop Shadows enabled.")
    END_TEST_METHOD()
    
    BEGIN_TEST_METHOD(ThemeShadowDropShadowBasicPopup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Basic ThemeShadow usage on Popup element with Drop Shadows enabled.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowVerifyShadowPixels)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Basic DropShadow with pixel verification using RenderTargetBitmap.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowDynamicCornerRadius)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Testing DropShadows with different corner radius on caster element.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowDynamicCornerRadiusOnParent)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Testing DropShadows where child inherits corner radius values from parent.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowRoundedCornersTargetOfLTE)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"ThemeShadow with drop shadow, element is the target of an LTE and has rounded corners.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowUseCachedBrush)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Testing DropShadows that all use the same dimensions in order to exercise the cached DropShadow.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowSystemThemeRedrawRTB)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Testing DropShadows that differ in appearance based on the system theme, verified with RTB.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThemeShadowDropShadowCornerRoundingOnControl)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"Description", L"Testing DropShadows on Control elements with corner rounding.")
    END_TEST_METHOD()

private:
    void ThemeShadowBasicPopupAndChild();
    void ThemeShadowBasicNonPopupInternal(bool useLTE);
    void ThemeShadowDropShadowWindowedPopupInternal(bool useRTL, float displayScale);
    void ThemeShadowDropShadowHitTestInternal(bool useWindowedPopup);
    void ThemeShadowDropShadowWindowedParentlessPopupInternal(bool useRTL);
};

} } } } } }

