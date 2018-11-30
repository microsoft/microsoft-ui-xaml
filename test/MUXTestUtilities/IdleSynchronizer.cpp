// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "IdleSynchronizer.h"
#include "Event.h"

#include <strsafe.h>

using namespace MUXTestUtilities;

using namespace MUXControls::Common;

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;

using namespace concurrency;

Handle OpenNamedEvent(DWORD processId, DWORD threadId, const wchar_t* eventNamePrefix)
{
    wchar_t eventName[_MAX_PATH];
    StringCchPrintfW(eventName, _countof(eventName), L"%s.%d.%d", eventNamePrefix, processId, threadId);
    Handle handle = Handle(::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false /* inherit handle */, eventName));
    if (!handle.IsValid())
    {
        // Warning: Opening a session wide event handle, test may listen for events coming from the wrong process
        handle = Handle(::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false /* inherit handle */, eventNamePrefix));
    }

    if (!handle.IsValid())
    {
        WCHAR message[512];
        StringCchPrintfW(message, _countof(message), L"Failed to open %s handle.", eventNamePrefix);
        throw ref new Platform::FailureException(ref new Platform::String(message));
    }

    return handle;
}

inline Handle OpenNamedEvent(DWORD threadId, const wchar_t* eventNamePrefix)
{
    return OpenNamedEvent(::GetCurrentProcessId(), threadId, eventNamePrefix);
}

DWORD GetUIThreadId(CoreDispatcher^ dispatcher)
{
    DWORD threadId = 0;
    if (dispatcher->HasThreadAccess)
    {
        threadId = ::GetCurrentThreadId();
    }
    else
    {
        Event runCompleted;
        dispatcher->RunAsync(
            CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([&runCompleted,&threadId]()
        {
            threadId = ::GetCurrentThreadId();
            runCompleted.Set();
        }));
        runCompleted.Wait();
    }

    return threadId;
}

inline Handle OpenNamedEvent(CoreDispatcher^ dispatcher, const wchar_t* eventNamePrefix)
{
    return OpenNamedEvent(::GetCurrentProcessId(), GetUIThreadId(dispatcher), eventNamePrefix);
}

IdleSynchronizer::IdleSynchronizer(CoreDispatcher^ dispatcher)
    : m_coreDispatcher(dispatcher)
    , m_hasAnimationsHandle(OpenNamedEvent(m_coreDispatcher, s_hasAnimationsHandleName))
    , m_animationsCompleteHandle(OpenNamedEvent(m_coreDispatcher, s_animationsCompleteHandleName))
    , m_hasDeferredAnimationOperationsHandle(OpenNamedEvent(m_coreDispatcher, s_hasDeferredAnimationOperationsHandleName))
    , m_deferredAnimationOperationsCompleteHandle(OpenNamedEvent(m_coreDispatcher, s_deferredAnimationOperationsCompleteHandleName))
    , m_rootVisualResetHandle(OpenNamedEvent(m_coreDispatcher, s_rootVisualResetHandleName))
    , m_imageDecodingIdleHandle(OpenNamedEvent(m_coreDispatcher, s_imageDecodingIdleHandleName))
    , m_fontDownloadsIdleHandle(OpenNamedEvent(m_coreDispatcher, s_fontDownloadsIdleHandleName))
    , m_hasBuildTreeWorksHandle(OpenNamedEvent(m_coreDispatcher, s_hasBuildTreeWorksHandleName))
    , m_buildTreeServiceDrainedHandle(OpenNamedEvent(m_coreDispatcher, s_buildTreeServiceDrainedHandleName))
{
}

void IdleSynchronizer::Wait(Platform::String^* logMessage)
{
    static IdleSynchronizer^ instance = nullptr;
    if (instance == nullptr)
    {
        instance = ref new IdleSynchronizer(CoreApplication::MainView->Dispatcher);
    }

    Platform::String^ errorString = instance->WaitInternal(logMessage);

    if (errorString->Length())
    {
        throw ref new Platform::FailureException(errorString);
    }
}

Platform::String^ IdleSynchronizer::TryWait(Platform::String^* logMessage)
{
    static IdleSynchronizer^ instance = nullptr;
    if (instance == nullptr)
    {
        instance = ref new IdleSynchronizer(CoreApplication::MainView->Dispatcher);
    }
    return instance->WaitInternal(logMessage);
}

#define ReturnIfFailed(errorString) { if (errorString->Length() > 0) { return errorString; } }

void AddLog(IdleSynchronizer^ pThis, PCWSTR formatMessage, ...) // Not a member because C++/CX types can't have ellipsis parameters
{
    va_list args;
    va_start(args, formatMessage);

    WCHAR szOutput[512];

    if (pThis->Log != nullptr && pThis->Log != L"LOG: ")
    {
        pThis->Log += "; ";
    }

    StringCchPrintfW(szOutput, _countof(szOutput), L"%d: ", (GetTickCount64() - pThis->TickCountBegin));

    pThis->Log += ref new Platform::String(szOutput);

    StringCchVPrintfW(szOutput, _countof(szOutput), formatMessage, args);

    pThis->Log += ref new Platform::String(szOutput);

    va_end(args);
}

Platform::String^ IdleSynchronizer::WaitInternal(Platform::String^* logMessage)
{
    if (m_coreDispatcher->HasThreadAccess)
    {
        return L"Cannot wait for UI thread idle from the UI thread.";
    }

    Log = L"LOG: ";
    TickCountBegin = GetTickCount64();

    bool isIdle = false;
    while (!isIdle)
    {
        bool hadAnimations = true;
        bool hadDeferredAnimationOperations = true;
        bool hadBuildTreeWork = false;

        ReturnIfFailed(WaitForRootVisualReset());
        AddLog(this, L"After WaitForRootVisualReset");

        ReturnIfFailed(WaitForImageDecodingIdle());

        AddLog(this, L"After WaitForImageDecodingIdle");

        SynchronouslyTickUIThread(1);

        AddLog(this, L"After SynchronouslyTickUIThread(1)");
        ReturnIfFailed(WaitForFontDownloadsIdle());

        AddLog(this, L"After WaitForFontDownloadsIdle");

        WaitForIdleDispatcher();

        AddLog(this, L"After WaitForIdleDispatcher");

        // At this point, we know that the UI thread is idle - now we need to make sure
        // that XAML isn't animating anything.
        ReturnIfFailed(WaitForBuildTreeServiceWork(&hadBuildTreeWork));

        AddLog(this, L"After WaitForBuildTreeServiceWork");

        // The AnimationsComplete handle sometimes is never set in RS1,
        // so we'll skip waiting for animations to complete
        // if we've timed out once while waiting for animations in RS1.
        if (!m_waitForAnimationsIsDisabled)
        {
            ReturnIfFailed(WaitForAnimationsComplete(&hadAnimations));
            AddLog(this, L"After WaitForAnimationsComplete");
        }
        else
        {
            hadAnimations = false;
        }

        ReturnIfFailed(WaitForDeferredAnimationOperationsComplete(&hadDeferredAnimationOperations));
        AddLog(this, L"After WaitForDeferredAnimationOperationsComplete");

        // In the case where we waited for an animation to complete there's a possibility that
        // XAML, at the completion of the animation, scheduled a new tick. We will loop
        // for as long as needed until we complete an idle dispatcher callback without
        // waiting for a pending animation to complete.
        isIdle = !hadAnimations && !hadDeferredAnimationOperations && !hadBuildTreeWork;

        AddLog(this, L"IsIdle? %d", isIdle);
    }

    AddLog(this, L"End");

    if (logMessage)
    {
        *logMessage = Log;
    }

    return L"";
}

Platform::String^ IdleSynchronizer::WaitForRootVisualReset()
{
    DWORD waitResult = ::WaitForSingleObject(m_rootVisualResetHandle, 5000);

    if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
    {
        return L"Waiting for root visual reset handle returned an invalid value.";
    }

    return L"";
}

Platform::String^ IdleSynchronizer::WaitForImageDecodingIdle()
{
    DWORD waitResult = ::WaitForSingleObject(m_imageDecodingIdleHandle, 5000);

    if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
    {
        return L"Waiting for image decoding idle handle returned an invalid value.";
    }

    return L"";
}

Platform::String^ IdleSynchronizer::WaitForFontDownloadsIdle()
{
    DWORD waitResult = ::WaitForSingleObject(m_fontDownloadsIdleHandle, 5000);

    if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
    {
        return L"Waiting for font downloads handle returned an invalid value.";
    }

    return L"";
}

void IdleSynchronizer::WaitForIdleDispatcher()
{
    bool isDispatcherIdle = false;
    Event shouldContinueEvent;

    while (!isDispatcherIdle)
    {
        IAsyncAction^ action = m_coreDispatcher->RunIdleAsync(ref new IdleDispatchedHandler([&](IdleDispatchedHandlerArgs^ args)
        {
            isDispatcherIdle = args->IsDispatcherIdle;
        }));

        action->Completed = ref new AsyncActionCompletedHandler([&](IAsyncAction^, AsyncStatus)
        {
            shouldContinueEvent.Set();
        });

        shouldContinueEvent.WaitFor(10000);
    }
}

Platform::String^ IdleSynchronizer::WaitForBuildTreeServiceWork(bool* pHadBuildTreeWork)
{
    bool hasBuildTreeWork = true;

    // We want to avoid an infinite loop, so we'll iterate 20 times before concluding that
    // we probably are never going to become idle.
    int waitCount = 20;

    while (hasBuildTreeWork && waitCount-- > 0)
    {
        if (!::ResetEvent(m_buildTreeServiceDrainedHandle))
        {
            return L"Failed to reset BuildTreeServiceDrained handle.";
        }

        Event layoutUpdatedEvent;

        m_coreDispatcher->RunAsync(
            CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([&layoutUpdatedEvent]()
        {
            if (Window::Current != nullptr && Window::Current->Content != nullptr)
            {
                Window::Current->Content->UpdateLayout();
            }

            layoutUpdatedEvent.Set();
        }));

        layoutUpdatedEvent.Wait();

        // This will be signaled if and only if Jupiter plans to at some point in the near
        // future set the BuildTreeServiceDrained event.
        DWORD waitResult = ::WaitForSingleObject(m_hasBuildTreeWorksHandle, 0);

        if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
        {
            return L"HasBuildTreeWork handle wait returned an invalid value.";
        }

        hasBuildTreeWork = (waitResult == WAIT_OBJECT_0);

        if (hasBuildTreeWork)
        {
            DWORD waitResult = ::WaitForSingleObject(m_buildTreeServiceDrainedHandle, 10000);

            if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
            {
                return L"Wait for build tree service failed";
            }
        }
    }

    *pHadBuildTreeWork = hasBuildTreeWork;
    return L"";
}

Platform::String^ IdleSynchronizer::WaitForAnimationsComplete(bool* hadAnimations)
{
    if (!::ResetEvent(m_animationsCompleteHandle))
    {
        return L"Failed to reset AnimationsComplete handle.";
    }

    AddLog(this, L"WaitForAnimationsComplete: After ResetEvent");


    // This will be signaled if and only if XAML plans to at some point in the near
    // future set the animations complete event.
    DWORD waitResult = ::WaitForSingleObject(m_hasAnimationsHandle, 0);

    if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
    {
        return L"HasAnimations handle wait returned an invalid value.";
    }

    AddLog(this, L"WaitForAnimationsComplete: After Wait(m_hasAnimationsHandle)");

    bool hasAnimations = (waitResult == WAIT_OBJECT_0);

    if (hasAnimations)
    {
        DWORD animationCompleteWaitResult = ::WaitForSingleObject(m_animationsCompleteHandle, s_idleTimeoutMs);

        AddLog(this, L"WaitForAnimationsComplete: HasAnimations, After Wait(m_animationsCompleteHandle)");

        if (animationCompleteWaitResult != WAIT_OBJECT_0)
        {
            if (!IsRS2OrHigher())
            {
                // The AnimationsComplete handle is sometimes just never signaled on RS1, ever.
                // If we run into this problem, we'll just disable waiting for animations to complete
                // and continue execution.  When the current test completes, we'll then close and reopen
                // the test app to minimize the effects of this problem.
                m_waitForAnimationsIsDisabled = true;

                *hadAnimations = false;
            }

            return L"Animation complete wait took longer than idle timeout.";
        }
    }

    *hadAnimations = hasAnimations;
    return L"";
}

Platform::String^ IdleSynchronizer::WaitForDeferredAnimationOperationsComplete(bool* pHadDeferredAnimationOperations)
{
    if (!::ResetEvent(m_deferredAnimationOperationsCompleteHandle))
    {
        return L"Failed to reset DeferredAnimationOperations handle.";
    }

    // This will be signaled if and only if XAML plans to at some point in the near
    // future set the animations complete event.
    DWORD waitResult = ::WaitForSingleObject(m_hasDeferredAnimationOperationsHandle, 0);

    if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
    {
        return L"HasDeferredAnimationOperations handle wait returned an invalid value.";
    }

    bool hasDeferredAnimationOperations = (waitResult == WAIT_OBJECT_0);

    if (hasDeferredAnimationOperations)
    {
        DWORD animationCompleteWaitResult = ::WaitForSingleObject(m_deferredAnimationOperationsCompleteHandle, s_idleTimeoutMs);

        if (animationCompleteWaitResult != WAIT_OBJECT_0 && animationCompleteWaitResult != WAIT_TIMEOUT)
        {
            return L"Deferred animation operations complete wait took longer than idle timeout.";
        }
    }

    *pHadDeferredAnimationOperations = hasDeferredAnimationOperations;
    return L"";
}

void IdleSynchronizer::SynchronouslyTickUIThread(unsigned int ticks)
{
    EventRegistrationToken renderingToken = {};
    Event tickCompleteEvent;

    for (unsigned int i = 0; i < ticks; i++)
    {
        tickCompleteEvent.Reset();

        m_coreDispatcher->RunAsync(
            CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([&tickCompleteEvent, &renderingToken]()
        {
            renderingToken = CompositionTarget::Rendering += ref new EventHandler<Platform::Object^>(
                [&tickCompleteEvent, &renderingToken](Platform::Object^, Platform::Object^)
            {
                CompositionTarget::Rendering -= renderingToken;
                tickCompleteEvent.Set();
            });
        }));

        tickCompleteEvent.Wait();
    }
}

bool IdleSynchronizer::IsRS2OrHigher()
{
    if (!m_isRS2OrHigherInitialized)
    {
        m_isRS2OrHigherInitialized = true;
        m_isRS2OrHigher = Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 4);
    }

    return m_isRS2OrHigher;
}
