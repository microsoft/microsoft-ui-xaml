// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class UIElementFacadeTests : public WEX::TestClass<UIElementFacadeTests>
{
public:
    BEGIN_TEST_CLASS(UIElementFacadeTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ActualOffsetAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of ActualOffset API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset is different
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ActualOffsetReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - ActualOffset")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ActualOffsetReferenceCanvas)
        TEST_METHOD_PROPERTY(L"Description", L"Reference ActualOffset via Canvas.Top/Left")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ActualSizeAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of ActualSize API")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ActualSizeCanvas)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of ActualSize API on Canvas")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ActualSizeReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - ActualSize")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ActualSizeReferenceCanvas)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - ActualSize on Canvas")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of Translation API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root, extra transform on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationAPIWithClip)
        TEST_METHOD_PROPERTY(L"Description", L"Translation with prepend clip")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root, extra transform on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of Rotation API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of Scale API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformMatrixAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of TramsformMatrix API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CenterPointAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of CenterPoint API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationAxisAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of RotationAxis API")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CombinedAPI)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of multiple facade APIs")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - Translation")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - Rotation")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - Scale")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformMatrixReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - TransformMatrix")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CenterPointReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - CenterPoint")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationAxisReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - RotationAxis")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - Opacity")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SubChannelReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - subchannels")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MultiReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - multiple properties")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InvalidReference)
        TEST_METHOD_PROPERTY(L"Description", L"Basic usage of references - Invalid property name")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PopulatePropertyInfoOverride)
        TEST_METHOD_PROPERTY(L"Description", L"Implementing PopulatePropertyInfoOverride virtual")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RedirectionTransform)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies correct static redirection transform above a Popup")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Translation property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationAnimationPlusECP)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Translation property in combination with ECP Translation")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root, extra transform on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationAnimationPlusECPAndClip)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Translation property in combination with ECP Translation")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root, extra transform on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationAnimationSubChannel)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Translation property, sub-channel targeting")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationPlusLTETarget)
        TEST_METHOD_PROPERTY(L"Description", L"Set Translation property and also target with an LTE")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationPlusLTETarget2)
        TEST_METHOD_PROPERTY(L"Description", L"Set Translation property and also target with an LTE and TransformParent with transform")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Rotation property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Scale property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleAnimationSubChannel)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Scale property, sub-channel targeting")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformMatrixAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate TransformMatrix property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformMatrixAnimationSubChannel)
        TEST_METHOD_PROPERTY(L"Description", L"Animate TransformMatrix property, sub-channel targeting")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CenterPointAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate CenterPoint property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CenterPointAnimationSubChannel)
        TEST_METHOD_PROPERTY(L"Description", L"Animate CenterPoint property, sub-channel targeting")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationAxisAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate RotationAxis property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationAxisAnimationSubChannel)
        TEST_METHOD_PROPERTY(L"Description", L"Animate RotationAxis property, sub-channel targeting")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate Opacity property, various variations")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  // UIElementFacadeTests.OpacityAnimation is unreliable
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MultiAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Animate multiple properties simultaneously")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimationAndReference)
        TEST_METHOD_PROPERTY(L"Description", L"Animate and reference a facade simultaneously")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InvalidAnimation)
        TEST_METHOD_PROPERTY(L"Description", L"Invalid uses of StartAnimation")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Configuration2RTL)
        TEST_METHOD_PROPERTY(L"Description", L"Create configuration where we have extra Components visual in the CompNode due to RTL")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Configuration2ScrollViewer)
        TEST_METHOD_PROPERTY(L"Description", L"Create configuration where we have extra Components visual in the CompNode due to ScrollViewer")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimationGroup)
        TEST_METHOD_PROPERTY(L"Description", L"Animate multiple properties through AnimationGroup")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestingAnimated)
        TEST_METHOD_PROPERTY(L"Description", L"Animate transform related facades and verify hit-testing behavior")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // TransformToVisual mismatch
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestingAnimatedAndReferenced)
        TEST_METHOD_PROPERTY(L"Description", L"Animate transform related facades, and also reference them, and verify hit-testing behavior")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // TransformToVisual mismatch
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestingTransformMatrix)
        TEST_METHOD_PROPERTY(L"Description", L"Verify hit-testing behavior when using TransformMatrix")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestingLargeMove)
        TEST_METHOD_PROPERTY(L"Description", L"Animate a large move of Translation and verify hit-testing")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTesting3D)
        TEST_METHOD_PROPERTY(L"Description", L"Hit test against facade properties that give the element 3D depth")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event timed out
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTesting2DRotations)
        TEST_METHOD_PROPERTY(L"Description", L"Hit test against facade Rotation properties that's 2D")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Event timed out
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HitTestingTranslationCombos)
        TEST_METHOD_PROPERTY(L"Description", L"Hit test against combinations of Translation with other properties")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(StrictChecks)
        TEST_METHOD_PROPERTY(L"Description", L"Verify strict enforcement across Facades APIs")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslationTransition)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleTransition)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationTransition)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityTransition)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityTransitionTo0)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransitionsFromMarkup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Offset on root
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ThisDotTarget)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PropertiesFromMarkup)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    void TranslationAPIInternal(bool useClip);
    void TranslationAnimationPlusECPInternal(bool useClip);
    void ExpectExceptionWithHRESULT(HRESULT expected, std::function<void()> functionCall);
    void ExpectEInvalidArg(std::function<void()> functionCall);
    void ExpectEAccessDenied(std::function<void()> functionCall);
    void ExpectSuccess(std::function<void()> functionCall);
    void VerifyCannotQIToDO(IInspectable* inspectable);

};

} } } } } }

