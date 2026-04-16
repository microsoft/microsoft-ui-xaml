// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class StoryboardTests : public WEX::TestClass<StoryboardTests>
{
public:
    BEGIN_TEST_CLASS(StoryboardTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ComplexStoryboardsWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEventWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEventCollapsedTargetWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEventWUC_ZeroDuration)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEventWUC_TargetDetached)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEventWUC_BlankStoryboard)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEventWUC_ClippedAnimation)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEvent_TargetNotRenderedWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEvent_TargetNotRendered_RenderDisabledWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ColorCompletedEventWUC)
        TEST_METHOD_PROPERTY(L"Description", L"A Storyboard with a ColorAnimation inside.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ColorCompletedEvent_BrushNotRenderedWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ColorCompletedEvent_BrushNotRendered_RenderDisabledWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompletedEvent_DeviceLost)
        TEST_METHOD_PROPERTY(L"Description", L"Ensures WaitForIdle still works after device lost.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RestartStoryboardWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SuspendResumeWUC)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LongStoryboardsWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(StoryboardExpires_KeepExpression)
        TEST_METHOD_PROPERTY(L"Description", L"Start an animation and let it expire. The animation target keeps a WUC expression throughout.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(StoryboardExpires_RecreateExpression)
        TEST_METHOD_PROPERTY(L"Description", L"Start an animation and let it expire. The animation target tosses its WUC expression, then tries to recreate it.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PauseSeekResumeWUCFull)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CustomTimelineRepeat)
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(WUCAnimationTelemetry)
        // PerfAnalyzerSession fails with "Unable to load perfcore.dll" on OneCore
        // PerfAnalyzerSession fails with "The system cannot find the file specified" on WindowsCore
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore,WindowsCore")
        TEST_METHOD_PROPERTY(L"Ignore", L"True")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SeekPastEnd_FillBehaviorStop)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SeekPastEnd_RepeatDurationAndSpeedRatio)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MixedDependentAndIndependent)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    void CompletedEvent(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CompletedEvent_ZeroDuration(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CompletedEvent_TargetDetached(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CompletedEvent_BlankStoryboard(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CompletedEvent_ClippedAnimation(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CompletedEvent_DeviceLost(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void CompletedEvent_TargetNotRenderedCommon();

    void SuspendResume(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void StoryboardDeletedWhileAnimationIsRunning(bool keepExpression);

    void PauseSeekResume(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void LongStoryboardsCommon();

    void ColorCompletedEventCommon(bool hideCanvas);

    inline Platform::String^ GetResourcesPath() const;

    void ApplyCanvasProperties(Microsoft::UI::Xaml::Controls::Canvas^ canvas);
};

} } } } } }

