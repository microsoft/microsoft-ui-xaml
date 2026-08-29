// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "IdleSynchronizer.h"
#include <XamlTailored.h>
#include "IXamlTestHooks-win.h"
#include "Utilities.h"
#include "WindowHelper.h"
#include "Hosting.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace std::chrono_literals;

namespace Private { namespace Infrastructure {

    TickWithNoContentScopeGuard::TickWithNoContentScopeGuard()
    {
        RunOnUIThread([]() { WindowHelper::GetTestHooks()->SetCanTickWithNoContent(true); });
    }

    TickWithNoContentScopeGuard::~TickWithNoContentScopeGuard()
    {
        RunOnUIThread([]() { WindowHelper::GetTestHooks()->SetCanTickWithNoContent(false); });
    }

    const wchar_t* IdleSynchronizer::s_hasAnimationsHandleName = L"HasAnimations";
    const wchar_t* IdleSynchronizer::s_animationsCompleteHandleName = L"AnimationsComplete";
    const wchar_t* IdleSynchronizer::s_hasDeferredAnimationOperationsHandleName = L"HasDeferredAnimationOperations";
    const wchar_t* IdleSynchronizer::s_deferredAnimationOperationsCompleteHandleName = L"DeferredAnimationOperationsComplete";
    const wchar_t* IdleSynchronizer::s_popupMenuCommandInvokedHandleName = L"PopupMenuCommandInvoked";
    const wchar_t* IdleSynchronizer::s_rootVisualResetHandleName = L"RootVisualReset";
    const wchar_t* IdleSynchronizer::s_layoutCleanHandleName = L"LayoutClean";
    const wchar_t* IdleSynchronizer::s_imageDecodingIdleHandleName = L"ImageDecodingIdle";
    const wchar_t* IdleSynchronizer::s_fontDownloadsIdleHandleName = L"FontDownloadsIdle";
    const wchar_t* IdleSynchronizer::s_hasBuildTreeWorksHandleName = L"HasBuildTreeWorks";
    const wchar_t* IdleSynchronizer::s_BuildTreeServiceDrainedHandleName = L"BuildTreeServiceDrained";
    const wchar_t* IdleSynchronizer::s_implicitShowHideCompleteHandleName = L"ImplicitShowHideComplete";
    const wchar_t* IdleSynchronizer::s_hasFacadeAnimationsHandleName = L"HasFacadeAnimations";
    const wchar_t* IdleSynchronizer::s_facadeAnimationsCompleteHandleName = L"FacadeAnimationsComplete";
    const wchar_t* IdleSynchronizer::s_AnimatedFacadePropertyChangesCompleteHandleName = L"AnimatedFacadePropertyChangesComplete";
    const wchar_t* IdleSynchronizer::s_hasBrushTransitionsHandleName = L"HasBrushTransitions";
    const wchar_t* IdleSynchronizer::s_brushTransitionsCompleteHandleName = L"BrushTransitionsComplete";

    IdleSynchronizer::IdleSynchronizer(DWORD uiThreadId, test_infra::ITestServicesStatics* pTestServices, test_infra::IWindowHelper* pWindowHelper)
        : m_hasAnimationsHandle(OpenNamedEvent(uiThreadId, s_hasAnimationsHandleName))
        , m_animationsCompleteHandle(OpenNamedEvent(uiThreadId, s_animationsCompleteHandleName))
        , m_hasDeferredAnimationOperationsHandle(OpenNamedEvent(uiThreadId, s_hasDeferredAnimationOperationsHandleName))
        , m_deferredAnimationOperationsCompleteHandle(OpenNamedEvent(uiThreadId, s_deferredAnimationOperationsCompleteHandleName))
        , m_popupMenuCommandInvokedHandle(OpenNamedEvent(uiThreadId, s_popupMenuCommandInvokedHandleName))
        , m_rootVisualResetHandle(OpenNamedEvent(uiThreadId, s_rootVisualResetHandleName))
        , m_layoutCleanHandle(OpenNamedEvent(uiThreadId, s_layoutCleanHandleName))
        , m_imageDecodingIdleHandle(OpenNamedEvent(uiThreadId, s_imageDecodingIdleHandleName))
        , m_fontDownloadsIdleHandle(OpenNamedEvent(uiThreadId, s_fontDownloadsIdleHandleName))
        , m_hasBuildTreeWorksHandle(OpenNamedEvent(uiThreadId, s_hasBuildTreeWorksHandleName))
        , m_BuildTreeServiceDrainedHandle(OpenNamedEvent(uiThreadId, s_BuildTreeServiceDrainedHandleName))
        , m_implicitShowHideCompleteHandle(OpenNamedEvent(uiThreadId, s_implicitShowHideCompleteHandleName))
        , m_hasFacadeAnimationsHandle(OpenNamedEvent(uiThreadId, s_hasFacadeAnimationsHandleName))
        , m_facadeAnimationsCompleteHandle(OpenNamedEvent(uiThreadId, s_facadeAnimationsCompleteHandleName))
        , m_AnimatedFacadePropertyChangesCompleteHandle(OpenNamedEvent(uiThreadId, s_AnimatedFacadePropertyChangesCompleteHandleName))
        , m_hasBrushTransitionsHandle(OpenNamedEvent(uiThreadId, s_hasBrushTransitionsHandleName))
        , m_brushTransitionsCompleteHandle(OpenNamedEvent(uiThreadId, s_brushTransitionsCompleteHandleName))
        , m_pTestServices(pTestServices)
        , m_pWindowHelper(pWindowHelper)
    {
        WEX::Common::Throw::LastErrorIf(!m_hasAnimationsHandle.IsValid(), L"Failed to create HasAnimations handle.");
        WEX::Common::Throw::LastErrorIf(!m_animationsCompleteHandle.IsValid(), L"Failed to create AnimationsComplete handle.");
        WEX::Common::Throw::LastErrorIf(!m_hasDeferredAnimationOperationsHandle.IsValid(), L"Failed to create HasDeferredAnimationOperations handle.");
        WEX::Common::Throw::LastErrorIf(!m_deferredAnimationOperationsCompleteHandle.IsValid(), L"Failed to create DeferredAnimationOperationsComplete handle.");
        WEX::Common::Throw::LastErrorIf(!m_popupMenuCommandInvokedHandle.IsValid(), L"Failed to create PopupMenuCommandInvoked handle.");
        WEX::Common::Throw::LastErrorIf(!m_rootVisualResetHandle.IsValid(), L"Failed to create RootVisualReset handle.");
        WEX::Common::Throw::LastErrorIf(!m_layoutCleanHandle.IsValid(), L"Failed to create LayoutClean handle.");
        WEX::Common::Throw::LastErrorIf(!m_imageDecodingIdleHandle.IsValid(), L"Failed to create ImageDecodingIdle handle.");
        WEX::Common::Throw::LastErrorIf(!m_fontDownloadsIdleHandle.IsValid(), L"Failed to create FontDownloadsIdle handle.");
        WEX::Common::Throw::LastErrorIf(!m_hasBuildTreeWorksHandle.IsValid(), L"Failed to create HasBuildTreeWorks handle.");
        WEX::Common::Throw::LastErrorIf(!m_BuildTreeServiceDrainedHandle.IsValid(), L"Failed to create BuildTreeServiceDrainedHandle handle.");
        WEX::Common::Throw::LastErrorIf(!m_implicitShowHideCompleteHandle.IsValid(), L"Failed to create ImplicitShowHideComplete handle.");
        WEX::Common::Throw::LastErrorIf(!m_hasFacadeAnimationsHandle.IsValid(), L"Failed to create HasFacadeAnimations handle.");
        WEX::Common::Throw::LastErrorIf(!m_facadeAnimationsCompleteHandle.IsValid(), L"Failed to create FacadeAnimationsComplete handle.");
        WEX::Common::Throw::LastErrorIf(!m_AnimatedFacadePropertyChangesCompleteHandle.IsValid(), L"Failed to create AnimatedFacadePropertyChangesComplete handle.");
        WEX::Common::Throw::LastErrorIf(!m_hasBrushTransitionsHandle.IsValid(), L"Failed to create HasBrushTransitions handle.");
        WEX::Common::Throw::LastErrorIf(!m_brushTransitionsCompleteHandle.IsValid(), L"Failed to create BrushTransitionsComplete handle.");
    }

    void IdleSynchronizer::WaitForIdle(msy::IDispatcherQueue* dispatcherQueue, bool waitForBuildTreeWork /*= true*/)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForIdle() started");

        WEX::Common::Throw::IfNull(dispatcherQueue, L"Dispatcher cannot be null");

        bool notIdle = true;
        while (notIdle)
        {
            bool hadAnimations = true;
            bool hadFacadeAnimations = false;
            bool hadBrushTransitions = false;
            bool hadDeferredAnimationOperations = true;
            bool hadBuildTreeWork = false;

            WaitForRootVisualReset();
            WaitForImageDecodingIdle();

            SynchronouslyTickUIThread(1);
            WaitForFontDownloadsIdle();

            // Even though we ticked a frame, there's no guarantee that the tree is clean for layout. After we finish
            // running layout in the frame, Image elements have an UpdateDirtyState pass that can dirty them again. If
            // this happens we need to wait until next frame to run layout again.
            WaitForLayoutClean();

            WaitForIdleDispatcher(dispatcherQueue);
            // At this point we're idle on the UI thread! Now lets make sure
            // Jupiter isn't animating anything.

            if (waitForBuildTreeWork)
            {
                WaitForBuildTreeServiceWork(&hadBuildTreeWork);
            }

            WaitForAnimationsComplete(&hadAnimations);

            WaitForFacadeAnimationsComplete(&hadFacadeAnimations);

            WaitForBrushTransitionsComplete(&hadBrushTransitions);

            Event commitComplete(L"CommitCompleted");
            RunOnUIThread([&]()
            {
                WindowHelper::GetTestHooks()->WaitForCommitCompletion();
                commitComplete.Set();
            });
            commitComplete.WaitFor(5min);

            WaitForDeferredAnimationOperationsComplete(&hadDeferredAnimationOperations);

            // In the case where we waited for an animation to complete there's a possibility that
            // Jupiter, at the completion of the animation, scheduled a new tick. We will loop
            // for as long as needed until we complete an idle dispatcher callback without
            // waiting for a pending animation to complete.

            notIdle = hadAnimations || hadFacadeAnimations || hadBrushTransitions || hadDeferredAnimationOperations || hadBuildTreeWork;

            BVT_OUTPUT(L"IdleSynchronizer::WaitForIdle() notIdle=%d", notIdle);
        }

        WEX::Common::String value_ignored;
        if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"GoSlow", value_ignored)))
        {
            ::Sleep(1000);
        }

        BVT_OUTPUT(L"IdleSynchronizer::WaitForIdle() ended");
    }

    void IdleSynchronizer::WaitForBuildTreeServiceWork(bool* pHadBuildTreeWork)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForBuildTreeServiceWork() started");

        bool hasBuildTreeWork = true;
        // rip cord to stop from going into an infinite loop
        int waitCount = 20;
        const DWORD timeOut = 10000;

        while (hasBuildTreeWork && waitCount-- > 0)
        {
            Throw::LastErrorIf(!::ResetEvent(m_BuildTreeServiceDrainedHandle));
            RunOnUIThread([&]()
            {
                wrl::ComPtr<xaml::IUIElement> spWindowContent;
                LogThrow_IfFailed(m_pWindowHelper->get_WindowContent(&spWindowContent));
                if (spWindowContent)
                {
                    spWindowContent->UpdateLayout();
                }
            });

            // This will be signaled if and only if Jupiter plans to at some point in the near
            // future set the BuildTreeServiceDrained event.
            DWORD waitResult = ::WaitForSingleObject(m_hasBuildTreeWorksHandle, 0);
            Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
                L"HasBuildTreeWork handle wait returned an invalid value.");
            hasBuildTreeWork = (waitResult == WAIT_OBJECT_0);

            if (hasBuildTreeWork)
            {
                LOG_OUTPUT(L"Waiting for BuildTreeService to finish...");
                waitResult = ::WaitForSingleObject(m_BuildTreeServiceDrainedHandle, timeOut);
                Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
                    L"Wait for build tree service failed");
                LOG_OUTPUT(L"BuildTreeService drained");
            }
        }

        if (hasBuildTreeWork)
        {
            LOG_WARNING(L"BuildTreeService did not complete");
        }

        *pHadBuildTreeWork = hasBuildTreeWork;
        BVT_OUTPUT(L"IdleSynchronizer::WaitForBuildTreeServiceWork() end, hasBuildTreeWork=%d", hasBuildTreeWork);
    }


    void IdleSynchronizer::WaitForIdleDispatcher(msy::IDispatcherQueue* dispatcherQueue)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForIdleDispatcher() started");

        boolean dispatcherIdle = false;

        while (!dispatcherIdle)
        {
            RunOnDispatcherThread(dispatcherQueue, [&]
            {
                dispatcherIdle = true;
            }, msy::DispatcherQueuePriority::DispatcherQueuePriority_Low, Event::GetDefaultTimeout());
        }
        BVT_OUTPUT(L"IdleSynchronizer::WaitForIdleDispatcher() ended");
    }

    void IdleSynchronizer::PrepareForPopupMenuWait()
    {
        Throw::LastErrorIf(!::ResetEvent(m_popupMenuCommandInvokedHandle));
    }

    bool IdleSynchronizer::WaitForPopupMenuCommandInvoked(std::chrono::milliseconds timeout)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForPopupMenuCommandInvoked()");
        // Return True when the event is set during the timeout period, and False in case of a timeout.
        return ::WaitForSingleObject(m_popupMenuCommandInvokedHandle, static_cast<long>(timeout.count())) == WAIT_OBJECT_0;
    }

    void IdleSynchronizer::WaitForRootVisualReset()
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForRootVisualReset()");
        DWORD waitResult = ::WaitForSingleObject(m_rootVisualResetHandle, static_cast<long>(Event::GetDefaultTimeout().count()));
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"Waiting for root visual reset handle returned an invalid value.");
    }

    void IdleSynchronizer::WaitForAnimationsComplete(bool* pHadAnimations)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForAnimationsComplete() started");
        Throw::LastErrorIf(!::ResetEvent(m_animationsCompleteHandle));

        // This will be signaled if and only if Jupiter plans to at some point in the near
        // future set the animations complete event.
        DWORD waitResult = ::WaitForSingleObject(m_hasAnimationsHandle, 0);
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"HasAnimations handle wait returned an invalid value.");
        bool hasAnimations = (waitResult == WAIT_OBJECT_0);

        if (hasAnimations)
        {
            DWORD animationCompleteWaitResult = ::WaitForSingleObject(m_animationsCompleteHandle, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
            const bool animationsCompleted = animationCompleteWaitResult == WAIT_OBJECT_0;
            Throw::IfFalse(animationsCompleted, E_FAIL,
                L"Animation complete wait took longer than idle timeout.");
        }

        *pHadAnimations = hasAnimations;
        BVT_OUTPUT(L"IdleSynchronizer::WaitForAnimationsComplete() ended, hasAnimations=%d", hasAnimations);
    }

    void IdleSynchronizer::WaitForFacadeAnimationsComplete(bool* hadFacadeAnimations)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForFacadeAnimationsComplete() started");

        // We handle facade animation idle slightly differently than the others.
        // m_hasFacadeAnimationsHandle will be signaled if a facade animation has ever been started since
        // the last time we did a WaitForIdle().  It is this function's responsibility to reset this event.
        // If m_hasFacadeAnimationsHandle is signaled we then wait for m_facadeAnimationsCompleteHandle.
        // This guarantees that if we ever start a facade animation, we'll detect this here and set the hadFacadeAnimations
        // flag to true, which will cause us to schedule one more tick after waiting for facade animations to complete.
        // This extra tick after animations complete is required for us to allow Jupiter to run the RenderWalk one more time
        // and update the state of the Visual tree.  Without this detection we could do a MockDComp dump before that final tick
        // and our Mock validation will fail randomly, depending on if we happened to do the extra tick yet or not.
        DWORD waitResult = ::WaitForSingleObject(m_hasFacadeAnimationsHandle, 0);
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"HasFacadeAnimations handle wait returned an invalid value.");
        bool hasFacadeAnimations = (waitResult == WAIT_OBJECT_0);

        if (hasFacadeAnimations)
        {
            auto scopeGuard = wil::scope_exit([&]
            {
                // Always reset the event, even in the case of timeout, in order to avoid blocking forever on any tests that run after this.
                ResetEvent(m_hasFacadeAnimationsHandle);
            });

            DWORD facadeAnimationCompleteWaitResult = ::WaitForSingleObject(m_facadeAnimationsCompleteHandle, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
            Throw::IfFalse(facadeAnimationCompleteWaitResult == WAIT_OBJECT_0, E_FAIL,
                L"Facade Animation complete wait took longer than idle timeout.");

        }

        *hadFacadeAnimations = hasFacadeAnimations;
        BVT_OUTPUT(L"IdleSynchronizer::WaitForFacadeAnimationsComplete() ended, hasFacadeAnimations=%d", hasFacadeAnimations);
    }

    void IdleSynchronizer::WaitForBrushTransitionsComplete(bool* hadBrushTransitions)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForBrushTransitionsComplete() started");

        // Brush transitions are handled like facade animations. If we ever start a brush transition, we'll detect this here
        // and set the hadBrushTransitions flag to true, which will cause us to schedule one more tick after waiting for brush
        // transitions to complete.
        // This extra tick after transitions complete is required for us to allow Jupiter to run the RenderWalk one more time
        // and update the state of the Visual tree.  Without this detection we could do a MockDComp dump before that final tick
        // and our Mock validation will fail randomly, depending on if we happened to do the extra tick yet or not.
        DWORD waitResult = ::WaitForSingleObject(m_hasBrushTransitionsHandle, 0);
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"HasBrushTransitions handle wait returned an invalid value.");
        bool hasBrushTransitions = (waitResult == WAIT_OBJECT_0);

        if (hasBrushTransitions)
        {
            auto scopeGuard = wil::scope_exit([&]
            {
                // Always reset the event, even in the case of timeout, in order to avoid blocking forever on any tests that run after this.
                ResetEvent(m_hasBrushTransitionsHandle);
            });

            DWORD brushTransitionCompleteWaitResult = ::WaitForSingleObject(m_brushTransitionsCompleteHandle, static_cast<long>(Event::GetDefaultTimeout().count()));
            Throw::IfFalse(brushTransitionCompleteWaitResult == WAIT_OBJECT_0, E_FAIL,
                L"Brush Transition complete wait took longer than idle timeout.");

        }

        *hadBrushTransitions = hasBrushTransitions;
        BVT_OUTPUT(L"IdleSynchronizer::WaitForBrushTransitionsComplete() ended, hasBrushTransitions=%d", hasBrushTransitions);
    }

    void IdleSynchronizer::WaitForDeferredAnimationOperationsComplete(bool* pHadDeferredAnimationOperations)
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForDeferredAnimationOperationsComplete() started");

        Throw::LastErrorIf(!::ResetEvent(m_deferredAnimationOperationsCompleteHandle));

        // This will be signaled if and only if Jupiter plans to at some point in the near
        // future set the animations complete event.
        DWORD waitResult = ::WaitForSingleObject(m_hasDeferredAnimationOperationsHandle, 0);
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"HasDeferredAnimationOperations handle wait returned an invalid value.");
        bool hasDeferredAnimationOperations = (waitResult == WAIT_OBJECT_0);

        if (hasDeferredAnimationOperations)
        {
            DWORD animationCompleteWaitResult = ::WaitForSingleObject(m_deferredAnimationOperationsCompleteHandle, static_cast<long>(Event::GetDefaultTimeout().count()));
            Throw::IfFalse(animationCompleteWaitResult == WAIT_OBJECT_0, E_FAIL,
                L"Deferred animation operations complete wait took longer than idle timeout.");
        }

        *pHadDeferredAnimationOperations = hasDeferredAnimationOperations;
        BVT_OUTPUT(L"IdleSynchronizer::WaitForDeferredAnimationOperationsComplete() ended, hasDeferredAnimationOperations=%d", hasDeferredAnimationOperations);
    }

    void IdleSynchronizer::WaitForLayoutClean()
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForLayoutClean()");

        DWORD waitResult = ::WaitForSingleObject(m_layoutCleanHandle, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"Waiting for layout clean handle returned an invalid value.");
    }

    void IdleSynchronizer::WaitForImageDecodingIdle()
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForImageDecodingIdle()");

        DWORD waitResult = ::WaitForSingleObject(m_imageDecodingIdleHandle, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"Waiting for image decoding idle handle returned an invalid value.");
    }

    void IdleSynchronizer::WaitForFontDownloadsIdle()
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForFontDownloadsIdle()");

        DWORD waitResult = ::WaitForSingleObject(m_fontDownloadsIdleHandle, 30000);
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"Waiting for font downloads handle returned an invalid value.");
    }

    void IdleSynchronizer::WaitForImplicitShowHideComplete()
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForImplicitShowHideComplete()");

        DWORD waitResult = ::WaitForSingleObject(m_implicitShowHideCompleteHandle, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"Waiting for ImplicitShowHideComplete returned an invalid value.");
    }

    void IdleSynchronizer::WaitForAnimatedFacadePropertyChangesComplete()
    {
        BVT_OUTPUT(L"IdleSynchronizer::WaitForAnimatedFacadePropertyChangesComplete()");

        DWORD waitResult = ::WaitForSingleObject(m_AnimatedFacadePropertyChangesCompleteHandle, static_cast<DWORD>(Event::GetDefaultTimeout().count()));
        Throw::IfFalse(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, E_UNEXPECTED,
            L"Waiting for AnimatedFacadePropertyChangesComplete returned an invalid value.");
    }

    void IdleSynchronizer::SynchronouslyTickUIThread(unsigned int ticks)
    {
        BVT_OUTPUT(L"IdleSynchronizer::SynchronouslyTickUIThread(%d) started", ticks);

        EventRegistrationToken renderingToken = {};
        Event tickCompleteEvent(L"TickCompleted");

        TickWithNoContentScopeGuard allowTickWithNoContent;

        for (unsigned int i = 0; i < ticks; i++)
        {
            tickCompleteEvent.Reset();

            RunOnUIThread([&tickCompleteEvent, &renderingToken] () {
                wrl::ComPtr<xaml::Media::ICompositionTargetStatics> spCompTargetStatics;
                LogThrow_IfFailed(wf::GetActivationFactory(
                    wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_CompositionTarget).Get(),
                    &spCompTargetStatics));

                LogThrow_IfFailed(spCompTargetStatics->add_Rendering(wrl::Callback<wf::IEventHandler<IInspectable*>>(
                    [spCompTargetStatics, &tickCompleteEvent, &renderingToken] (IInspectable*, IInspectable*) -> HRESULT
                    {
                        spCompTargetStatics->remove_Rendering(renderingToken);
                        tickCompleteEvent.Set();
                        return S_OK;
                    }).Get(),
                &renderingToken));
            });

            tickCompleteEvent.WaitFor(5min);
        }
    }
} }
