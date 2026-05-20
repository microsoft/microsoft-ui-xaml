// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include "VisualElementTests.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>
#include <DisableDCompLeakDetectionScopeGuard.h>
#include <Collection.h>
#include <WindowsNumerics.h>

using namespace Platform;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Composition::Interactions;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool VisualElementTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool VisualElementTests::ClassCleanup()
{
    return true;
}

bool VisualElementTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool VisualElementTests::TestCleanup()
{
    DisableDCompLeakDetectionScopeGuard disableLeakGuard;
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void VisualElementTests::BasicTest()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        canvas = ref new Canvas();
        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating VisualInteractionSource");
        auto vis = VisualInteractionSource::CreateFromIVisualElement(canvas);
        VERIFY_IS_NOT_NULL(vis);
    });
}

void VisualElementTests::InteractionTrackerTest()
{
    TestCleanupWrapper cleanup;

    auto wh = TestServices::WindowHelper;
    Canvas^ canvas;
    xaml_shapes::Rectangle^ rectangle;
    Compositor^ compositor;
    ExpressionAnimation^ expression;
    InteractionTracker^ tracker;
    CompositionScopedBatch^ scopedBatch;
    std::shared_ptr<Event> scopedBatchCompleteEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating XAML tree");
        canvas = ref new Canvas();
        canvas->Width = 200;
        canvas->Height = 200;

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 400;
        rectangle->Height = 400;
        rectangle->Fill = ref new SolidColorBrush(mu::Colors::Red);

        canvas->Children->Append(rectangle);

        wh->WindowContent = canvas;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting up InteractionTracker");
        compositor = CompositionTarget::GetCompositorForCurrentThread();
        tracker = InteractionTracker::Create(compositor);
        tracker->MinPosition = wfn_::float3(-10000, -10000, 0);
        tracker->MaxPosition = wfn_::float3(10000, 10000, 0);
        auto vis = VisualInteractionSource::CreateFromIVisualElement(canvas);
        tracker->InteractionSources->Add(vis);
        expression = compositor->CreateExpressionAnimation(L"-tracker.position");
        expression->SetReferenceParameter(L"tracker", tracker);
        expression->Target = L"Translation";

        scopedBatch = compositor->CreateScopedBatch(CompositionBatchTypes::AllAnimations);
        scopedBatch->Completed += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^,CompositionBatchCompletedEventArgs^> ([&](Platform::Object^, CompositionBatchCompletedEventArgs^)
        {
            LOG_OUTPUT(L"Scoped Batch complete");
            scopedBatchCompleteEvent->Set();
        });
        rectangle->StartAnimation(expression);
        scopedBatch->End();

        tracker->TryUpdatePosition(wfn_::float3(-200, -200, 0));
    });
    wh->SynchronouslyTickUIThread(3);

    RunOnUIThread([&]()
    {
        rectangle->StopAnimation(expression);
    });
    scopedBatchCompleteEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Translation is [%f,%f,%f]", rectangle->Translation.x, rectangle->Translation.y, rectangle->Translation.z);
        VERIFY_IS_TRUE(rectangle->Translation == wfn_::float3(200, 200, 0));
    });
    wh->WaitForIdle();
}

}}}}}}

