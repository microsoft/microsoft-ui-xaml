#pragma once

#include "IdleSynchronizer.g.h"
#include "AutoHandle.h"

namespace winrt::AppTestAutomationHelpers::implementation
{
    struct IdleSynchronizer : IdleSynchronizerT<IdleSynchronizer>
    {
        IdleSynchronizer(winrt::Windows::System::DispatcherQueue dispatcherQueue);
        
        winrt::hstring TryWait();

        ULONGLONG TickCountBegin;
        winrt::hstring Log;

    private:

        winrt::hstring WaitInternal();

        winrt::hstring WaitForRootVisualReset();
        winrt::hstring WaitForImageDecodingIdle();
        winrt::hstring WaitForFontDownloadsIdle();
        void WaitForIdleDispatcher();
        winrt::hstring WaitForBuildTreeServiceWork(bool* hadBuildTreeWork);
        winrt::hstring WaitForAnimationsComplete(bool* hadAnimations);
        winrt::hstring WaitForDeferredAnimationOperationsComplete(bool* hadDeferredAnimationOperations);

        void SynchronouslyTickUIThread(unsigned int ticks);

    private:
        long s_idleTimeoutMs = 100000;
        const wchar_t* s_hasAnimationsHandleName = L"HasAnimations";
        const wchar_t* s_animationsCompleteHandleName = L"AnimationsComplete";
        const wchar_t* s_hasDeferredAnimationOperationsHandleName = L"HasDeferredAnimationOperations";
        const wchar_t* s_deferredAnimationOperationsCompleteHandleName = L"DeferredAnimationOperationsComplete";
        const wchar_t* s_rootVisualResetHandleName = L"RootVisualReset";
        const wchar_t* s_imageDecodingIdleHandleName = L"ImageDecodingIdle";
        const wchar_t* s_fontDownloadsIdleHandleName = L"FontDownloadsIdle";
        const wchar_t* s_hasBuildTreeWorksHandleName = L"HasBuildTreeWorks";
        const wchar_t* s_buildTreeServiceDrainedHandleName = L"BuildTreeServiceDrained";

        winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };

        ::AppTestAutomationHelpers::Handle m_hasAnimationsHandle;
        ::AppTestAutomationHelpers::Handle m_animationsCompleteHandle;
        ::AppTestAutomationHelpers::Handle m_hasDeferredAnimationOperationsHandle;
        ::AppTestAutomationHelpers::Handle m_deferredAnimationOperationsCompleteHandle;
        ::AppTestAutomationHelpers::Handle m_rootVisualResetHandle;
        ::AppTestAutomationHelpers::Handle m_imageDecodingIdleHandle;
        ::AppTestAutomationHelpers::Handle m_fontDownloadsIdleHandle;
        ::AppTestAutomationHelpers::Handle m_hasBuildTreeWorksHandle;
        ::AppTestAutomationHelpers::Handle m_buildTreeServiceDrainedHandle;
    };
}

namespace winrt::AppTestAutomationHelpers::factory_implementation
{
    struct IdleSynchronizer : IdleSynchronizerT<IdleSynchronizer, implementation::IdleSynchronizer>
    {
    };
}
