// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "UIElementFacadeTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "SafeEventRegistration.h"
#include "WUCRenderingScopeGuard.h"
#include <WindowsNumerics.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;

using namespace MockDComp;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics;

bool UIElementFacadeTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();

    return true;
}

bool UIElementFacadeTests::TestSetup()
{
   TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool UIElementFacadeTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void LogTransformMatrix(wfn_::float4x4 transformMatrix)
{
    LOG_OUTPUT(L"TransformMatrix is {%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f}",
    transformMatrix.m11,transformMatrix.m12,transformMatrix.m13,transformMatrix.m14,
    transformMatrix.m21,transformMatrix.m22,transformMatrix.m23,transformMatrix.m24,
    transformMatrix.m31,transformMatrix.m32,transformMatrix.m33,transformMatrix.m34,
    transformMatrix.m41,transformMatrix.m42,transformMatrix.m43,transformMatrix.m44);
}

void UIElementFacadeTests::ActualOffsetAPI()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        wfn_::float3 actualOffset = canvas->ActualOffset;
        VERIFY_IS_TRUE(actualOffset == wfn_::float3(0, 0, 0));

        Canvas::SetLeft(canvas, 50);
        Canvas::SetTop(canvas, 75);

        VERIFY_IS_TRUE(actualOffset == wfn_::float3(0, 0, 0));    // Layout must run first

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 actualOffset = canvas->ActualOffset;
        VERIFY_IS_TRUE(actualOffset == wfn_::float3(50, 75, 0));
    });

    RunOnUIThread([&]()
    {
        Compositor^ compositor = CompositionTarget::GetCompositorForCurrentThread();
        ExpressionAnimation^ animation = compositor->CreateExpressionAnimation(L"vector3(1,2,3)");
        animation->Target = L"ActualOffset";
        ExpectEAccessDenied([&]()
        {
            canvas->StartAnimation(animation);
        });
    });
}

void UIElementFacadeTests::ActualOffsetReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_shapes::Rectangle^ rectangle1;
    xaml_shapes::Rectangle^ rectangle2;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        StackPanel^ stackPanel = ref new StackPanel;

        rectangle1 = ref new xaml_shapes::Rectangle();
        rectangle1->Width = 50;
        rectangle1->Height = 50;
        rectangle1->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        rectangle2 = ref new xaml_shapes::Rectangle();
        rectangle2->Width = 50;
        rectangle2->Height = 50;
        rectangle2->Fill = ref new SolidColorBrush(mu::Colors::Green);

        stackPanel->Children->Append(rectangle1);
        stackPanel->Children->Append(rectangle2);

        wh->WindowContent = stackPanel;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualOffset");
        animation->SetExpressionReferenceParameter(L"element", rectangle2);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Offset", animation);
        scopedBatch->End();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(175, 50, 0));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualOffset");
        animation->SetExpressionReferenceParameter(L"element", rectangle2);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Offset", animation);
        scopedBatch->End();

        // Now force layout to compute a new layout offset on rectangle2
        rectangle1->Height = 100;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(175, 100, 0));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::ActualOffsetReferenceCanvas()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_shapes::Rectangle^ rectangle1;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        Canvas^ canvas = ref new Canvas();

        rectangle1 = ref new xaml_shapes::Rectangle();
        rectangle1->Width = 50;
        rectangle1->Height = 50;
        Canvas::SetLeft(rectangle1, 50);
        Canvas::SetTop(rectangle1, 50);
        rectangle1->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        canvas->Children->Append(rectangle1);

        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualOffset");
        animation->SetExpressionReferenceParameter(L"element", rectangle1);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Offset", animation);
        scopedBatch->End();

        Canvas::SetLeft(rectangle1, 100);
        Canvas::SetTop(rectangle1, 100);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(100, 100, 0));
    });
    wh->WaitForIdle();
}
void UIElementFacadeTests::ActualSizeAPI()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    StackPanel^ stackPanel;

    RunOnUIThread([&]()
    {
        stackPanel = ref new StackPanel;
        wfn_::float2 actualSize = stackPanel->ActualSize;
        VERIFY_IS_TRUE(actualSize == wfn_::float2(0, 0));

        stackPanel->Width = 50;
        stackPanel->Height = 75;

        actualSize = stackPanel->ActualSize;
        VERIFY_IS_TRUE(actualSize == wfn_::float2(0, 0));    // Layout must run first

        wh->WindowContent = stackPanel;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float2 actualSize = stackPanel->ActualSize;
        VERIFY_IS_TRUE(actualSize == wfn_::float2(50, 75));
    });

    RunOnUIThread([&]()
    {
        Compositor^ compositor = CompositionTarget::GetCompositorForCurrentThread();
        ExpressionAnimation^ animation = compositor->CreateExpressionAnimation(L"vector2(1,2)");
        animation->Target = L"ActualSize";
        ExpectEAccessDenied([&]()
        {
            stackPanel->StartAnimation(animation);
        });
    });
}

void UIElementFacadeTests::ActualSizeCanvas()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas;
        wfn_::float2 actualSize = canvas->ActualSize;
        VERIFY_IS_TRUE(actualSize == wfn_::float2(0, 0));

        canvas->Width = 50;
        canvas->Height = 75;

        actualSize = canvas->ActualSize;
        VERIFY_IS_TRUE(actualSize == wfn_::float2(50, 75));    // Canvas resizes immediately

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float2 actualSize = canvas->ActualSize;
        VERIFY_IS_TRUE(actualSize == wfn_::float2(50, 75));
    });
}

void UIElementFacadeTests::ActualSizeReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_shapes::Rectangle^ rectangle1;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        StackPanel^ stackPanel = ref new StackPanel;

        rectangle1 = ref new xaml_shapes::Rectangle();
        rectangle1->Width = 50;
        rectangle1->Height = 100;
        rectangle1->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        stackPanel->Children->Append(rectangle1);

        wh->WindowContent = stackPanel;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualSize");
        animation->SetExpressionReferenceParameter(L"element", rectangle1);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Size", animation);
        scopedBatch->End();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Size");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Size is [%f,%f]", spriteVisual->Size.x, spriteVisual->Size.y);
        VERIFY_IS_TRUE(spriteVisual->Size == wfn_::float2(50, 100));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualSize");
        animation->SetExpressionReferenceParameter(L"element", rectangle1);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Size", animation);
        scopedBatch->End();

        // Now force layout to compute a new layout size on rectangle1
        rectangle1->Width = 100;
        rectangle1->Height = 150;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Size");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Size is [%f,%f]", spriteVisual->Size.x, spriteVisual->Size.y);
        VERIFY_IS_TRUE(spriteVisual->Size == wfn_::float2(100, 150));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::ActualSizeReferenceCanvas()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        canvas = ref new Canvas();
        canvas->Width = 50;
        canvas->Height = 100;

        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualSize");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Size", animation);
        scopedBatch->End();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Size");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Size is [%f,%f]", spriteVisual->Size.x, spriteVisual->Size.y);
        VERIFY_IS_TRUE(spriteVisual->Size == wfn_::float2(50, 100));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.ActualSize");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Size", animation);
        scopedBatch->End();

        // Now push a new layout size into canvas
        canvas->Width = 100;
        canvas->Height = 150;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Size");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Size is [%f,%f]", spriteVisual->Size.x, spriteVisual->Size.y);
        VERIFY_IS_TRUE(spriteVisual->Size == wfn_::float2(100, 150));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::TranslationAPI()
{
    TranslationAPIInternal(false);
}

void UIElementFacadeTests::TranslationAPIWithClip()
{
    TranslationAPIInternal(true);
}

void UIElementFacadeTests::TranslationAPIInternal(bool useClip)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 100;
        root->Height = 100;
        root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);

        // This puts a 2X scale into prepend visual for canvas
        auto scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 2;
        root->RenderTransform = scale;

        if (useClip)
        {
            // This puts a clip into prepend visual for canvas
            auto clip = ref new RectangleGeometry();
            clip->Rect = {0,0,75,75};
            root->Clip = clip;
        }

        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(0, 0, 0));
        canvas->Translation = {10, 20, 30};
        translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(10, 20, 30));

        root->Children->Append(canvas);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting to 2D Translate");
        canvas->Translation = {10, 20, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Turning on ECP Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(canvas, true);
        auto handOffVisual = ElementCompositionPreview::GetElementVisual(canvas);
        handOffVisual->Properties->InsertVector3(L"Translation", ::Windows::Foundation::Numerics::float3(15.0f, 25.0f, 35.0f));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Changing Translation");
        canvas->Translation = {20, 30, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Turning off ECP Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(canvas, false);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing prepend clip");
        root->Clip = nullptr;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"6");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting back to default");
        canvas->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"7");
}

void UIElementFacadeTests::RotationAPI()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        FLOAT rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 0);
        canvas->Rotation = 45;
        VERIFY_IS_TRUE(canvas->Rotation == 45);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Rotation back to 0");
        canvas->Rotation = 0;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::ScaleAPI()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1, 1, 1));
        canvas->Scale = {1, 2, 3};
        scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1, 2, 3));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting Scale back to identity");
        canvas->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::TransformMatrixAPI()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float4x4 transformMatrix = canvas->TransformMatrix;
        VERIFY_IS_TRUE(transformMatrix == wfn_::float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
        canvas->TransformMatrix = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        transformMatrix = canvas->TransformMatrix;
        VERIFY_IS_TRUE(transformMatrix == wfn_::float4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting TransformMatrix back to identity");
        canvas->TransformMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::CenterPointAPI()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 centerPoint = canvas->CenterPoint;
        VERIFY_IS_TRUE(centerPoint == wfn_::float3(0, 0, 0));
        canvas->CenterPoint = {4, 5, 6};
        centerPoint = canvas->CenterPoint;
        VERIFY_IS_TRUE(centerPoint == wfn_::float3(4, 5, 6));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting CenterPoint back to default");
        canvas->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::RotationAxisAPI()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 rotationAxis = canvas->RotationAxis;
        VERIFY_IS_TRUE(rotationAxis == wfn_::float3(0, 0, 1));
        canvas->RotationAxis = {7, 8, 9};
        rotationAxis = canvas->RotationAxis;
        VERIFY_IS_TRUE(rotationAxis == wfn_::float3(7, 8, 9));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting RotationAxis back to default");
        canvas->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::CombinedAPI()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        canvas->Translation = {100, 200, 300};
        canvas->Rotation = 45;
        canvas->Scale = {1, 2, 3};
        canvas->TransformMatrix = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        canvas->CenterPoint = {4, 5, 6};
        canvas->RotationAxis = {7, 8, 9};
        VERIFY_IS_TRUE(canvas->Translation == wfn_::float3(100, 200, 300));
        VERIFY_IS_TRUE(canvas->Rotation == 45);
        VERIFY_IS_TRUE(canvas->Scale == wfn_::float3(1, 2, 3));
        VERIFY_IS_TRUE(canvas->TransformMatrix == wfn_::float4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
        VERIFY_IS_TRUE(canvas->CenterPoint == wfn_::float3(4, 5, 6));
        VERIFY_IS_TRUE(canvas->RotationAxis == wfn_::float3(7, 8, 9));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void UIElementFacadeTests::TranslationReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;

        animation = compositor->CreateExpressionAnimation(L"element.Translation");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        canvas->Translation = {1, 2, 0};

        spriteVisual->StartAnimation(L"Offset", animation);
        scopedBatch->End();

        // Now update Translation, make sure we update the backing CO
        canvas->Translation = {1, 2, 3};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(1, 2, 3));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::RotationReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.Rotation");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        // Force a backing CompNode to be created before we start referencing the facade.
        canvas->Rotation = 90;

        spriteVisual->StartAnimation(L"RotationAngleInDegrees", animation);
        scopedBatch->End();

        // Now change the property value and make sure we update the backing CompositionObject.
        canvas->Rotation = 45;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"RotationAngleInDegrees");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Rotation is %f", spriteVisual->RotationAngleInDegrees);
        VERIFY_IS_TRUE(spriteVisual->RotationAngleInDegrees == 45);
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::ScaleReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    ExpressionAnimation^ elementAnimation;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Start an arbitrary animation on the element so it already has a backing CompositionObject
        // at the time we resolve the reference to the Scale property
        elementAnimation = compositor->CreateExpressionAnimation(L"vector3(2,4,6)");
        elementAnimation->Target = L"Translation";

        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.Scale");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        LOG_OUTPUT(L"Starting canvas animation");
        canvas->StartAnimation(elementAnimation);

        // Now reference the Scale facade
        LOG_OUTPUT(L"Starting SpriteVisual animation");
        spriteVisual->StartAnimation(L"Scale", animation);
        scopedBatch->End();

        LOG_OUTPUT(L"Setting Scale");
        canvas->Scale = {1, 2, 3};
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        canvas->StopAnimation(elementAnimation);
        spriteVisual->StopAnimation(L"Scale");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Scale is [%f,%f,%f]", spriteVisual->Scale.x, spriteVisual->Scale.y, spriteVisual->Scale.z);
        VERIFY_IS_TRUE(spriteVisual->Scale == wfn_::float3(1, 2, 3));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::TransformMatrixReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.TransformMatrix");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"TransformMatrix", animation);
        scopedBatch->End();

        canvas->TransformMatrix = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"TransformMatrix");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LogTransformMatrix(spriteVisual->TransformMatrix);
        VERIFY_IS_TRUE(spriteVisual->TransformMatrix == wfn_::float4x4(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::CenterPointReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.CenterPoint");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"CenterPoint", animation);
        scopedBatch->End();

        canvas->CenterPoint = {1, 2, 3};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"CenterPoint");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"CenterPoint is [%f,%f,%f]", spriteVisual->CenterPoint.x, spriteVisual->CenterPoint.y, spriteVisual->CenterPoint.z);
        VERIFY_IS_TRUE(spriteVisual->CenterPoint == wfn_::float3(1, 2, 3));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::RotationAxisReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.RotationAxis");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"RotationAxis", animation);
        scopedBatch->End();

        canvas->RotationAxis = {1, 2, 3};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"RotationAxis");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"RotationAxis is [%f,%f,%f]", spriteVisual->RotationAxis.x, spriteVisual->RotationAxis.y, spriteVisual->RotationAxis.z);
        VERIFY_IS_TRUE(spriteVisual->RotationAxis == wfn_::float3(1, 2, 3));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::OpacityReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.Opacity");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Opacity", animation);
        scopedBatch->End();

        canvas->Opacity = 0.5;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Opacity");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Opacity is %f", spriteVisual->Opacity);
        VERIFY_IS_TRUE(spriteVisual->Opacity == 0.5);
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::SubChannelReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.Translation.X");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Offset.X", animation);
        scopedBatch->End();

        // Now update Translation, make sure we update the backing CO
        canvas->Translation = {1, 2, 3};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(1, 0, 0));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::MultiReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.Translation + element.Scale + vector3(element.Opacity, element.Opacity, element.Opacity)");
        animation->SetExpressionReferenceParameter(L"element", canvas);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        // First set some properties
        canvas->Translation = {1, 0, 0};
        canvas->Scale = {2, 1, 1};
        canvas->Opacity = 0.75;

        spriteVisual->StartAnimation(L"Offset", animation);
        scopedBatch->End();

        // Now update the properties, make sure they get updated in backing CompositionObject
        canvas->Translation = {1, 2, 3};
        canvas->Scale = {2, 4, 6};
        canvas->Opacity = 0.5;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(3.5, 6.5, 9.5));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::InvalidReference()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wh->WindowContent = canvas;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation1 = compositor->CreateExpressionAnimation(L"element.FooBazProperty");
        animation1->SetExpressionReferenceParameter(L"element", canvas);

        ExpectEInvalidArg([&]()
        {
            spriteVisual->StartAnimation(L"Opacity", animation1);
        });

        ExpressionAnimation^ animation2 = compositor->CreateExpressionAnimation(L"element.Translation.FooBazProperty");
        animation2->SetExpressionReferenceParameter(L"element", canvas);

        ExpectEInvalidArg([&]()
        {
            spriteVisual->StartAnimation(L"Opacity", animation2);
        });

    });
    wh->WaitForIdle();
}

ref class MyPanel : public Panel
{
protected:
    void PopulatePropertyInfoOverride(Platform::String^ propertyName, AnimationPropertyInfo^ animationPropertyInfo) override
    {
        if (propertyName == L"FooBazProperty")
        {
            return Panel::PopulatePropertyInfoOverride(L"Translation", animationPropertyInfo);
        }

        return Panel::PopulatePropertyInfoOverride(propertyName, animationPropertyInfo);
    }
};

void UIElementFacadeTests::PopulatePropertyInfoOverride()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    MyPanel^ myPanel;
    Compositor^ compositor;
    SpriteVisual^ spriteVisual;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        myPanel = ref new MyPanel();
        // Custom types like MyPanel are not supported in the WPF/WinRT interop layer
        // We need to wrap our custom type via a public activatable WinRT type
        // In this case we wrapped with a Grid.
        auto root = ref new Grid();
        root->Children->Append(myPanel);
        wh->WindowContent = root;

        compositor = CompositionTarget::GetCompositorForCurrentThread();
        spriteVisual = compositor->CreateSpriteVisual();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ExpressionAnimation^ animation;
        animation = compositor->CreateExpressionAnimation(L"element.FooBazProperty + element.Translation");
        animation->SetExpressionReferenceParameter(L"element", myPanel);

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });

        spriteVisual->StartAnimation(L"Offset", animation);
        scopedBatch->End();

        myPanel->Translation = {1, 2, 3};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        spriteVisual->StopAnimation(L"Offset");
    });
    wh->WaitForIdle();
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Offset is [%f,%f,%f]", spriteVisual->Offset.x, spriteVisual->Offset.y, spriteVisual->Offset.z);
        VERIFY_IS_TRUE(spriteVisual->Offset == wfn_::float3(2, 4, 6));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::RedirectionTransform()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Primitives::Popup^ popup;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();
        canvas->Translation = {10, 20, 0};

        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        wh->WindowContent = canvas;

        popup = ref new Primitives::Popup();
        popup->Child = rect;

        {
            auto xamlRoot = canvas->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup->XamlRoot = xamlRoot;
            }
        }
        popup->IsOpen = true;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        popup->Translation = {30, 40, 0};
        popup->Scale = {2, 2, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        canvas->Children->Append(popup);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void UIElementFacadeTests::TranslationAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Vector3KeyFrameAnimation^ animation;
    Vector3KeyFrameAnimation^ animation2;
    Vector3KeyFrameAnimation^ animation3;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(100.0f, 200.0f, 300.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Translation";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation (short time span)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector3KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float3(10.0f, 20.0f, 30.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "Translation";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (2D) Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation3 = compositor->CreateVector3KeyFrameAnimation();
        animation3->InsertKeyFrame(1.0, wfn_::float3(100.0f, 200.0f, 0.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation3->Duration = span;
        animation3->Target = "Translation";
        animation3->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation3);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");
}

void UIElementFacadeTests::TranslationAnimationPlusECP()
{
    TranslationAnimationPlusECPInternal(false);
}

void UIElementFacadeTests::TranslationAnimationPlusECPAndClip()
{
    TranslationAnimationPlusECPInternal(true);
}

void UIElementFacadeTests::TranslationAnimationPlusECPInternal(bool useClip)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    Canvas^ canvas;
    ExpressionAnimation^ animation;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 100;
        root->Height = 100;
        root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);

        // This puts a 2X scale into prepend visual for canvas
        auto scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 2;
        root->RenderTransform = scale;

        if (useClip)
        {
            // This puts a clip into prepend visual for canvas
            auto clip = ref new RectangleGeometry();
            clip->Rect = {0,0,75,75};
            root->Clip = clip;
        }

        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(0, 0, 0));
        canvas->Translation = {11, 21, 31};
        translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(11, 21, 31));

        root->Children->Append(canvas);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateExpressionAnimation(L"vector3(10, 20, 30)");
        animation->Target = "Translation";

        canvas->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Turning on ECP Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(canvas, true);
        auto handOffVisual = ElementCompositionPreview::GetElementVisual(canvas);
        handOffVisual->Properties->InsertVector3(L"Translation", ::Windows::Foundation::Numerics::float3(15.0f, 25.0f, 35.0f));
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Turning off ECP Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(canvas, false);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing prepend clip");
        root->Clip = nullptr;
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        canvas->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"6");
}

void UIElementFacadeTests::TranslationAnimationSubChannel()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation1;
    Vector2KeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation1 = compositor->CreateScalarKeyFrameAnimation();
        animation1->InsertKeyFrame(1.0, 100);
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation1->Duration = span;
        animation1->Target = "translation.X";
        animation1->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 0));

        rect->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 2");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector2KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float2(50, 75));
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation2->Duration = span;
        animation2->Target = "translation.XY";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(50, 75, 0));

        rect->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 1 again");
        rect->StartAnimation(animation1);

        bool testPass = false;
        try
        {
            LOG_OUTPUT(L"Trying to simultaneously start 2nd animation on different sub-channels, should fail");
            rect->StartAnimation(animation2);
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 0));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::TranslationPlusLTETarget()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 100;
        root->Height = 100;
        root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Translation = {50, 50, 0};
        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = ::Windows::Foundation::Rect(0, 0, 100, 100);
        root->Clip = clip;

        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 translation = canvas->Translation;
        canvas->Translation = {10, 20, 30};

        root->Children->Append(canvas);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        wh->AddTestLTE(canvas /* target */, root /* parent */, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::TranslationPlusLTETarget2()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    Canvas^ child;
    Canvas^ target;
    UIElement^ lte;

    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        root->Width = 100;
        root->Height = 100;
        root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        root->Translation = {50, 50, 0};
        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = ::Windows::Foundation::Rect(0, 0, 100, 100);
        root->Clip = clip;

        // Now create an element that will generate a CompNode and serve as the LTE's TransformParent.
        // This element has a transform which exposed an issue.
        child = ref new Canvas();
        child->Translation = {-10, -20, 30};

        target = ref new Canvas();
        target->Width = 100;
        target->Height = 100;
        target->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 translation = target->Translation;
        target->Translation = {20, 40, 30};

        root->Children->Append(child);
        child->Children->Append(target);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        lte = wh->AddTestLTE(target /* target */, root /* parent */, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        wh->RemoveTestLTE(lte);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3"); // A VisualId is now present and is why we need version 3 baseline
}

void UIElementFacadeTests::RotationAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation;
    ScalarKeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateScalarKeyFrameAnimation();
        animation->InsertKeyFrame(1.0, 45);
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Rotation";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->Rotation = 0;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation (short time span)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->Rotation = 0;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->Rotation = 0;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateScalarKeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, 90);
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "Rotation";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::ScaleAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Vector3KeyFrameAnimation^ animation;
    Vector3KeyFrameAnimation^ animation2;
    Vector3KeyFrameAnimation^ animation3;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(1.0f, 2.0f, 3.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Scale";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation (short time span)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector3KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float3(2.0f, 3.0f, 4.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "Scale";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    // TODO - add a test case that takes scale back to a non-identity, 2D transform
    // after the GetLocalTransform changes are made
}

void UIElementFacadeTests::ScaleAnimationSubChannel()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation1;
    Vector2KeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation1 = compositor->CreateScalarKeyFrameAnimation();
        animation1->InsertKeyFrame(1.0, 2);
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation1->Duration = span;
        animation1->Target = "scale.X";
        animation1->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 scale = rect->Scale;
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 1, 1));

        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 2");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector2KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float2(2, 3));
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation2->Duration = span;
        animation2->Target = "scale.XY";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 scale = rect->Scale;
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 3, 1));

        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 1 again");
        rect->StartAnimation(animation1);

        bool testPass = false;
        try
        {
            LOG_OUTPUT(L"Trying to simultaneously start 2nd animation on different sub-channels, should fail");
            rect->StartAnimation(animation2);
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 scale = rect->Scale;
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 1, 1));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::TransformMatrixAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ExpressionAnimation^ animation;
    ExpressionAnimation^ animation2;
    ExpressionAnimation^ animation3;

    wfn_::float4x4 identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateExpressionAnimation(L"Matrix4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ,16)");
        animation->Target = "TransformMatrix";

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->TransformMatrix = identity;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->TransformMatrix = identity;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->TransformMatrix = identity;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateExpressionAnimation(L"Matrix4x4(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32)");
        animation2->Target = "TransformMatrix";

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->TransformMatrix = identity;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (identity) Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation3 = compositor->CreateExpressionAnimation(L"Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)");
        animation3->Target = "TransformMatrix";

        rect->StartAnimation(animation3);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation3);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");
}

void UIElementFacadeTests::TransformMatrixAnimationSubChannel()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation1;
    Vector2KeyFrameAnimation^ animation2;

    wfn_::float4x4 identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation1 = compositor->CreateScalarKeyFrameAnimation();
        animation1->InsertKeyFrame(1.0, 2);
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation1->Duration = span;
        animation1->Target = "TransformMatrix._11";
        animation1->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float4x4 transformMatrix = rect->TransformMatrix;
        LogTransformMatrix(transformMatrix);
        VERIFY_IS_TRUE(transformMatrix == wfn_::float4x4(2,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1));

        rect->TransformMatrix = identity;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 2");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector2KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float2(2, 3));
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation2->Duration = span;
        animation2->Target = "TransformMatrix._11_22";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float4x4 transformMatrix = rect->TransformMatrix;
        LogTransformMatrix(transformMatrix);
        VERIFY_IS_TRUE(transformMatrix == wfn_::float4x4(2,0,0,0,0,3,0,0,0,0,1,0,0,0,0,1));

        rect->TransformMatrix = identity;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 1 again");
        rect->StartAnimation(animation1);

        bool testPass = false;
        try
        {
            LOG_OUTPUT(L"Trying to simultaneously start 2nd animation on different sub-channels, should fail");
            rect->StartAnimation(animation2);
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float4x4 transformMatrix = rect->TransformMatrix;
        LogTransformMatrix(transformMatrix);
        VERIFY_IS_TRUE(transformMatrix == wfn_::float4x4(2,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::CenterPointAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Vector3KeyFrameAnimation^ animation;
    Vector3KeyFrameAnimation^ animation2;
    Vector3KeyFrameAnimation^ animation3;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(10.0f, 20.0f, 30.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "CenterPoint";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation (short time span)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector3KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float3(20.0f, 30.0f, 40.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "CenterPoint";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");
}

void UIElementFacadeTests::CenterPointAnimationSubChannel()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation1;
    Vector2KeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation1 = compositor->CreateScalarKeyFrameAnimation();
        animation1->InsertKeyFrame(1.0, 50);
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation1->Duration = span;
        animation1->Target = "CenterPoint.X";
        animation1->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 centerPoint = rect->CenterPoint;
        LOG_OUTPUT(L"CenterPoint is {%f,%f,%f}", centerPoint.x, centerPoint.y, centerPoint.z);
        VERIFY_IS_TRUE(centerPoint == wfn_::float3(50, 0, 0));

        rect->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 2");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector2KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float2(50, 75));
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation2->Duration = span;
        animation2->Target = "CenterPoint.XY";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 centerPoint = rect->CenterPoint;
        LOG_OUTPUT(L"CenterPoint is {%f,%f,%f}", centerPoint.x, centerPoint.y, centerPoint.z);
        VERIFY_IS_TRUE(centerPoint == wfn_::float3(50, 75, 0));

        rect->CenterPoint = {0, 0, 0};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 1 again");
        rect->StartAnimation(animation1);

        bool testPass = false;
        try
        {
            LOG_OUTPUT(L"Trying to simultaneously start 2nd animation on different sub-channels, should fail");
            rect->StartAnimation(animation2);
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 centerPoint = rect->CenterPoint;
        LOG_OUTPUT(L"CenterPoint is {%f,%f,%f}", centerPoint.x, centerPoint.y, centerPoint.z);
        VERIFY_IS_TRUE(centerPoint == wfn_::float3(50, 0, 0));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::RotationAxisAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Vector3KeyFrameAnimation^ animation;
    Vector3KeyFrameAnimation^ animation2;
    Vector3KeyFrameAnimation^ animation3;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(1.0f, 2.0f, 3.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "RotationAxis";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation (short time span)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector3KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float3(2.0f, 3.0f, 4.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "RotationAxis";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");
}

void UIElementFacadeTests::RotationAxisAnimationSubChannel()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation1;
    Vector2KeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation1 = compositor->CreateScalarKeyFrameAnimation();
        animation1->InsertKeyFrame(1.0, 1);
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation1->Duration = span;
        animation1->Target = "RotationAxis.X";
        animation1->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation1);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 rotationAxis = rect->RotationAxis;
        LOG_OUTPUT(L"RotationAxis is {%f,%f,%f}", rotationAxis.x, rotationAxis.y, rotationAxis.z);
        VERIFY_IS_TRUE(rotationAxis == wfn_::float3(1, 0, 1));

        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 2");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector2KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float2(1, 0));
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation2->Duration = span;
        animation2->Target = "RotationAxis.XZ";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 rotationAxis = rect->RotationAxis;
        LOG_OUTPUT(L"RotationAxis is {%f,%f,%f}", rotationAxis.x, rotationAxis.y, rotationAxis.z);
        VERIFY_IS_TRUE(rotationAxis == wfn_::float3(1, 0, 0));

        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation 1 again");
        rect->StartAnimation(animation1);

        bool testPass = false;
        try
        {
            LOG_OUTPUT(L"Trying to simultaneously start 2nd animation on different sub-channels, should fail");
            rect->StartAnimation(animation2);
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            testPass = true;
        }
        VERIFY_IS_TRUE(testPass);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 rotationAxis = rect->RotationAxis;
        LOG_OUTPUT(L"RotationAxis is {%f,%f,%f}", rotationAxis.x, rotationAxis.y, rotationAxis.z);
        VERIFY_IS_TRUE(rotationAxis == wfn_::float3(1, 0, 1));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::OpacityAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ScalarKeyFrameAnimation^ animation;
    ScalarKeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateScalarKeyFrameAnimation();
        animation->InsertKeyFrame(1.0, 0.5);
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Opacity";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"Simulating device lost");
    wh->SimulateDeviceLost();
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        rect->Opacity = 1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation (short time span)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;

        rect->StartAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(1);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Interrupting animation with static value");
        rect->Opacity = 1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Resetting before scoped batch completes");
        rect->Opacity = 1;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateScalarKeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, 0.25);
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "Opacity";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting (HandOff) Animation");
        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Opacity=0");
        rect->Opacity = 0;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Culled");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Starting animation on Opacity=0 element.");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateScalarKeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, 0.25);
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "Opacity";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Stopping animation. The element should snap to the final opacity.");
            // via FacadeAnimationHelper::ScopedBatchCompleted's PullFacadePropertyValueFromCompositionObject

        rect->StopAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void UIElementFacadeTests::MultiAnimation()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Vector3KeyFrameAnimation^ animation;
    Vector3KeyFrameAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Pre");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animations");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(10.0f, 20.0f, 30.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Translation";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        animation2 = compositor->CreateVector3KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float3(2.0f, 2.0f, 1.0f));
        animation2->Duration = span;
        animation2->Target = "Scale";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        rect->StartAnimation(animation);
        rect->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animations");
        rect->StopAnimation(animation);
        rect->StopAnimation(animation2);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");
}

void UIElementFacadeTests::AnimationAndReference()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect1;
    xaml_shapes::Rectangle^ rect2;
    ExpressionAnimation^ animation1;
    ExpressionAnimation^ animation2;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect1 = ref new xaml_shapes::Rectangle();
        rect1->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect1->Width = 100;
        rect1->Height = 100;

        rect2 = ref new xaml_shapes::Rectangle();
        rect2->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Green);
        rect2->Width = 100;
        rect2->Height = 100;
        Canvas::SetLeft(rect2, 100);

        canvas->Children->Append(rect1);
        canvas->Children->Append(rect2);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animations");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation1 = compositor->CreateExpressionAnimation(L"45");
        animation1->Target = "Rotation";

        animation2 = compositor->CreateExpressionAnimation(L"r1.Rotation");
        animation2->SetExpressionReferenceParameter(L"r1", rect1);
        animation2->Target = "Rotation";

        rect1->StartAnimation(animation1);
        rect2->StartAnimation(animation2);
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animations");
        rect1->StopAnimation(animation1);
        rect2->StopAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"rect1->Rotation is %f", rect1->Rotation);
        VERIFY_IS_TRUE(rect1->Rotation == 45);

        LOG_OUTPUT(L"rect2->Rotation is %f", rect2->Rotation);
        VERIFY_IS_TRUE(rect2->Rotation == 45);
    });
}

void UIElementFacadeTests::ExpectExceptionWithHRESULT(HRESULT expected, std::function<void()> functionCall)
{
    bool testPass = false;
    try
    {
        functionCall();
    }
    catch (Platform::Exception^ e)
    {
        VERIFY_IS_TRUE(e->HResult == expected);
        testPass = true;
    }
    VERIFY_IS_TRUE(testPass);
}

void UIElementFacadeTests::ExpectEInvalidArg(std::function<void()> functionCall)
{
    ExpectExceptionWithHRESULT(E_INVALIDARG, functionCall);
}

void UIElementFacadeTests::ExpectEAccessDenied(std::function<void()> functionCall)
{
    ExpectExceptionWithHRESULT(E_ACCESSDENIED, functionCall);
}

void UIElementFacadeTests::ExpectSuccess(std::function<void()> functionCall)
{
    bool testPass = true;
    try
    {
        functionCall();
    }
    catch (Platform::Exception^ e)
    {
        testPass = false;
    }
    VERIFY_IS_TRUE(testPass);
}

void UIElementFacadeTests::InvalidAnimation()
{
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();

        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        auto animation = compositor->CreateVector3KeyFrameAnimation();

        LOG_OUTPUT(L"Validating TranslationFooBar");
        animation->Target = "TranslationFooBar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating TranslationFoo.Bar");
        animation->Target = "TranslationFoo.Bar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating ScaleFooBar");
        animation->Target = "ScaleFooBar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating ScaleFoo.Bar");
        animation->Target = "ScaleFoo.Bar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating CenterPointFooBar");
        animation->Target = "CenterPointFooBar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating CenterPointFoo.Bar");
        animation->Target = "CenterPointFoo.Bar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating RotationAxisFooBar");
        animation->Target = "RotationAxisFooBar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        LOG_OUTPUT(L"Validating RotationAxisFoo.Bar");
        animation->Target = "RotationAxisFoo.Bar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation);
        });

        auto animation2 = compositor->CreateScalarKeyFrameAnimation();
        LOG_OUTPUT(L"Validating RotationFooBar");
        animation2->Target = "RotationFooBar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation2);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation2);
        });

        LOG_OUTPUT(L"Validating Rotation.Foo");
        animation2->Target = "Rotation.Foo";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation2);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation2);
        });

        auto animation3 = compositor->CreateExpressionAnimation();
        LOG_OUTPUT(L"Validating TransformMatrixFooBar");
        animation3->Target = "TransformMatrixFooBar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation3);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation3);
        });

        LOG_OUTPUT(L"Validating TransformMatrixFoo.Bar");
        animation3->Target = "TransformMatrixFoo.Bar";
        ExpectEInvalidArg([&]()
        {
            canvas->StartAnimation(animation3);
        });
        ExpectEInvalidArg([&]()
        {
            canvas->StopAnimation(animation3);
        });
    });
}

void UIElementFacadeTests::Configuration2RTL()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = ::Windows::Foundation::Rect(10, 20, 30, 40);
        rect->Clip = clip;

        LOG_OUTPUT(L"Adding facades");
        rect->Translation = {10, 20, 30};
        rect->Rotation = 45;
        rect->Scale = {2, 2, 2};
        rect->TransformMatrix = {1.5, 0, 0, 0, 0, 1.5, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        rect->CenterPoint = {50, 50, 0};
        rect->RotationAxis = {1, 0, 0};

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding RTL");
        rect->FlowDirection = xaml::FlowDirection::RightToLeft;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Removing RTL");
        rect->FlowDirection = xaml::FlowDirection::LeftToRight;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");
}

void UIElementFacadeTests::Configuration2ScrollViewer()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        ScrollViewer^ sv = ref new ScrollViewer();
        sv->Width = 100;
        sv->Height = 100;
        sv->HorizontalAlignment = HorizontalAlignment::Center;
        sv->VerticalAlignment = VerticalAlignment::Center;
        sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
        sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;
        sv->HorizontalScrollMode = ScrollMode::Enabled;
        sv->VerticalScrollMode = ScrollMode::Enabled;

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = ::Windows::Foundation::Rect(10, 20, 30, 40);
        rect->Clip = clip;

        sv->Content = rect;
        canvas->Children->Append(sv);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding facades");
        rect->Translation = {10, 20, 30};
        rect->Rotation = 45;
        rect->Scale = {2, 2, 2};
        rect->TransformMatrix = {1.5, 0, 0, 0, 0, 1.5, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        rect->CenterPoint = {50, 50, 0};
        rect->RotationAxis = {1, 0, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting facades back to default, expect to keep Components visual");
        rect->Translation = {0, 0, 0};
        rect->Rotation = 0;
        rect->Scale = {1, 1, 1};
        rect->TransformMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        rect->CenterPoint = {0, 0, 0};
        rect->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
}

void UIElementFacadeTests::AnimationGroup()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    Vector3KeyFrameAnimation^ animation;
    Vector3KeyFrameAnimation^ animation2;
    CompositionAnimationGroup^ animationGroup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting AnimationGroup");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(10.0f, 20.0f, 30.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Translation";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        animation2 = compositor->CreateVector3KeyFrameAnimation();
        animation2->InsertKeyFrame(1.0, wfn_::float3(2.0f, 2.0f, 1.0f));
        animation2->Duration = span;
        animation2->Target = "Scale";
        animation2->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        animationGroup = compositor->CreateAnimationGroup();
        animationGroup->Add(animation);
        animationGroup->Add(animation2);

        rect->StartAnimation(animationGroup);
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        rect->StopAnimation(animationGroup);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        wfn_::float3 scale = rect->Scale;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(10, 20, 30));
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 2, 1));

        rect->Translation = {0, 0, 0};
        rect->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting AnimationGroup (short time)");
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation->Duration = span;
        animation2->Duration = span;
        rect->StartAnimation(animationGroup);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        wfn_::float3 scale = rect->Scale;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(10, 20, 30));
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 2, 1));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting AnimationGroup");
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation2->Duration = span;
        rect->StartAnimation(animationGroup);
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Simulating first animation completing");
        rect->StopAnimation(animation);

        LOG_OUTPUT(L"Ensure StopAnimation on animation that was just stopped doesn't fail");
        rect->StopAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping entire animation group with one remaining running animation, should succeed");
        rect->StopAnimation(animationGroup);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        wfn_::float3 scale = rect->Scale;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(10, 20, 30));
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 2, 1));
    });
    wh->WaitForIdle();

}

static Vector3KeyFrameAnimation^ CreateHTAnimationVector3(Compositor^ compositor, wfn_::float3 value, Platform::String^ target)
{
    auto animation = compositor->CreateVector3KeyFrameAnimation();
    animation->InsertKeyFrame(0.0, value);
    animation->InsertKeyFrame(1.0, value);
    ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
    animation->Duration = span;
    animation->Target = target;

    return animation;
}

static bool CompareFloatsWithEpsilon(float a, float b, float epsilon)
{
    return (a - b <= epsilon) && (b - a <= epsilon);
}

static void VerifyPointWithEpsilon(wf::Point pt1, wf::Point pt2, float epsilon)
{
    LOG_OUTPUT(L"Comparing {%f,%f} to {%f,%f}", pt1.X, pt1.Y, pt2.X, pt2.Y);
    VERIFY_IS_TRUE(CompareFloatsWithEpsilon(pt1.X, pt2.X, epsilon));
    VERIFY_IS_TRUE(CompareFloatsWithEpsilon(pt1.Y, pt2.Y, epsilon));
}

void UIElementFacadeTests::HitTestingAnimated()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Vector3KeyFrameAnimation^ animation1;
    ScalarKeyFrameAnimation^ animation2;
    Vector3KeyFrameAnimation^ animation3;
    Vector3KeyFrameAnimation^ animation4;
    Vector3KeyFrameAnimation^ animation5;
    Vector2KeyFrameAnimation^ animation6;

    Compositor^ compositor;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

        wh->WindowContent = canvas;
        compositor = CompositionTarget::GetCompositorForCurrentThread();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Translation");

        canvas->Translation = {100, 100, 0};
        animation1 = CreateHTAnimationVector3(compositor, wfn_::float3(50, 50, 0), "Translation");
        canvas->StartAnimation(animation1);

        LOG_OUTPUT(L"Verifying initial value");
        wfn_::float3 animatedTranslation = wh->GetAnimatedTranslation(canvas);
        LOG_OUTPUT(L"AnimatedTranslation is {%f,%f,%f}", animatedTranslation.x, animatedTranslation.y, animatedTranslation.z);
        VERIFY_IS_TRUE(animatedTranslation == wfn_::float3(100, 100, 0));
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated value");
        wfn_::float3 animatedTranslation = wh->GetAnimatedTranslation(canvas);
        LOG_OUTPUT(L"AnimatedTranslation is {%f,%f,%f}", animatedTranslation.x, animatedTranslation.y, animatedTranslation.z);
        VERIFY_IS_TRUE(animatedTranslation == wfn_::float3(50, 50, 0));

        LOG_OUTPUT(L"Verifying TransformToVisual");
        auto transform = canvas->TransformToVisual(nullptr);
        wf::Point bottomRight = transform->TransformPoint(wf::Point(100, 100));
        VerifyPointWithEpsilon(bottomRight, wf::Point(150, 150), 0);

        canvas->StopAnimation(animation1);
        canvas->Translation = {0, 0, 0};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Rotation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();

        canvas->Rotation = 45;
        animation2 = compositor->CreateScalarKeyFrameAnimation();
        animation2->InsertKeyFrame(0.0, 90);
        animation2->InsertKeyFrame(1.0, 90);
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation2->Duration = span;
        animation2->Target = "Rotation";

        canvas->StartAnimation(animation2);

        LOG_OUTPUT(L"Verifying initial value");
        float animatedRotation = wh->GetAnimatedRotation(canvas);
        LOG_OUTPUT(L"AnimatedRotation is %f", animatedRotation);
        VERIFY_IS_TRUE(animatedRotation == 45);
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated value");
        float animatedRotation = wh->GetAnimatedRotation(canvas);
        LOG_OUTPUT(L"AnimatedRotation is %f", animatedRotation);
        VERIFY_IS_TRUE(animatedRotation == 90);

        LOG_OUTPUT(L"Verifying TransformToVisual");
        auto transform = canvas->TransformToVisual(nullptr);
        wf::Point bottomRight = transform->TransformPoint(wf::Point(100, 100));
        VerifyPointWithEpsilon(bottomRight, wf::Point(-100, 100), 0.01f);

        canvas->StopAnimation(animation2);
        canvas->Rotation = 0;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Scale");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();

        canvas->Scale = {2, 2, 1};
        animation3 = CreateHTAnimationVector3(compositor, wfn_::float3(1.5, 1.5, 1), "Scale");
        canvas->StartAnimation(animation3);

        LOG_OUTPUT(L"Verifying initial value");
        wfn_::float3 animatedScale = wh->GetAnimatedScale(canvas);
        LOG_OUTPUT(L"AnimatedScale is {%f,%f,%f}", animatedScale.x, animatedScale.y, animatedScale.z);
        VERIFY_IS_TRUE(animatedScale == wfn_::float3(2, 2, 1));
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated value");
        wfn_::float3 animatedScale = wh->GetAnimatedScale(canvas);
        LOG_OUTPUT(L"AnimatedScale is {%f,%f,%f}", animatedScale.x, animatedScale.y, animatedScale.z);
        VERIFY_IS_TRUE(animatedScale == wfn_::float3(1.5, 1.5, 1));

        LOG_OUTPUT(L"Verifying TransformToVisual");
        auto transform = canvas->TransformToVisual(nullptr);
        wf::Point bottomRight = transform->TransformPoint(wf::Point(100,100));
        VerifyPointWithEpsilon(bottomRight, wf::Point(150, 150), 0);

        canvas->StopAnimation(animation3);
        canvas->Scale = {1, 1, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"RotationAxis");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        canvas->RotationAxis = {0, 1, 0};
        animation4 = CreateHTAnimationVector3(compositor, wfn_::float3(1, 0, 0), "RotationAxis");
        canvas->StartAnimation(animation4);

        LOG_OUTPUT(L"Verifying initial value");
        wfn_::float3 animatedRotationAxis = wh->GetAnimatedRotationAxis(canvas);
        LOG_OUTPUT(L"AnimatedRotationAxis is {%f,%f,%f}", animatedRotationAxis.x, animatedRotationAxis.y, animatedRotationAxis.z);
        VERIFY_IS_TRUE(animatedRotationAxis == wfn_::float3(0, 1, 0));
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated value");
        wfn_::float3 animatedRotationAxis = wh->GetAnimatedRotationAxis(canvas);
        LOG_OUTPUT(L"AnimatedRotationAxis is {%f,%f,%f}", animatedRotationAxis.x, animatedRotationAxis.y, animatedRotationAxis.z);
        VERIFY_IS_TRUE(animatedRotationAxis == wfn_::float3(1, 0, 0));

        canvas->StopAnimation(animation4);
        canvas->RotationAxis = {0, 0, 1};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"CenterPoint");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        canvas->CenterPoint = {25, 25, 0};
        canvas->Rotation = 90;
        animation5 = CreateHTAnimationVector3(compositor, wfn_::float3(50, 50, 0), "CenterPoint");
        canvas->StartAnimation(animation5);

        LOG_OUTPUT(L"Verifying initial value");
        wfn_::float3 animatedCenterPoint = wh->GetAnimatedCenterPoint(canvas);
        LOG_OUTPUT(L"AnimatedCenterPoint is {%f,%f,%f}", animatedCenterPoint.x, animatedCenterPoint.y, animatedCenterPoint.z);
        VERIFY_IS_TRUE(animatedCenterPoint == wfn_::float3(25, 25, 0));
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated value");
        wfn_::float3 animatedCenterPoint = wh->GetAnimatedCenterPoint(canvas);
        LOG_OUTPUT(L"AnimatedCenterPoint is {%f,%f,%f}", animatedCenterPoint.x, animatedCenterPoint.y, animatedCenterPoint.z);
        VERIFY_IS_TRUE(animatedCenterPoint == wfn_::float3(50, 50, 0));

        LOG_OUTPUT(L"Verifying TransformToVisual");
        auto transform = canvas->TransformToVisual(nullptr);
        wf::Point bottomRight = transform->TransformPoint(wf::Point(100, 100));
        VerifyPointWithEpsilon(bottomRight, wf::Point(0, 100), 0.01f);

        canvas->StopAnimation(animation5);
        canvas->CenterPoint = {0, 0, 0};
        canvas->Rotation = 0;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"TransformMatrix");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        canvas->TransformMatrix =  {1.25, 0, 0, 0, 0, 1.25, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        animation6 = compositor->CreateVector2KeyFrameAnimation();
        animation6->InsertKeyFrame(0.0, wfn_::float2(1.5, 1.5));
        animation6->InsertKeyFrame(1.0, wfn_::float2(1.5, 1.5));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation6->Duration = span;
        animation6->Target = "TransformMatrix._11_22";

        canvas->StartAnimation(animation6);

        LOG_OUTPUT(L"Verifying initial value");
        wfn_::float4x4 animatedTransformMatrix = wh->GetAnimatedTransformMatrix(canvas);
        LogTransformMatrix(animatedTransformMatrix);
        VERIFY_IS_TRUE(animatedTransformMatrix == wfn_::float4x4(1.25, 0, 0, 0, 0, 1.25, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated value");
        wfn_::float4x4 animatedTransformMatrix = wh->GetAnimatedTransformMatrix(canvas);
        LogTransformMatrix(animatedTransformMatrix);
        VERIFY_IS_TRUE(animatedTransformMatrix == wfn_::float4x4(1.5, 0, 0, 0, 0, 1.5, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));

        LOG_OUTPUT(L"Verifying TransformToVisual");
        auto transform = canvas->TransformToVisual(nullptr);
        wf::Point bottomRight = transform->TransformPoint(wf::Point(100,100));
        VerifyPointWithEpsilon(bottomRight, wf::Point(150, 150), 0);

        canvas->StopAnimation(animation6);
        canvas->TransformMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::HitTestingAnimatedAndReferenced()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    ExpressionAnimation^ animation1;
    ExpressionAnimation^ animation2;
    ExpressionAnimation^ animation3;
    CompositionPropertySet^ propertySet;

    Compositor^ compositor;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

        wh->WindowContent = canvas;
        compositor = CompositionTarget::GetCompositorForCurrentThread();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Adding references and animations");
        animation1 = compositor->CreateExpressionAnimation(L"Vector3(100, 100, 0)");
        animation1->Target = L"Translation";
        animation2 = compositor->CreateExpressionAnimation(L"Vector3(2, 2, 1)");
        animation2->Target = L"Scale";
        animation3 = compositor->CreateExpressionAnimation(L"element.Scale + element.Translation");
        animation3->SetExpressionReferenceParameter(L"element", canvas);

        propertySet = compositor->CreatePropertySet();
        propertySet->InsertVector3(L"dummy", {0, 0, 0});
        propertySet->StartAnimation(L"dummy", animation3);

        canvas->StartAnimation(animation1);
        canvas->StartAnimation(animation2);

        LOG_OUTPUT(L"Verifying initial values");
        wfn_::float3 animatedTranslation = wh->GetAnimatedTranslation(canvas);
        LOG_OUTPUT(L"Initial AnimatedTranslation is {%f,%f,%f}", animatedTranslation.x, animatedTranslation.y, animatedTranslation.z);
        VERIFY_IS_TRUE(animatedTranslation == wfn_::float3(0, 0, 0));

        wfn_::float3 animatedScale = wh->GetAnimatedScale(canvas);
        LOG_OUTPUT(L"Initial AnimatedScale is {%f,%f,%f}", animatedScale.x, animatedScale.y, animatedScale.z);
        VERIFY_IS_TRUE(animatedScale == wfn_::float3(1, 1, 1));

    });
    wh->WaitForAnimatedFacadePropertyChanges(4);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying animated values");
        wfn_::float3 animatedTranslation = wh->GetAnimatedTranslation(canvas);
        LOG_OUTPUT(L"AnimatedTranslation is {%f,%f,%f}", animatedTranslation.x, animatedTranslation.y, animatedTranslation.z);
        VERIFY_IS_TRUE(animatedTranslation == wfn_::float3(100, 100, 0));

        wfn_::float3 animatedScale = wh->GetAnimatedScale(canvas);
        LOG_OUTPUT(L"AnimatedScale is {%f,%f,%f}", animatedScale.x, animatedScale.y, animatedScale.z);
        VERIFY_IS_TRUE(animatedScale == wfn_::float3(2, 2, 1));

        LOG_OUTPUT(L"Verifying TransformToVisual");
        auto transform = canvas->TransformToVisual(nullptr);
        wf::Point bottomRight = transform->TransformPoint(wf::Point(100, 100));
        VerifyPointWithEpsilon(bottomRight, wf::Point(300, 300), 0);

        propertySet->StopAnimation(L"dummy");
        canvas->StopAnimation(animation1);
        canvas->StopAnimation(animation2);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying final values");
        wfn_::float3 translation = canvas->Translation;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 100, 0));

        wfn_::float3 scale = canvas->Scale;
        LOG_OUTPUT(L"Scale is {%f,%f,%f}", scale.x, scale.y, scale.z);
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 2, 1));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::HitTestingTransformMatrix()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ root;
    Canvas^ canvas1;
    Canvas^ canvas2;
    Vector4KeyFrameAnimation^ animation;
    Vector4KeyFrameAnimation^ animation2;

    auto pointerPressedRegistration1 = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent1 = std::make_shared<Event>();

    auto pointerPressedRegistration2 = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent2 = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        root = ref new Canvas();

        canvas1 = ref new Canvas();
        canvas1->Width = 200;
        canvas1->Height = 200;
        canvas1->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Green);

        canvas2 = ref new Canvas();
        canvas2->Width = 200;
        canvas2->Height = 200;
        canvas2->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);

        root->Children->Append(canvas1);
        root->Children->Append(canvas2);

        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Attaching PointerPressed handlers");

        pointerPressedRegistration1.Attach(
            canvas1,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"PointerPressed event received on canvas1");
            pointerPressedEvent1->Set();
        }));

        pointerPressedRegistration2.Attach(
            canvas2,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"PointerPressed event received on canvas2");
            pointerPressedEvent2->Set();
        }));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting TransformMatrix with 3D:  RotationY of 60 degrees");
        wfn_::float4x4 matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        matrix.m11 = 0.5f;
        matrix.m13 = 0.866f;
        matrix.m31 = -0.866f;
        matrix.m33 = 0.5f;
        canvas2->TransformMatrix = matrix;

        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(canvas2, false);
        LOG_OUTPUT(L"Global Bounds: {%f,%f,%f,%f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 100, 200));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(50, 100));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    TestServices::InputHelper->Tap(wf::Point(150, 100));
    pointerPressedEvent1->WaitForDefault();
    pointerPressedEvent1->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting TransformMatrix back to 2D");
        wfn_::float4x4 matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        canvas2->TransformMatrix = matrix;

        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(canvas2, false);
        LOG_OUTPUT(L"Global Bounds: {%f,%f,%f,%f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 200, 200));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(50, 100));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    TestServices::InputHelper->Tap(wf::Point(150, 100));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateVector4KeyFrameAnimation();
        animation->InsertKeyFrame(0.0, wfn_::float4(0.5f, 0.866f, -0.866f, 0.5f));
        animation->InsertKeyFrame(1.0, wfn_::float4(0.5f, 0.866f, -0.866f, 0.5f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "TransformMatrix._11_13_31_33";

        canvas2->StartAnimation(animation);
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    TestServices::InputHelper->Tap(wf::Point(50, 100));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    TestServices::InputHelper->Tap(wf::Point(150, 100));
    pointerPressedEvent1->WaitForDefault();
    pointerPressedEvent1->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(canvas2, false);
        LOG_OUTPUT(L"Global Bounds: {%f,%f,%f,%f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 100, 200));

        LOG_OUTPUT(L"Starting HandOff animation back to 2D");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation2 = compositor->CreateVector4KeyFrameAnimation();
        animation2->InsertKeyFrame(0.0, wfn_::float4(1.0f, 0.0f, 0.0f, 1.0f));
        animation2->InsertKeyFrame(1.0, wfn_::float4(1.0f, 0.0f, 0.0f, 1.0f));
        ::Windows::Foundation::TimeSpan span = {1000000L};    // 100 ms
        animation2->Duration = span;
        animation2->Target = "TransformMatrix._11_13_31_33";

        canvas2->StartAnimation(animation2);
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(50, 100));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    TestServices::InputHelper->Tap(wf::Point(150, 100));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    RunOnUIThread([&]()
    {
        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(canvas2, false);
        LOG_OUTPUT(L"Global Bounds: {%f,%f,%f,%f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(0, 0, 200, 200));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting TransformMatrix to 2D translate");
        wfn_::float4x4 matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 50, 150, 0, 1};
        canvas2->TransformMatrix = matrix;

        wf::Rect globalBounds = wh->TestGetGlobalBoundsForUIElement(canvas2, false);
        LOG_OUTPUT(L"Global Bounds: {%f,%f,%f,%f}", globalBounds.X, globalBounds.Y, globalBounds.Width, globalBounds.Height);
        VERIFY_IS_TRUE(globalBounds == wf::Rect(50, 150, 200, 200));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(100, 250));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();

    TestServices::InputHelper->Tap(wf::Point(200, 250));
    pointerPressedEvent2->WaitForDefault();
    pointerPressedEvent2->Reset();
}

void UIElementFacadeTests::HitTestingLargeMove()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    xaml_shapes::Rectangle^ rect;
    ExpressionAnimation^ animation;
    Compositor^ compositor;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        canvas = ref new Canvas();

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
        compositor = CompositionTarget::GetCompositorForCurrentThread();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Attaching PointerPressed handler");

        pointerPressedRegistration.Attach(
            rect,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"PointerPressed event received");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(50, 50));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting animation");
        animation = compositor->CreateExpressionAnimation(L"Vector3(100,100,0)");
        animation->Target = L"Translation";
        rect->StartAnimation(animation);
    });
    wh->WaitForAnimatedFacadePropertyChanges(2);

    TestServices::InputHelper->Tap(wf::Point(150, 150));
    pointerPressedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::HitTesting3D()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    xaml_shapes::Rectangle^ rect;
    ExpressionAnimation^ animation;
    ExpressionAnimation^ animation2;
    ExpressionAnimation^ animation3;
    Compositor^ compositor;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating XAML tree.");

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 200;
        rect->Height = 100;

        canvas = ref new Canvas();
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        canvas->Width = 200;
        canvas->Height = 100;
        canvas->Children->Append(rect);

        wh->WindowContent = canvas;
        compositor = CompositionTarget::GetCompositorForCurrentThread();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching PointerPressed handler.");

        pointerPressedRegistration.Attach(
            rect,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"> PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(150, 50));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    LOG_OUTPUT(L"> Translation");
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Static value.");
            rect->Translation = { 200, 200, 10 };
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(350, 250));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Starting animation.");
            animation = compositor->CreateExpressionAnimation(L"Vector3(100,100,20)");
            animation->Target = L"Translation";
            rect->StartAnimation(animation);
        });
        wh->WaitForAnimatedFacadePropertyChanges(2);

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(250, 150));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Stopping animation.");
            rect->StopAnimation(animation);
        });
        wh->WaitForIdle();
    }

    LOG_OUTPUT(L"> Rotation");
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Set axis.");
            rect->RotationAxis = {1, 2, 3};

            LOG_OUTPUT(L"  > Static value.");
            rect->Rotation = -30;
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(270, 40));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Set axis.");
            rect->RotationAxis = {1, 2, 3};

            LOG_OUTPUT(L"  > Starting animation.");
            animation = compositor->CreateExpressionAnimation(L"30");
            animation->Target = L"Rotation";
            rect->StartAnimation(animation);
        });
        wh->WaitForAnimatedFacadePropertyChanges(2);

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(220, 250));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Stopping animation.");
            rect->StopAnimation(animation);
            rect->RotationAxis = { 0, 0, 1 };
            rect->Rotation = 0;
        });
        wh->WaitForIdle();
    }

    LOG_OUTPUT(L"> Rotation on parent + Translation");
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Set static RotationAxis & Rotation on parent.");
            canvas->RotationAxis = { 1, 0, 0 };
            canvas->Rotation = -60;

            LOG_OUTPUT(L"  > Set static Translation.");
            rect->Translation = { 0, 0, 200 };
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"  > Hit testing.");
        // 60 degrees is a 1/2/root(3) triangle. The Z=200 translation becomes a Y=173 translation
        TestServices::InputHelper->Tap(wf::Point(25, 173+25));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Starting animation.");
            animation = compositor->CreateExpressionAnimation(L"-60");
            animation->Target = L"Rotation";
            canvas->StartAnimation(animation);

            animation2 = compositor->CreateExpressionAnimation(L"Vector3(0,0,200)");
            animation2->Target = L"Translation";
            rect->StartAnimation(animation2);
        });
        wh->WaitForAnimatedFacadePropertyChanges(2);

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(25, 173+25));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Stopping animation.");

            canvas->RotationAxis = {0, 0, 1};
            canvas->StopAnimation(animation);
            canvas->RotationAxis = { 0, 0, 1 };
            canvas->Rotation = 0;

            rect->StopAnimation(animation2);
            rect->RotationAxis = { 0, 0, 1 };
            rect->Rotation = 0;
            rect->Translation = { 0, 0, 0 };
        });
        wh->WaitForIdle();
    }

    LOG_OUTPUT(L"> Rotation + Scale on parent + Translation");
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Set static RotationAxis, Rotation, & Scale on parent.");
            canvas->RotationAxis = { 1, 0, 0 };
            canvas->Rotation = -60;
            canvas->Scale = { 1, 1, 2 };

            LOG_OUTPUT(L"  > Set static Translation.");
            rect->Translation = { 0, 0, 200 };
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(25, 173*2+25));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Starting animation.");
            animation = compositor->CreateExpressionAnimation(L"-60");
            animation->Target = L"Rotation";
            canvas->StartAnimation(animation);

            animation2 = compositor->CreateExpressionAnimation(L"Vector3(1,1,2)");
            animation2->Target = L"Scale";
            canvas->StartAnimation(animation2);

            animation3 = compositor->CreateExpressionAnimation(L"Vector3(0,0,200)");
            animation3->Target = L"Translation";
            rect->StartAnimation(animation3);
        });
        wh->WaitForAnimatedFacadePropertyChanges(2);

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(25, 173*2+25));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Stopping animation.");

            canvas->RotationAxis = {0, 0, 1};
            canvas->StopAnimation(animation);
            canvas->StopAnimation(animation2);
            canvas->RotationAxis = { 0, 0, 1 };
            canvas->Rotation = 0;
            canvas->Scale = { 1, 1, 1 };

            rect->StopAnimation(animation3);
            rect->Translation = { 0, 0, 0 };
        });
        wh->WaitForIdle();
    }
}

void UIElementFacadeTests::HitTesting2DRotations()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    xaml_shapes::Rectangle^ rect;
    ExpressionAnimation^ animation;
    ExpressionAnimation^ animation2;
    Compositor^ compositor;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating XAML tree.");

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 200;
        rect->Height = 100;

        canvas = ref new Canvas();
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        canvas->Width = 200;
        canvas->Height = 100;
        canvas->Children->Append(rect);

        wh->WindowContent = canvas;
        compositor = CompositionTarget::GetCompositorForCurrentThread();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching PointerPressed handler.");

        pointerPressedRegistration.Attach(
            rect,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"> PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(150, 50));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    LOG_OUTPUT(L"> 2D Rotation");
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Static value.");
            rect->Rotation = 30;
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(120, 160));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Starting animation.");
            animation = compositor->CreateExpressionAnimation(L"30");
            animation->Target = L"Rotation";
            rect->StartAnimation(animation);
        });
        wh->WaitForAnimatedFacadePropertyChanges(2);

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(120, 160));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Stopping animation.");
            rect->StopAnimation(animation);
            rect->Rotation = 0;
        });
        wh->WaitForIdle();
    }

    LOG_OUTPUT(L"> 2D Rotation, negative Z axis");
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Static value.");
            rect->RotationAxis = { 0, 0, -1 };
            rect->Rotation = -30;
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(120, 160));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"  > Starting animation.");
            animation = compositor->CreateExpressionAnimation(L"-30");
            animation->Target = L"Rotation";
            rect->StartAnimation(animation);

            animation2 = compositor->CreateExpressionAnimation(L"Vector3(0,0,-1)");
            animation2->Target = L"RotationAxis";
            rect->StartAnimation(animation2);
        });
        wh->WaitForAnimatedFacadePropertyChanges(2);

        LOG_OUTPUT(L"  > Hit testing.");
        TestServices::InputHelper->Tap(wf::Point(120, 160));
        pointerPressedEvent->WaitForDefault();
        pointerPressedEvent->Reset();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Stopping animation.");
            rect->StopAnimation(animation);
            rect->StopAnimation(animation2);
            rect->RotationAxis = { 0, 0, 1 };
            rect->Rotation = 0;
        });
        wh->WaitForIdle();
    }
}

void UIElementFacadeTests::HitTestingTranslationCombos()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    xaml_shapes::Rectangle^ rect;
    Visual^ handOffVisual;
    Compositor^ compositor;

    auto pointerPressedRegistration = CreateSafeEventRegistration(xaml::UIElement, PointerPressed);
    std::shared_ptr<Event> pointerPressedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating XAML tree.");

        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 50;
        rect->Height = 50;

        canvas = ref new Canvas();
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        canvas->Width = 400;
        canvas->Height = 400;
        canvas->Children->Append(rect);

        wh->WindowContent = canvas;
        compositor = CompositionTarget::GetCompositorForCurrentThread();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching PointerPressed handler.");

        pointerPressedRegistration.Attach(
            rect,
            ref new xaml_input::PointerEventHandler(
            [&](Platform::Object^, xaml_input::PointerRoutedEventArgs^ args)
        {
            LOG_OUTPUT(L"> PointerPressed event received.");
            pointerPressedEvent->Set();
        }));
    });
    wh->WaitForIdle();

    TestServices::InputHelper->Tap(wf::Point(25, 25));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Translation facade");
        rect->Translation = {50, 50, 0};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    TestServices::InputHelper->Tap(wf::Point(75, 75));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting RenderTransform");
        auto renderTransform = ref new TranslateTransform();
        renderTransform->X = 50;
        renderTransform->Y = 50;
        rect->RenderTransform = renderTransform;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    TestServices::InputHelper->Tap(wf::Point(125, 125));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Clearing RenderTransform");
        rect->RenderTransform = nullptr;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Transform3D");
        auto transform3D = ref new CompositeTransform3D();
        transform3D->TranslateX = 50;
        transform3D->TranslateY = 50;
        transform3D->TranslateZ = 50;
        rect->Transform3D = transform3D;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    TestServices::InputHelper->Tap(wf::Point(125, 125));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding 3D to Translation facade");
        rect->Translation = {50, 50, 50};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");

    TestServices::InputHelper->Tap(wf::Point(125, 125));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Clearing 3D");
        rect->Transform3D = nullptr;
        rect->Translation = {50, 50, 0};
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting ECP.Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(rect, true);
        handOffVisual = ElementCompositionPreview::GetElementVisual(rect);
        handOffVisual->Properties->InsertVector3(L"Translation", ::Windows::Foundation::Numerics::float3(50.0f, 50.0f, 0.0f));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"5");

    TestServices::InputHelper->Tap(wf::Point(125, 125));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting HandOff visual Scale");
        handOffVisual->Scale = {2, 2, 1};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"6");

    TestServices::InputHelper->Tap(wf::Point(175, 175));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding 3D to Translation facade");
        rect->Translation = {50, 50, 50};
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"7");

    TestServices::InputHelper->Tap(wf::Point(175, 175));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding 3D to HandOff visual");
        handOffVisual->Properties->InsertVector3(L"Translation", ::Windows::Foundation::Numerics::float3(50.0f, 50.0f, 50.0f));
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"8");

    TestServices::InputHelper->Tap(wf::Point(175, 175));
    pointerPressedEvent->WaitForDefault();
    pointerPressedEvent->Reset();
}

void UIElementFacadeTests::StrictChecks()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        canvas = ref new Canvas();

        LOG_OUTPUT(L"Verify facade APIs are banned if element is in NonStrict mode");
        canvas->RenderTransform = ref new TranslateTransform();

        LOG_OUTPUT(L"Translation");
        ExpectSuccess([&]()
        {
            canvas->Translation = {1, 2, 3};
        });

        LOG_OUTPUT(L"Rotation");
        ExpectEAccessDenied([&]()
        {
            canvas->Rotation = 45;
        });

        LOG_OUTPUT(L"Scale");
        ExpectEAccessDenied([&]()
        {
            canvas->Scale = {1, 2, 3};
        });

        LOG_OUTPUT(L"TransformMatrix");
        ExpectEAccessDenied([&]()
        {
            canvas->TransformMatrix = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        });

        LOG_OUTPUT(L"CenterPoint");
        ExpectEAccessDenied([&]()
        {
            canvas->CenterPoint = {1, 2, 3};
        });

        LOG_OUTPUT(L"RotationAxis");
        ExpectEAccessDenied([&]()
        {
            canvas->RotationAxis = {1, 2, 3};
        });

        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        auto animation = compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(1.0, wfn_::float3(100.0f, 200.0f, 300.0f));
        ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
        animation->Duration = span;
        animation->Target = "Translation";
        animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

        LOG_OUTPUT(L"StartAnimation:  Translation");
        ExpectSuccess([&]()
        {
            canvas->StartAnimation(animation);
        });

        LOG_OUTPUT(L"StopAnimation:  Translation");
        ExpectSuccess([&]()
        {
            canvas->StopAnimation(animation);
        });

        animation->Target = "Scale";

        LOG_OUTPUT(L"StartAnimation:  Scale");
        ExpectEAccessDenied([&]()
        {
            canvas->StartAnimation(animation);
        });

        LOG_OUTPUT(L"Verifying legacy properties are banned if element is in Strict mode");
        auto canvas2 = ref new Canvas();
        canvas2->Rotation = 45;

        LOG_OUTPUT(L"RenderTransform");
        ExpectEAccessDenied([&]()
        {
            canvas2->RenderTransform = ref new TranslateTransform();
        });

        LOG_OUTPUT(L"RenderTransformOrigin");
        ExpectEAccessDenied([&]()
        {
            canvas2->RenderTransformOrigin = wf::Point(1, 2);
        });

        LOG_OUTPUT(L"Projection");
        ExpectEAccessDenied([&]()
        {
            canvas2->Projection = ref new PlaneProjection();
        });

        LOG_OUTPUT(L"Transform3D");
        ExpectEAccessDenied([&]()
        {
            canvas2->Transform3D = ref new PerspectiveTransform3D();
        });

        LOG_OUTPUT(L"ECP.GetElementVisual");
        ExpectEAccessDenied([&]()
        {
            auto visual = ElementCompositionPreview::GetElementVisual(canvas2);
        });

        LOG_OUTPUT(L"ECP.SetIsTranslationEnabled");
        ExpectEAccessDenied([&]()
        {
            ElementCompositionPreview::SetIsTranslationEnabled(canvas2, true);
        });


        LOG_OUTPUT(L"Verify turning on Strict is banned if ECP APIs are in use");

        Canvas^ canvas3 = ref new Canvas();
        LOG_OUTPUT(L"HandOff Visual");
        auto visual = ElementCompositionPreview::GetElementVisual(canvas3);
        ExpectEAccessDenied([&]()
        {
            canvas3->Rotation = 45;
        });

        Canvas^ canvas4 = ref new Canvas();
        LOG_OUTPUT(L"ECP Translation");
        ElementCompositionPreview::SetIsTranslationEnabled(canvas4, true);
        ExpectEAccessDenied([&]()
        {
            canvas4->Rotation = 45;
        });

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::TranslationTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Vector3Transition^ transition;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(0, 0, 0));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting transition");
        transition = ref new Vector3Transition();
        transition->Duration = { 1000L * 10000L };   // 1 seconds
        canvas->TranslationTransition = transition;

        LOG_OUTPUT(L"> Casting Vector3Transition to IInspectable.");
        VerifyCannotQIToDO(reinterpret_cast<IInspectable*>(transition));

        LOG_OUTPUT(L"> Setting new Translation. It should be applied immediately.");
        canvas->Translation = {100, 200, 300};
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 200, 300));
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Translation should still be applied while transition is in progress.");
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 200, 300));
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Translation should still be applied after transition has ended.");
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 200, 300));

        LOG_OUTPUT(L"> Swizzle the transition. Setting to 2D Translate.");
        transition->Components = Vector3TransitionComponents::X;
        transition->Duration = { 100000L * 10000L };   // 100 seconds
        canvas->Translation = {200, 100, 0};
        translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(200, 100, 0));
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Translation should still be applied while transition is in progress.");
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(200, 100, 0));
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"swizzle");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Interrupt transition with another transition. We should hand off correctly.");
        transition->Components = Vector3TransitionComponents::X | Vector3TransitionComponents::Y;
        canvas->Translation = {100, 0, 0};
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 0));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation.");
        canvas->Translation = {100, 0, 0};
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 0));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation, even if the transition property was nulled out.");
        canvas->TranslationTransition = nullptr;
        canvas->Translation = {100, 0, 0};
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 0));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    wh->SynchronouslyTickUIThread(4);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> The interrupted value should still be applied after the previous animation has handed off to the new one.");
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 0));

        LOG_OUTPUT(L"> Interrupt the transition again by disabling transitions and setting a static value. We should hard cut to the final value.");
        canvas->Translation = {300, 200, 100};
        translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(300, 200, 100));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Disable all channels on the transition. Setting a static value should hard cut to the final value.");
        canvas->TranslationTransition = transition;
        transition->Components = static_cast<Vector3TransitionComponents>(0);
        canvas->Translation = {200, 200, 200};
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(200, 200, 200));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not kick off any animations.");
        transition->Components = Vector3TransitionComponents::X | Vector3TransitionComponents::Y | Vector3TransitionComponents::Z;
        canvas->Translation = {200, 200, 200};
        wfn_::float3 translation = canvas->Translation;
        VERIFY_IS_TRUE(translation == wfn_::float3(200, 200, 200));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static2");
}

void UIElementFacadeTests::ScaleTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    Vector3Transition^ transition;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1, 1, 1));

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting transition");
        transition = ref new Vector3Transition();
        transition->Duration = { 1000L * 10000L };
        canvas->ScaleTransition = transition;

        LOG_OUTPUT(L"> Casting Vector3Transition to IInspectable.");
        VerifyCannotQIToDO(reinterpret_cast<IInspectable*>(transition));

        LOG_OUTPUT(L"> Setting new Scale. It should be applied immediately.");
        canvas->Scale = {1.5, 1.5, 0.5};
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 1.5, 0.5));
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Scale should still be applied while transition is in progress.");
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 1.5, 0.5));
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Scale should still be applied after transition has ended.");
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 1.5, 0.5));

        LOG_OUTPUT(L"> Swizzle the transition. Setting to 2D Scale.");
        transition->Components = Vector3TransitionComponents::X;
        canvas->Scale = {2, 1, 0};
        scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 1, 0));
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Scale should still be applied while transition is in progress.");
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 1, 0));
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"swizzle");

    wh->SynchronouslyTickUIThread(4);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> The interrupted value should still be applied after the previous animation has handed off to the new one.");
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 1, 0));

        LOG_OUTPUT(L"> Interrupt transition with another transition. We should hand off correctly.");
        transition->Components = Vector3TransitionComponents::X | Vector3TransitionComponents::Y;
        canvas->Scale = {1.5, 2, 1};
        scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 2, 1));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation.");
        canvas->Scale = {1.5, 2, 1};
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 2, 1));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation, even if the transition property was nulled out.");
        canvas->ScaleTransition = nullptr;
        canvas->Scale = {1.5, 2, 1};
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 2, 1));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> The interrupted value should still be applied after the previous animation has handed off to the new one.");
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(1.5, 2, 1));

        LOG_OUTPUT(L"> Interrupt the transition again by disabling transitions and setting a static value. We should hard cut to the final value.");
        canvas->Scale = {3, 2, 1};
        scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(3, 2, 1));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Disable all channels on the transition. Setting a static value should hard cut to the final value.");
        canvas->ScaleTransition = transition;
        transition->Components = static_cast<Vector3TransitionComponents>(0);
        canvas->Scale = {2, 2, 2};
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 2, 2));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static2");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not kick off any animations.");
        transition->Components = Vector3TransitionComponents::X | Vector3TransitionComponents::Y | Vector3TransitionComponents::Z;
        canvas->Scale = {2, 2, 2};
        wfn_::float3 scale = canvas->Scale;
        VERIFY_IS_TRUE(scale == wfn_::float3(2, 2, 2));
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static2");
}

void UIElementFacadeTests::RotationTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    ScalarTransition^ transition;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 0);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting transition");
        transition = ref new ScalarTransition();
        transition->Duration = { 1000L * 10000L };
        canvas->RotationTransition = transition;

        LOG_OUTPUT(L"> Casting ScalarTransition to IInspectable.");
        VerifyCannotQIToDO(reinterpret_cast<IInspectable*>(transition));

        LOG_OUTPUT(L"> Setting new Rotation. It should be applied immediately.");
        canvas->Rotation = 45;
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 45);
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Rotation should still be applied while transition is in progress.");
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 45);
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Rotation should still be applied after transition has ended.");
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 45);

        LOG_OUTPUT(L"> Set new Rotation.");
        canvas->Rotation = 60;
        rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 60);
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Rotation should still be applied while transition is in progress.");
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 60);

        LOG_OUTPUT(L"> Interrupt transition with another transition. We should hand off correctly.");
        canvas->Rotation = -30;
        rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == -30);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation.");
        canvas->Rotation = -30;
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == -30);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation, even if the transition property was nulled out.");
        canvas->RotationTransition = nullptr;
        canvas->Rotation = -30;
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == -30);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    wh->SynchronouslyTickUIThread(4);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> The interrupted value should still be applied after the previous animation has handed off to the new one.");
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == -30);

        LOG_OUTPUT(L"> Interrupt the transition again by disabling transitions and setting a static value. We should hard cut to the final value.");
        canvas->Rotation = 15;
        rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 15);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not kick off any animations.");
        canvas->RotationTransition = transition;
        canvas->Rotation = 15;
        float rotation = canvas->Rotation;
        VERIFY_IS_TRUE(rotation == 15);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static");
}

void UIElementFacadeTests::OpacityTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    ScalarTransition^ transition;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 1);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting transition");
        transition = ref new ScalarTransition();
        transition->Duration = { 1000L * 10000L };
        canvas->OpacityTransition = transition;

        LOG_OUTPUT(L"> Casting ScalarTransition to IInspectable.");
        VerifyCannotQIToDO(reinterpret_cast<IInspectable*>(transition));

        LOG_OUTPUT(L"> Setting new Opacity. It should be applied immediately.");
        canvas->Opacity = 0.5f;
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.5f);
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Opacity should still be applied while transition is in progress.");
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.5f);
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Opacity should still be applied after transition has ended.");
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.5f);

        LOG_OUTPUT(L"> Set new Opacity.");
        canvas->Opacity = 0.2f;
        opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.2f);
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> New Opacity should still be applied while transition is in progress.");
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.2f);

        LOG_OUTPUT(L"> Interrupt transition with another transition. We should hand off correctly.");
        canvas->Opacity = 0.75f;
        opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.75f);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation.");
        canvas->Opacity = 0.75f;
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.75f);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not stop the ongoing animation, even if the transition property was nulled out.");
        canvas->OpacityTransition = nullptr;
        canvas->Opacity = 0.75f;
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.75f);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"handoff");

    wh->SynchronouslyTickUIThread(4);
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> The interrupted value should still be applied after the previous animation has handed off to the new one.");
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.75f);

        LOG_OUTPUT(L"> Interrupt the transition again by disabling transitions and setting a static value. We should hard cut to the final value.");
        canvas->Opacity = 0.4f;
        opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.4f);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting the same value should not kick off any animations.");
        canvas->OpacityTransition = transition;
        canvas->Opacity = 0.4f;
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0.4f);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"static");
}

void UIElementFacadeTests::OpacityTransitionTo0()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    ScalarTransition^ transition;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating tree.");
        canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
        canvas->Opacity = 0;

        LOG_OUTPUT(L"> Setting transition.");
        transition = ref new ScalarTransition();
        transition->Duration = { 1000L * 10000L };
        canvas->OpacityTransition = transition;

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Opacity=1. The element should be rendered.");
        canvas->Opacity = 1;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"From0");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Opacity=0 with hand off. The element should still be rendered.");
        canvas->Opacity = 0;
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"To0");

    LOG_OUTPUT(L"> Waiting for the transition to finish. The element should be culled.");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Culled");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Opacity=1. The element should be rendered");
        canvas->Opacity = 1;
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"From0");

    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting Opacity=0. The element should still be rendered.");
        canvas->Opacity = 0;
        double opacity = canvas->Opacity;
        VERIFY_IS_TRUE(opacity == 0);
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"To0");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing OpacityTransition. The element should still be rendered.");
        canvas->OpacityTransition = nullptr;
        canvas->Height = 50;    // OpacityTransition itself doesn't trigger another render walk, so update the UIElement as well
        canvas->Height = 100;
    });
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"To0");

    LOG_OUTPUT(L"> Waiting for the transition to finish. The element should be culled afterwards.");
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Culled");
}

void UIElementFacadeTests::TransitionsFromMarkup()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Loading markup.");

        canvas = dynamic_cast<Canvas^>(xaml_markup::XamlReader::Load(
            L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Width='100' Height='100' > "
            L"  <Canvas.BackgroundTransition> "
            L"    <BrushTransition /> "
            L"  </Canvas.BackgroundTransition> "
            L"  <Canvas.OpacityTransition> "
            L"    <ScalarTransition Duration='0:0:0.0005' /> "
            L"  </Canvas.OpacityTransition> "
            L"  <Canvas.RotationTransition> "
            L"    <ScalarTransition Duration='35.0:0:1' /> "
            L"  </Canvas.RotationTransition> "
            L"  <Canvas.TranslationTransition> "
            L"    <Vector3Transition Duration='0:0:1.5' /> "
            L"  </Canvas.TranslationTransition> "
            L"  <Canvas.ScaleTransition> "
            L"    <Vector3Transition Duration='2:0:2' Components='Z,Y' /> "
            L"  </Canvas.ScaleTransition> "
            L"</Canvas>"));
        VERIFY_IS_NOT_NULL(canvas);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting new properties.");
        canvas->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);
        canvas->Opacity = 0.5f;
        canvas->Rotation = 45;
        canvas->Translation = {100, 200, 300};
        canvas->Scale = {4, 3, 2};
    });
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void UIElementFacadeTests::VerifyCannotQIToDO(IInspectable* inspectable)
{
    {
        LOG_OUTPUT(L"  > QI to DependencyObject. This should fail.");
        void* dependencyObject = nullptr;
        HRESULT castHR = inspectable->QueryInterface(__uuidof(DependencyObject^), &dependencyObject);
        VERIFY_ARE_EQUAL(E_NOINTERFACE, castHR);
        VERIFY_IS_NULL(dependencyObject);
    }

    {
        LOG_OUTPUT(L"  > QI to IDependencyObject. This should fail.");
        void* dependencyObject = nullptr;
        HRESULT castHR = inspectable->QueryInterface(__uuidof(IDependencyObject^), &dependencyObject);
        VERIFY_ARE_EQUAL(E_NOINTERFACE, castHR);
        VERIFY_IS_NULL(dependencyObject);
    }
}

void UIElementFacadeTests::ThisDotTarget()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    xaml_shapes::Rectangle^ rect;
    ExpressionAnimation^ animation;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        Canvas^ canvas = ref new Canvas();
        rect = ref new xaml_shapes::Rectangle();
        rect->Fill = ref new xaml_media::SolidColorBrush(mu::Colors::Purple);
        rect->Width = 100;
        rect->Height = 100;
        rect->Translation = {0, 0, 100};

        canvas->Children->Append(rect);
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Animation");
        auto compositor = CompositionTarget::GetCompositorForCurrentThread();
        animation = compositor->CreateExpressionAnimation(L"this.target.Translation.Z");
        animation->Target = "Translation.X";

        rect->StartAnimation(animation);
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Stopping Animation");
        rect->StopAnimation(animation);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wfn_::float3 translation = rect->Translation;
        LOG_OUTPUT(L"Translation is {%f,%f,%f}", translation.x, translation.y, translation.z);
        VERIFY_IS_TRUE(translation == wfn_::float3(100, 0, 100));
    });
    wh->WaitForIdle();
}

void UIElementFacadeTests::PropertiesFromMarkup()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    Canvas^ canvas;
    RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Loading markup.");

            canvas = dynamic_cast<Canvas^>(xaml_markup::XamlReader::Load(
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Width='100' Height='100' Rotation='45' Scale='1,2,3' Translation='-0.5,2e3,2.5' /> "));
            VERIFY_IS_NOT_NULL(canvas);

            wh->WindowContent = canvas;

            LOG_OUTPUT(L"> Loaded markup.");
        });
    wh->WaitForIdle();
    wh->SynchronouslyTickUIThread(2);
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}
