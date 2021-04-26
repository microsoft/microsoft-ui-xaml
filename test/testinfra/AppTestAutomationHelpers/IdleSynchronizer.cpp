#include "pch.h"
#include "IdleSynchronizer.h"
#include "IdleSynchronizer.g.cpp"
#include "Event.h"

#include <strsafe.h>
#include <hstring.h>
#include <functional>

using namespace AppTestAutomationHelpers;

namespace
{
    Handle OpenNamedEvent(DWORD processId, DWORD threadId, const wchar_t* eventNamePrefix)
    {
        wchar_t eventName[_MAX_PATH];
        StringCchPrintfW(eventName, _countof(eventName), L"%s.%d.%d", eventNamePrefix, processId, threadId);
        Handle handle = Handle(::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false /* inherit handle */, eventName));
        if (!handle.IsValid())
        {
            handle = Handle(::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false /* inherit handle */, eventNamePrefix));
        }

        if (!handle.IsValid())
        {
            WCHAR message[512];
            StringCchPrintfW(message, _countof(message), L"Failed to open %s handle.", eventNamePrefix);
            throw winrt::hresult_error(E_FAIL, winrt::hstring(message));
        }

        return handle;
    }

    inline Handle OpenNamedEvent(DWORD threadId, const wchar_t* eventNamePrefix)
    {
        return OpenNamedEvent(::GetCurrentProcessId(), threadId, eventNamePrefix);
    }

    DWORD GetUIThreadId(winrt::Windows::UI::Core::CoreDispatcher dispatcher)
    {
        DWORD threadId = 0;
        if (dispatcher.HasThreadAccess())
        {
            threadId = ::GetCurrentThreadId();
        }
        else
        {
            Event runCompleted;
            auto asyncOp = dispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
                winrt::Windows::UI::Core::DispatchedHandler([&runCompleted, &threadId]()
                    {
                        threadId = ::GetCurrentThreadId();
                        runCompleted.Set();
                    }));

            runCompleted.Wait();
        }

        return threadId;
    }

    inline Handle OpenNamedEvent(winrt::Windows::UI::Core::CoreDispatcher dispatcher, const wchar_t* eventNamePrefix)
    {
        return OpenNamedEvent(::GetCurrentProcessId(), GetUIThreadId(dispatcher), eventNamePrefix);
    }
}

namespace winrt::AppTestAutomationHelpers::implementation
{
    IdleSynchronizer::IdleSynchronizer(winrt::Windows::UI::Core::CoreDispatcher dispatcher)
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

    winrt::hstring IdleSynchronizer::TryWait()
    {
        return WaitInternal();
    }

#define ReturnIfFailed(errorString) { if (!errorString.empty()) { return errorString; } }

    void AddLog(IdleSynchronizer* pThis, PCWSTR formatMessage, ...)
    {
        va_list args;
        va_start(args, formatMessage);

        WCHAR szOutput[512];

        if (!(pThis->Log.empty()) && pThis->Log != L"LOG: ")
        {
            pThis->Log = pThis->Log + L"; ";
        }

        StringCchPrintfW(szOutput, _countof(szOutput), L"%d: ", (GetTickCount64() - pThis->TickCountBegin));

        pThis->Log = pThis->Log + szOutput;

        StringCchVPrintfW(szOutput, _countof(szOutput), formatMessage, args);

        pThis->Log = pThis->Log + szOutput;

        va_end(args);
    }

    winrt::hstring IdleSynchronizer::WaitInternal()
    {
        if (m_coreDispatcher.HasThreadAccess())
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

            ReturnIfFailed(WaitForAnimationsComplete(&hadAnimations));
            AddLog(this, L"After WaitForAnimationsComplete");

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

        return L"";
    }

    winrt::hstring IdleSynchronizer::WaitForRootVisualReset()
    {
        DWORD waitResult = ::WaitForSingleObject(m_rootVisualResetHandle, 5000);

        if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
        {
            return L"Waiting for root visual reset handle returned an invalid value.";
        }

        return L"";
    }

    winrt::hstring IdleSynchronizer::WaitForImageDecodingIdle()
    {
        DWORD waitResult = ::WaitForSingleObject(m_imageDecodingIdleHandle, 5000);

        if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
        {
            return L"Waiting for image decoding idle handle returned an invalid value.";
        }

        return L"";
    }

    winrt::hstring IdleSynchronizer::WaitForFontDownloadsIdle()
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
        std::shared_ptr<bool> isDispatcherIdle = std::make_shared<bool>();
        std::shared_ptr<Event> shouldContinueEvent = std::make_shared<Event>();

        while (!isDispatcherIdle)
        {
            winrt::Windows::Foundation::IAsyncAction action = m_coreDispatcher.RunIdleAsync(winrt::Windows::UI::Core::IdleDispatchedHandler([isDispatcherIdle, shouldContinueEvent](winrt::Windows::UI::Core::IdleDispatchedHandlerArgs args)
                {
                    *isDispatcherIdle = args.IsDispatcherIdle();
                    shouldContinueEvent->Set();
                }));

            action.Completed([shouldContinueEvent](auto& /*asyncInfo*/, auto& /*asyncStatus*/)
                {
                    shouldContinueEvent->Set();
                });

            shouldContinueEvent->WaitFor(10000);
        }
    }

    winrt::hstring IdleSynchronizer::WaitForBuildTreeServiceWork(bool* pHadBuildTreeWork)
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

            auto asyncOp = m_coreDispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
                winrt::Windows::UI::Core::DispatchedHandler([&layoutUpdatedEvent]()
                    {
                        if (auto window = winrt::Windows::UI::Xaml::Window::Current())
                        {
                            if (auto content = window.Content())
                            {
                                content.UpdateLayout();
                            }
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
                waitResult = ::WaitForSingleObject(m_buildTreeServiceDrainedHandle, 10000);

                if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
                {
                    return L"Wait for build tree service failed";
                }
            }
        }

        *pHadBuildTreeWork = hasBuildTreeWork;
        return L"";
    }

    winrt::hstring IdleSynchronizer::WaitForAnimationsComplete(bool* hadAnimations)
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
                return L"Animation complete wait took longer than idle timeout.";
            }
        }

        *hadAnimations = hasAnimations;
        return L"";
    }

    winrt::hstring IdleSynchronizer::WaitForDeferredAnimationOperationsComplete(bool* pHadDeferredAnimationOperations)
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
        Event tickCompleteEvent;

        for (unsigned int i = 0; i < ticks; i++)
        {
            winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering_revoker renderingRevoker{};
            tickCompleteEvent.Reset();

            auto asyncOp = m_coreDispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
                winrt::Windows::UI::Core::DispatchedHandler([&tickCompleteEvent, &renderingRevoker]()
                    {
                        renderingRevoker = winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering(winrt::auto_revoke,
                            [&tickCompleteEvent, &renderingRevoker](auto&, auto&)
                            {
                                renderingRevoker.revoke();
                                tickCompleteEvent.Set();
                            }
                        );
                    }));

            tickCompleteEvent.Wait();
        }
    }

}
