// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ColorAnimationTests : public WEX::TestClass<ColorAnimationTests>
{
public:
    BEGIN_TEST_CLASS(ColorAnimationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;b7d949c9-6779-44c5-b16b-1f51e18f3866")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(SimpleColorAnimationWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LoopingColorAnimationWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ForeverColorAnimationWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ColorAndDoubleAnimationWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SolidColorBrushAnimationWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    TEST_METHOD(LinearAnimationReadbackWUCFull)

    TEST_METHOD(KeyFrameAnimationReadbackWUCFull)

    BEGIN_TEST_METHOD(HandoffToDepObjAnimWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NullDurationWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SeekHandoffWUCFull)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Reenable this test for OneCore once the DComp issue is reoslved: 7067420
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanvasWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ContentPresenterWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;

    static void ColorAnimationTests::CreateColorAnimation(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        Platform::String^ brushName,
        ::Windows::UI::Color toColor,
        int64 duration,
        Microsoft::UI::Xaml::Media::Animation::Storyboard^ storyboard
        );

    static void ColorAnimationTests::CreateColorAnimation(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        Platform::String^ brushName,
        ::Windows::UI::Color fromColor,
        ::Windows::UI::Color toColor,
        Microsoft::UI::Xaml::Media::Animation::Storyboard^ storyboard
        );

    static void ColorAnimationTests::CreateColorAnimation(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        Microsoft::UI::Xaml::Media::SolidColorBrush^ target,
        ::Windows::UI::Color fromColor,
        ::Windows::UI::Color toColor,
        int64 duration,
        bool autoReverse,
        bool repeatForever,
        Microsoft::UI::Xaml::Media::Animation::Storyboard^ storyboard
        );

    static void ColorAnimationTests::CreateColorAnimation(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        Platform::String^ brushName,
        ::Windows::UI::Color fromColor,
        ::Windows::UI::Color toColor,
        int64 duration,
        bool autoReverse,
        bool repeatForever,
        Microsoft::UI::Xaml::Media::Animation::Storyboard^ storyboard
        );

    static void ColorAnimationTests::CreateDoubleAnimation(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        int64 duration,
        int iterations,
        Microsoft::UI::Xaml::Media::Animation::Storyboard^ storyboard);

    void VerifyColor(int upperA, int lowerA, int upperR, int lowerR, int upperG, int lowerG, int upperB, int lowerB, ::Windows::UI::Color actual);
    void VerifyColor(int expectedA, int expectedR, int expectedG, int expectedB, ::Windows::UI::Color actual);

    void AnimationReadback(bool isKeyFrame);

    void OneColorAnimation(
        Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering,
        float iterations);
    void SolidColorBrushAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void ColorAndDoubleAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void ForeverColorAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void HandoffToDepObjAnimInternal();
    void NullDurationInternal();
    void SeekHandoffInternal();
};

} } } } } }

