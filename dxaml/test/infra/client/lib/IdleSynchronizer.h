// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Handle.h>
#include <TestEvent.h>
#include "TestServices.h"

namespace Private { namespace Infrastructure {

    class TickWithNoContentScopeGuard
    {
    public:
        explicit TickWithNoContentScopeGuard();
        ~TickWithNoContentScopeGuard();

        // Disallow copying/moving
        TickWithNoContentScopeGuard(const TickWithNoContentScopeGuard&) = delete;
        TickWithNoContentScopeGuard(TickWithNoContentScopeGuard&&) = delete;
        TickWithNoContentScopeGuard& operator=(const TickWithNoContentScopeGuard&) = delete;
        TickWithNoContentScopeGuard& operator=(TickWithNoContentScopeGuard&&) = delete;
    };

    class IdleSynchronizer
    {
    public:
        IdleSynchronizer(DWORD uiThreadId, test_infra::ITestServicesStatics* pTestServices, test_infra::IWindowHelper* pWindowHelper);

        // WaitForIdle is actually a pretty complicated thing to define. It's easy
        // to conceptualize and describe what you want, "I want to wait until all the
        // UI is on the screen", but tricky to be percise about that. Here's the
        // procedure we use:
        // 0) Try and wait on the RootVisualReset event. There are two ticks that
        //    occur when setting Window::Content. First the RootScrollViewer is created and
        //    laid out, and second, after a WM_APPLICATION_STARTUP_EVENT_COMPLETE is posted
        //    to the queue, the content of the RootScrollViewer is set, which triggers yet
        //    another tick to occur. The RootVisualReset event is reset for the duration
        //    between setting Window::Content and when the public root visual is actually
        //    set as the content of the RootScrollViewer. Waiting for this event ensures we
        //    are only ever one tick away from having the entire frame realized. This is one
        //    of the only places in Jupiter where setting a property requires more than a
        //    single tick to properly lay out.
        // 1) Subscribe and wait for a per-frame callback: We do this first because
        //    there's a situation where the render frame is waiting for a VBLANK and
        //    won't kick the UI thread immediately after we've updated a property. By
        //    subscribing and waiting for the per-frame callback we ensure the UI thread
        //    will have ticked once.
        // 2) Wait for the DispatcherQueue queue to go empty. This actually isn't as
        //    important as you might think it is. A single UI thread tick will perform
        //    as many layout cycles as needed. The only reason we really want to wait
        //    for the DispatcherQueue queue to go empty is to handle things like pending
        //    input message / DManip updates.
        // 3) Wait for any compositor-driven animations to complete. This is key to doing
        //    things like waiting for transitions and flicks to complete.
        //
        // We perform this loop until it executes in such a way that no animations
        // are pending to be completed. We do this because animation completion events
        // often schedule additional work on the UI thread that we are interested in
        // seeing run to completion as well.
        //
        // In the future some ways we can improve this function:
        // - We can wait for the UI thread to be 'kicked' instead of kicking it ourselves.
        //   This would allow us to wait for an action to occur, and wait for the consequences
        //   of that action to be complete.
        // - We can become aware of the phased work queue and respect it. Right now the way
        //   that work will be rescheduled makes it likely this method will return before all
        //   the phased work is complete.
        // - As we move away from the composition thread we'll need to find a way to be
        //   aware of DComp animations (if we're performing visual validation). This will
        //   likely become an ETW waiting event.
        // - Moving the other parts of this to ETW. There's no reason really to use a named
        //   event. I'm not sure if there's a strong advantage here.
        // - Become aware of things like ongoing manipulations. As we move away from the
        //   composition thread we'll need to be aware of the manipulations themselves, not
        //   just the fact that jupiter is animating (because it won't be, they will be
        //   performed in DComp directly).
        //
        // Also in the future we may want an IdleSynchronizer per island, for multi-island scenarios.
        void WaitForIdle(msy::IDispatcherQueue* dispatcherQueue, bool waitForBuildTreeWork = true);

        // Resets the event that is being used for the WaitForPopupMenuCommandInvoked method below.
        void PrepareForPopupMenuWait();

        // Wait for a text control context menu to be discarded because one of its commands was invoked.
        // Returns True when a command is invoked within the provided 'timeout' time period. Returns False
        // when the 'timeout' ellapses and no command was invoked.
        bool WaitForPopupMenuCommandInvoked(std::chrono::milliseconds timeout);

        void WaitForImplicitShowHideComplete();

        void WaitForAnimatedFacadePropertyChangesComplete();

        void SynchronouslyTickUIThread(unsigned int ticks);

        void WaitForRootVisualReset();

        void WaitForIdleDispatcher(msy::IDispatcherQueue* dispatcherQueue);

    private:
        void WaitForAnimationsComplete(bool* pHadAnimations);
        void WaitForFacadeAnimationsComplete(bool* hadFacadeAnimations);
        void WaitForDeferredAnimationOperationsComplete(bool* pHadDeferredAnimationOperations);
        void WaitForBrushTransitionsComplete(bool* hadBrushTransitions);
        void WaitForLayoutClean();
        void WaitForImageDecodingIdle();
        void WaitForFontDownloadsIdle();
        void WaitForBuildTreeServiceWork(bool* pHadBuildTreeWork);

        IdleSynchronizer(const IdleSynchronizer&);
        IdleSynchronizer& operator=(const IdleSynchronizer&);

        static const wchar_t* s_hasAnimationsHandleName;
        static const wchar_t* s_animationsCompleteHandleName;
        static const wchar_t* s_hasDeferredAnimationOperationsHandleName;
        static const wchar_t* s_deferredAnimationOperationsCompleteHandleName;
        static const wchar_t* s_popupMenuCommandInvokedHandleName;
        static const wchar_t* s_rootVisualResetHandleName;
        static const wchar_t* s_layoutCleanHandleName;
        static const wchar_t* s_imageDecodingIdleHandleName;
        static const wchar_t* s_fontDownloadsIdleHandleName;
        static const wchar_t* s_hasBuildTreeWorksHandleName;
        static const wchar_t* s_BuildTreeServiceDrainedHandleName;
        static const wchar_t* s_implicitShowHideCompleteHandleName;
        static const wchar_t* s_hasFacadeAnimationsHandleName;
        static const wchar_t* s_facadeAnimationsCompleteHandleName;
        static const wchar_t* s_AnimatedFacadePropertyChangesCompleteHandleName;
        static const wchar_t* s_hasBrushTransitionsHandleName;
        static const wchar_t* s_brushTransitionsCompleteHandleName;

        // These handles are opened by Jupiter when it initializes
        // to track whether there are pending animations running and when
        // a root visual reset occurs.
        Microsoft::UI::Xaml::Tests::Common::Handle m_hasAnimationsHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_animationsCompleteHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_hasDeferredAnimationOperationsHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_deferredAnimationOperationsCompleteHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_popupMenuCommandInvokedHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_rootVisualResetHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_layoutCleanHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_imageDecodingIdleHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_fontDownloadsIdleHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_hasBuildTreeWorksHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_BuildTreeServiceDrainedHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_implicitShowHideCompleteHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_hasFacadeAnimationsHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_facadeAnimationsCompleteHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_AnimatedFacadePropertyChangesCompleteHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_hasBrushTransitionsHandle;
        Microsoft::UI::Xaml::Tests::Common::Handle m_brushTransitionsCompleteHandle;
        test_infra::ITestServicesStatics* m_pTestServices = nullptr;
        test_infra::IWindowHelper* m_pWindowHelper = nullptr;
    };

} }
