// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

class HitTestTransform3D : public WEX::TestClass<HitTestTransform3D>
{
public:
    BEGIN_TEST_CLASS(HitTestTransform3D)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ScaleXYLarge)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D ScaleX/Y = 2.0, no PTx3D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslateAndScaleSameElement)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D TranslateX/Y = 20 & ScaleX/Y = 0.5")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Test CTx3D interop, one on parent, one on child
    BEGIN_TEST_METHOD(TranslateParentScaleChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid CTx3D TranslateX/Y = 20, Child Button ScaleX/Y = 0.5")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleParentTranslateChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid CTx3D ScaleX/Y = 0.5, Child Button TranslateX/Y = 20")
    END_TEST_METHOD()

    // Test Rotations with and without perspective
    BEGIN_TEST_METHOD(RotationXNoPerspective)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D RotationX = 45, no PTx3D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationYDefaultPerspective)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D RotationY = 45, default PTx3D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Perspective Parent, Tx3D Siblings
    BEGIN_TEST_METHOD(PerspectiveParentGridWithSiblingTransforms)
        TEST_METHOD_PROPERTY(L"Description", L"Perspective Parent, Tx3D Siblings")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PerspectiveParentGridWithSiblingTransforms2DAnd3D)
        TEST_METHOD_PROPERTY(L"Description", L"Perspective Parent, one sibling provides depth to parent. Other siblings are 2D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Perspective Parent, Nested Perspectives, Tx3D Siblings
    BEGIN_TEST_METHOD(PerspectiveParentGridWithSiblingNestedPerspectiveTransforms)
        TEST_METHOD_PROPERTY(L"Description", L"Perspective Parent, Nested Perspectives, Tx3D Siblings: Top Left")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleParentRenderTransformTranslateChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid CTx3D ScaleX/Y = 0.5, Child Button RenderTransform TranslateX/Y = 20")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderTransformTranslateParentTranslateChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid RenderTransform TranslateX/Y = 20, Child Button TranslateX/Y = 10")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Test interleaved 2D/3D
    // - PTx3D -> CTx3D -> RT2D
    BEGIN_TEST_METHOD(PerspectiveOuterScaleInnerRenderTransformTranslateChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid Default PTx3D, Inner Grid CTx3D ScaleX/Y = 0.5, Child Button RenderTransform TranslateX/Y = 20")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // - PTx3D -> RT2D -> CTx3D
    BEGIN_TEST_METHOD(PerspectiveOuterRenderTransformTranslateInnerRotationChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid Default PTx3D, Inner Grid TranslateX/Y = 20, Child Button CTx3D RotationX = 45")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // - PTx3D -> CTx3D RotY45/ProjX45
    BEGIN_TEST_METHOD(PerspectiveOuterRotationProjectionChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid Default PTx3D, Child Button CTx3D RotationY = 45 & Projection RotationX = 45")
#if defined(_WIN64) // Disable on 64-bit build due to precision issues
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
#endif
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // - RTXY20 -> PTx3D -> CTxRotY45/ProjX45
    BEGIN_TEST_METHOD(RenderTransformTranslateOuterPerspectiveInnerRotationProjectionChild)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid RT Translate X/Y = 20, Inner Grid Default PTx3D, Child Button CTx3D RotationY = 45 & Projection RotationX = 45")
#if defined(_WIN64) // Disable on 64-bit build due to precision issues
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
#endif
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // HitTest Bounds after the transform has been updated
    // - Bounds test basic
    BEGIN_TEST_METHOD(ChangeBoundsAndRetestSingleElement)
        TEST_METHOD_PROPERTY(L"Description", L"Original: CTx3D RotationY = 45, default PTx3D. Then: CTx3D TranslateXY += 20, retest to ensure bounds updated")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AddDepth)
        TEST_METHOD_PROPERTY(L"Description", L"Original: CTx3D RotationY = 0, default PTx3D. Then: CTx3D RotationY += 45, retest to ensure bounds updated")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LoseDepth)
        TEST_METHOD_PROPERTY(L"Description", L"Original: CTx3D RotationY = 45, default PTx3D. Then: CTx3D RotationY = 0, retest to ensure bounds updated")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // - Update parent transform, ensure child bounds correct
    BEGIN_TEST_METHOD(ChangeBoundsParentAndRetestChild)
        TEST_METHOD_PROPERTY(L"Description", L"Original Parent: CTx3D RotationY = 45 w/ default PTx3D. Then: CTx3D Parent TranslateXY += 20, retest to ensure Child bounds updated")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Non-{0,0} Center
    BEGIN_TEST_METHOD(ScaleXYNonDefaultCenter)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D ScaleX/Y = 0.5, no PTx3D, Center at bottom right corner")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RotationXNonDefaultCenter)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D RotationX = 45, default PTx3D, Center at bottom right corner")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScaleXYRotationXNonDefaultCenter)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D ScaleX/Y = 0.5 RotationX = 45, default PTx3D, Center at bottom right corner")
    END_TEST_METHOD()

    // Test clips
    BEGIN_TEST_METHOD(ClipOnlyPerspective)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid with default PTx3D, child Grid with clip, default child button")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ClipWithChildRotation)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid with default PTx3D, child Grid with clip, child with RotationY=-45")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // ----- Begin WUC versions -------------
    BEGIN_TEST_METHOD(DefaultCompositeAndPerspectiveWUC)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D and PTx3D have default values")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // [DCPP-test] WPF tests are failing with E_INVALIDARG in CDirectManipulationService::ActivateDirectManipulationManager
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PerspectivePlusPopupScaledWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Perspective on HandOff Visual plus a rignt-aligned Popup and plateau scale")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Failure assigning Popup.Child - XamlRoot API?
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
    END_TEST_METHOD()

    // Test CTx3D on an element, no chaining
    BEGIN_TEST_METHOD(ScaleXYSmallWUC)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D ScaleX/Y = 0.5, no PTx3D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TranslateXYWUC)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D TranslateX/Y = 20, no PTx3D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Test Rotations with and without perspective
    BEGIN_TEST_METHOD(RotationXDefaultPerspectiveWUC)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D RotationX = 45, default PTx3D")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // Test 2D/3D interop
    BEGIN_TEST_METHOD(ScaleAndRenderTransformTranslateSameElementWUC)
        TEST_METHOD_PROPERTY(L"Description", L"CTx3D ScaleX/Y = 0.5 & RenderTransform TranslateX/Y = 20")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // [DCPP-test] WPF tests are failing with E_INVALIDARG in CDirectManipulationService::ActivateDirectManipulationManager
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    // - PTx3D -> CTx3D RotY45 -> ProjX45
    BEGIN_TEST_METHOD(PerspectiveOuterRotationInnerProjectionChildWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid Default PTx3D, Inner Grid CTx3D RotationY = 45, Child Button Projection RotationX = 45")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PerspectiveOuterRotationInnerTranslationProjectionChildWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Parent Grid Default PTx3D, Inner Grid CTx3D RotationY = 45, Child Button Translation + Projection RotationX = 45")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

public:
    static void VerifyGetGlobalBounds(UIElement^ element, const ::Windows::Foundation::Rect* expectedRect);
    static void VerifyGetGlobalBounds(UIElement^ element, const ::Windows::Foundation::Rect& expectedRect1, const ::Windows::Foundation::Rect& expectedRect2);
    static bool CompareRectsWithEpsilon(const ::Windows::Foundation::Rect* expectedRect, const ::Windows::Foundation::Rect* actualRect, float epsilon = 0.01f);
    static bool CompareFloatsWithEpsilon(float a, float b, float epsilon);

private:
    inline Platform::String^ GetResourcesPath() const;

    void DefaultCompositeAndPerspectiveInternal();
    void ScaleXYSmallInternal();
    void TranslateXYInternal();
    void RotationXDefaultPerspectiveInternal();
    void ScaleAndRenderTransformTranslateSameElementInternal();
    void PerspectiveOuterRotationInnerProjectionChildInternal(bool useTranslation, bool isWUCMode);

    Microsoft::UI::Xaml::Media::TranslateTransform^ HitTestTransform3D::GenerateRenderTransform();
    Microsoft::UI::Xaml::Media::PlaneProjection^ GeneratePlaneProjection();
    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ GenerateScaleTransform3D(float scaleX, float scaleY, float scaleZ);
    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ GenerateTranslateTransform3D(float translateX, float translateY, float translateZ);
    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ GenerateRotationTransform3D(float rotationX, float rotationY, float rotationZ);
    Microsoft::UI::Xaml::Media::Media3D::CompositeTransform3D^ GenerateCompositeTransform3D(float scaleX, float scaleY, float scaleZ, float translateX, float translateY, float translateZ, float rotationX, float rotationY, float rotationZ);
};

} } } } } } }
