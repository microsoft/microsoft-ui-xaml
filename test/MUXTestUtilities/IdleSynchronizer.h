// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutoHandle.h"

namespace MUXTestUtilities
{
    public ref class IdleSynchronizer sealed
    {
    public:
        [Windows::Foundation::Metadata::DefaultOverload] 
        static void Wait(Platform::String^* logMessage);
        [Windows::Foundation::Metadata::DefaultOverload] 
        static Platform::String^ TryWait(Platform::String^* logMessage);

        static void Wait() { Wait(nullptr); }
        static Platform::String^ TryWait() { return TryWait(nullptr); }

        property ULONGLONG TickCountBegin;
        property Platform::String^ Log;

    private:
        IdleSynchronizer(Windows::UI::Core::CoreDispatcher^ coreDispatcher);

        Platform::String^ WaitInternal(Platform::String^* logMessage);

        Platform::String^ WaitForRootVisualReset();
        Platform::String^ WaitForImageDecodingIdle();
        Platform::String^ WaitForFontDownloadsIdle();
        void WaitForIdleDispatcher();
        Platform::String^ WaitForBuildTreeServiceWork(bool* hadBuildTreeWork);
        Platform::String^ WaitForAnimationsComplete(bool* hadAnimations);
        Platform::String^ WaitForDeferredAnimationOperationsComplete(bool* hadDeferredAnimationOperations);

        void SynchronouslyTickUIThread(unsigned int ticks);

    private:
        bool IsRS2OrHigher();

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

        Windows::UI::Core::CoreDispatcher^ m_coreDispatcher = nullptr;

        MUXControls::Common::Handle m_hasAnimationsHandle;
        MUXControls::Common::Handle m_animationsCompleteHandle;
        MUXControls::Common::Handle m_hasDeferredAnimationOperationsHandle;
        MUXControls::Common::Handle m_deferredAnimationOperationsCompleteHandle;
        MUXControls::Common::Handle m_rootVisualResetHandle;
        MUXControls::Common::Handle m_imageDecodingIdleHandle;
        MUXControls::Common::Handle m_fontDownloadsIdleHandle;
        MUXControls::Common::Handle m_hasBuildTreeWorksHandle;
        MUXControls::Common::Handle m_buildTreeServiceDrainedHandle;

        bool m_waitForAnimationsIsDisabled = false;
        bool m_isRS2OrHigherInitialized = false;
        bool m_isRS2OrHigher = false;
    };

} // namespace MUXTestUtilities

