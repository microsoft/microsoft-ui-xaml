// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TimerTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ TimerTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool TimerTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();

    return true;
}

bool TimerTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool TimerTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void TimerTests::Tick()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"2Canvases.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    auto timerCompletedRegistration = CreateSafeEventRegistration(DispatcherTimer, Tick); auto timerCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        DispatcherTimer^ timer = ref new DispatcherTimer();
        ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L; timer->Interval = span;
        timerCompletedRegistration.Attach(timer, ref new wf::EventHandler<Object^>([timerCompletedEvent](Object^ sender, Object^ e) { timerCompletedEvent->Set(); }));
        timer->Start();
    });

    timerCompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(timerCompletedEvent->HasFired());
}

void TimerTests::TickWhileSuspended()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->SetIsSuspended(false);
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"2Canvases.xaml"));
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting WindowContent");
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto timerCompletedRegistration = CreateSafeEventRegistration(DispatcherTimer, Tick); auto timerCompletedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Starting Timer");
        DispatcherTimer^ timer = ref new DispatcherTimer();
        ::Windows::Foundation::TimeSpan span; span.Duration = 2500000L; timer->Interval = span;
        timerCompletedRegistration.Attach(timer, ref new wf::EventHandler<Object^>([timerCompletedEvent](Object^ sender, Object^ e)
        {
            LOG_OUTPUT(L"Timer fired");
            timerCompletedEvent->Set();
        }));
        timer->Start();

        TestServices::WindowHelper->SetIsSuspended(true);
    });

    LOG_OUTPUT(L"Waiting for timer");
    timerCompletedEvent->WaitForDefault();
    VERIFY_IS_TRUE(timerCompletedEvent->HasFired());
}

} } } } } }
