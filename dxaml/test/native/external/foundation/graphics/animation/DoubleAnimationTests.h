// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class DoubleAnimationTests : public WEX::TestClass<DoubleAnimationTests>
{
public:
    BEGIN_TEST_CLASS(DoubleAnimationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"02f4717a-a120-494b-a28e-9a2cbd41ca58;bd1463b3-e5f2-4d54-9394-63a431c53a6e;b7d949c9-6779-44c5-b16b-1f51e18f3866")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(RenderAnimatedCanvasWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

#if 0   // These tests take a very long time to run, and we're not currently doing anything with the measurements.
        // Disabling these tests to save precious time doing our test pass.

    //: Perf tests run longer than functional tests, so limit them to desktop (VM) for now.
    //                    Once we have HW runs, we'll expnad to more devices.
    BEGIN_TEST_METHOD(RenderAnimatedCanvasWUCFullFrameAnalysis)
        TEST_METHOD_PROPERTY(L"Classification", L"Performance")
        TEST_METHOD_PROPERTY(L"Description", L"Measure per-frame CPU and GPU cost of simple geometry animations.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderTransformAnimationWUCFullFrameAnalysis)
        TEST_METHOD_PROPERTY(L"Classification", L"Performance")
        TEST_METHOD_PROPERTY(L"Description", L"Measure per-frame CPU and GPU cost of hundreds rectangles with a TranslateTransform with one animated property.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SimulateCPGStartMenuTestWUCFullFrameAnalysis)
        TEST_METHOD_PROPERTY(L"Classification", L"Performance")
        TEST_METHOD_PROPERTY(L"Description", L"Measure per-frame CPU and GPU cost of scenario that simulates CPG Start Menu test.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ManyAnimationsWUCFullFrameAnalysis)
        TEST_METHOD_PROPERTY(L"Classification", L"Performance")
        TEST_METHOD_PROPERTY(L"Description", L"Measure per-frame CPU and GPU cost of hundreds rectangles with CompositeTransofrm and PlaneProjection with several animated properties.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()
#endif

    BEGIN_TEST_METHOD(RetargetAnimation)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimationDuration)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimationHandoffWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AnimationHandoff_DirtyFlags)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LoadedTrigger_BeginTime)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PauseResumeOpacityAnimationWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Pause and resume an opacity animation. Make sure we don't crash on a null expression.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PauseResumeOffsetAnimationWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Pause and resume an offset animation. Make sure we don't crash on a null expression.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PauseAndSeekBeforeFirstFrameWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PauseAndOverwriteWithStaticValueWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SeekAlignedToLastTickWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanvasLeftTopAnimationWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ClipAnimationWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(FillBehaviorWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityAnimationWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TrickyStoryboardsWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompleteAnimationWhileInvisibleWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NegativeKeyTimesWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EasingFunctionsWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ProjectionAnimationWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ProjectionAnimationOnZeroSizedElementWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Rotation animations on zero-sized projections get skipped. Make sure they still fire their completed events.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Transform3DAnimationWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Transform3DAnimationLifecycle)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LinearAnimationReadback)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(KeyFrameAnimationReadback)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RestartAnimationWithBaseValueReadback)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SwitchAnimationTargets)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OpacityAnimationHandoff)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NoDCompCompletion)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NullDuration)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LeaveEnterTree)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScrollBarIndicatorMode)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EmptyKeyFrameAnimation)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PixelSnappingDuringTransformAnimation)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PixelSnap2WUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformAnimationRealizations1WUCFull)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformAnimationRealizations2WUCFull)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TransformAnimationLTEChildWUCFull)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DependentAnimation)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RTLAnimationWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DeviceLostDuringAnimationWUCFull)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Test hangs
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(KeyFrameAnimationClippedByDurationWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(BounceEaseInvalidBounciness)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(BackEaseInvalidAmplitude)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DependentAnimationShouldNotRunByDefault)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    void RenderAnimatedCanvas(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void RenderAnimatedCanvasFrameAnalysis(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void RenderTransformAnimationFrameAnalysis(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void SimulateCPGStartMenuTestFrameAnalysis(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void ManyAnimationsFrameAnalysis(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void AnimationHandoff(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CanvasLeftTopAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void ClipAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void ProjectionAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void Transform3DAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void OpacityAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void TransformAnimationRealizationsHelper(bool animateRootGrid);

    void PauseAndSeekBeforeFirstFrame(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void PauseAndOverwriteWithStaticValue(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void SeekAlignedToLastTick(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void FillBehavior(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void RTLAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void PixelSnap2Internal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    inline Platform::String^ GetResourcesPath() const;

    void TestFillBehavior(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        Microsoft::UI::Xaml::Media::Animation::FillBehavior animFillBehavior,
        Microsoft::UI::Xaml::Media::Animation::FillBehavior sbFillBehavior,
        double expectedUIThreadValue,
        bool forceCompNode,
        Platform::String^ animationEndFileName,
        double staticOverrideValue,
        Platform::String^ staticOverrideFileName
        );

    void TestOpacity(
        Microsoft::UI::Xaml::Controls::Canvas^ rootCanvas,
        bool forceCompNode,
        Platform::String^ animationFileName,
        Platform::String^ stopAnimationFileName,
        double from
        );

    void AnimationReadback(bool isKeyFrame);

    Microsoft::UI::Xaml::Media::Animation::DoubleAnimationUsingKeyFrames^ DoubleAnimationTests::MakeDuration60DAUKF(Microsoft::UI::Xaml::DependencyObject^ target);

    void RenderAnimatedCanvasFrameAnalysisScenario();
    void RenderTransformAnimationFrameAnalysisScenario();
    void SimulateCPGStartMenuTestFrameAnalysisScenario();
    void ManyAnimationsFrameAnalysisScenario();
};

} } } } } }

