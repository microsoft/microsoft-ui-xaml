// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#ifndef XAMLPROFILER_ENABLED
#error "XamlLaunchTraceUnitTests.cpp requires XAMLPROFILER_ENABLED; keep its ClCompile entry conditioned on XamlProfilerEnabled."
#endif

#include "XamlLaunchTraceUnitTests.h"
#include <XamlLaunchPhase.h>
#include <thread>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    namespace
    {
        bool HasFlag(const XamlLaunchObservation& observation, XamlLaunchObservationFlags flag)
        {
            return (observation.flags & static_cast<uint32_t>(flag)) != 0;
        }

        void Draw(XamlLaunchTrace& trace)
        {
            const auto attempt = trace.BeginDrawAttempt();
            trace.BeginLayout(1, attempt);
            trace.BeginProduction(1, attempt);
            trace.EndDraw(1, attempt, true, S_OK);
        }
    }

    void XamlLaunchTraceUnitTests::LinearBoundaries()
    {
        XamlLaunchStartupScope startup;
        XamlLaunchTrace trace;
        const auto callback = trace.BeginOnLaunched();
        VERIFY_ARE_EQUAL(2u, callback.ordinal);
        VERIFY_IS_TRUE(callback.entryKind == XamlLaunchEntryKind::ApplicationStart);
        VERIFY_IS_TRUE(callback.startupId != 0 && callback.coreId != 0);
        VERIFY_ARE_NOT_EQUAL(callback.startupId, callback.coreId);
        VERIFY_ARE_EQUAL(3u, XamlLaunchTrace::EndOnLaunched(callback, &trace, S_OK).ordinal);

        const auto attempt = trace.BeginDrawAttempt();
        trace.BeginLayout(1, attempt);
        VERIFY_ARE_EQUAL(4u, trace.GetLastObservation().ordinal);
        trace.BeginProduction(1, attempt);
        VERIFY_ARE_EQUAL(5u, trace.GetLastObservation().ordinal);
        trace.EndDraw(1, attempt, true, S_OK);
        const auto completed = trace.GetLastObservation();
        VERIFY_ARE_EQUAL(6u, completed.ordinal);
        VERIFY_ARE_EQUAL(0u, completed.flags);
        VERIFY_ARE_EQUAL(callback.startupId, completed.startupId);
        VERIFY_ARE_EQUAL(callback.coreId, completed.coreId);
        VERIFY_ARE_EQUAL(attempt, completed.drawAttemptId);
    }

    void XamlLaunchTraceUnitTests::RenderingBeforeCallbackReturn()
    {
        for (const HRESULT result : { S_OK, E_FAIL })
        {
            XamlLaunchStartupScope startup;
            XamlLaunchTrace trace;
            const auto callback = trace.BeginOnLaunched();
            Draw(trace);
            VERIFY_ARE_EQUAL(6u, trace.GetLastObservation().ordinal);
            VERIFY_IS_TRUE(HasFlag(trace.GetLastObservation(), XamlLaunchObservationFlags::NonCanonicalOrder));

            const auto returned = XamlLaunchTrace::EndOnLaunched(callback, &trace, static_cast<uint32_t>(result));
            VERIFY_ARE_EQUAL(3u, returned.ordinal);
            VERIFY_IS_TRUE(HasFlag(returned, XamlLaunchObservationFlags::NonCanonicalOrder));
            VERIFY_ARE_EQUAL(FAILED(result), HasFlag(returned, XamlLaunchObservationFlags::CallbackFailed));
            VERIFY_ARE_EQUAL(uint64_t{0}, trace.BeginDrawAttempt());
        }
    }

    void XamlLaunchTraceUnitTests::CallbackFailureDoesNotStopFrameObservations()
    {
        XamlLaunchStartupScope startup;
        XamlLaunchTrace trace;
        const auto callback = trace.BeginOnLaunched();
        const auto returned = XamlLaunchTrace::EndOnLaunched(callback, &trace, static_cast<uint32_t>(E_FAIL));
        VERIFY_IS_TRUE(returned.nextPhase == XamlLaunchPhase::Aborted);
        VERIFY_ARE_EQUAL(static_cast<uint32_t>(E_FAIL), returned.result);
        Draw(trace);
        VERIFY_IS_TRUE(trace.GetLastObservation().nextPhase == XamlLaunchPhase::Complete);
        VERIFY_IS_TRUE(HasFlag(trace.GetLastObservation(), XamlLaunchObservationFlags::CallbackFailed));
    }

    void XamlLaunchTraceUnitTests::RetriesKeepBoundaryAndAttemptIdentity()
    {
        for (const bool productionInFirstAttempt : { false, true })
        {
            XamlLaunchStartupScope startup;
            XamlLaunchTrace trace;
            const auto callback = trace.BeginOnLaunched();
            XamlLaunchTrace::EndOnLaunched(callback, &trace, S_OK);
            const auto firstAttempt = trace.BeginDrawAttempt();
            trace.BeginLayout(1, firstAttempt);
            if (productionInFirstAttempt)
            {
                trace.BeginProduction(1, firstAttempt);
            }
            trace.EndDraw(1, firstAttempt, false, S_OK);
            const auto lastBoundary = trace.GetLastObservation();

            const auto secondAttempt = trace.BeginDrawAttempt();
            VERIFY_ARE_EQUAL(firstAttempt + 1, secondAttempt);
            trace.BeginLayout(1, secondAttempt);
            VERIFY_ARE_EQUAL(lastBoundary.ordinal, trace.GetLastObservation().ordinal);
            trace.BeginProduction(1, secondAttempt);
            VERIFY_ARE_EQUAL(5u, trace.GetLastObservation().ordinal);
            trace.EndDraw(1, secondAttempt, true, S_OK);
            VERIFY_ARE_EQUAL(6u, trace.GetLastObservation().ordinal);
            VERIFY_ARE_EQUAL(secondAttempt, trace.GetLastObservation().drawAttemptId);
            VERIFY_ARE_EQUAL(1u, trace.GetLastObservation().frameNumber);
            VERIFY_IS_TRUE(HasFlag(trace.GetLastObservation(), XamlLaunchObservationFlags::MultipleDrawAttempts));
            VERIFY_IS_FALSE(HasFlag(trace.GetLastObservation(), XamlLaunchObservationFlags::NonCanonicalOrder));
        }
    }

    void XamlLaunchTraceUnitTests::IndependentCoreLifetimes()
    {
        XamlLaunchTrace standalone;
        VERIFY_IS_TRUE(HasFlag(standalone.GetLastObservation(), XamlLaunchObservationFlags::NoApplicationStart));
        XamlLaunchStartupScope startup;
        XamlLaunchObservation first;
        {
            XamlLaunchTrace trace;
            first = trace.BeginOnLaunched();
            VERIFY_IS_TRUE(first.entryKind == XamlLaunchEntryKind::ApplicationStart);
        }
        XamlLaunchTrace replacement;
        const auto second = replacement.BeginOnLaunched();
        VERIFY_IS_TRUE(second.entryKind == XamlLaunchEntryKind::CoreInitialization);
        VERIFY_IS_TRUE(HasFlag(second, XamlLaunchObservationFlags::NoApplicationStart));
        VERIFY_ARE_NOT_EQUAL(first.startupId, second.startupId);
        VERIFY_ARE_NOT_EQUAL(first.coreId, second.coreId);
        VERIFY_ARE_NOT_EQUAL(standalone.GetLastObservation().coreId, second.coreId);
    }

    void XamlLaunchTraceUnitTests::StartupScopeIsThreadLocal()
    {
        XamlLaunchStartupScope startup;
        XamlLaunchObservation otherThread;
        std::thread worker([&otherThread]
        {
            XamlLaunchTrace trace;
            otherThread = trace.BeginOnLaunched();
        });
        worker.join();

        XamlLaunchTrace trace;
        const auto currentThread = trace.BeginOnLaunched();
        VERIFY_IS_TRUE(currentThread.entryKind == XamlLaunchEntryKind::ApplicationStart);
        VERIFY_IS_TRUE(otherThread.entryKind == XamlLaunchEntryKind::CoreInitialization);
        VERIFY_IS_TRUE(HasFlag(otherThread, XamlLaunchObservationFlags::NoApplicationStart));
        VERIFY_ARE_NOT_EQUAL(currentThread.startupId, otherThread.startupId);
        VERIFY_ARE_NOT_EQUAL(currentThread.coreId, otherThread.coreId);
    }

    void XamlLaunchTraceUnitTests::NestedStartupScopes()
    {
        XamlLaunchStartupScope outer;
        XamlLaunchObservation innerObservation;
        {
            XamlLaunchStartupScope inner;
            XamlLaunchTrace innerTrace;
            innerObservation = innerTrace.BeginOnLaunched();
        }
        XamlLaunchTrace outerTrace;
        const auto outerObservation = outerTrace.BeginOnLaunched();
        VERIFY_IS_TRUE(innerObservation.entryKind == XamlLaunchEntryKind::ApplicationStart);
        VERIFY_IS_TRUE(outerObservation.entryKind == XamlLaunchEntryKind::ApplicationStart);
        VERIFY_ARE_NOT_EQUAL(innerObservation.startupId, outerObservation.startupId);
    }

    void XamlLaunchTraceUnitTests::CallbackReturnAfterCoreReplacement()
    {
        XamlLaunchStartupScope startup;
        XamlLaunchObservation callback;
        {
            XamlLaunchTrace original;
            callback = original.BeginOnLaunched();
        }
        XamlLaunchTrace replacement;
        const auto replacementBefore = replacement.GetLastObservation();
        const auto returned = XamlLaunchTrace::EndOnLaunched(callback, &replacement, static_cast<uint32_t>(E_FAIL));
        VERIFY_ARE_EQUAL(callback.startupId, returned.startupId);
        VERIFY_ARE_EQUAL(callback.coreId, returned.coreId);
        VERIFY_ARE_EQUAL(3u, returned.ordinal);
        VERIFY_IS_TRUE(HasFlag(returned, XamlLaunchObservationFlags::CoreUnavailableAtCallbackReturn));
        VERIFY_IS_TRUE(HasFlag(returned, XamlLaunchObservationFlags::CallbackFailed));
        VERIFY_ARE_EQUAL(replacementBefore.ordinal, replacement.GetLastObservation().ordinal);
        VERIFY_ARE_EQUAL(replacementBefore.flags, replacement.GetLastObservation().flags);

        const auto withoutCore = XamlLaunchTrace::EndOnLaunched(callback, nullptr, S_OK);
        VERIFY_ARE_EQUAL(callback.coreId, withoutCore.coreId);
        VERIFY_IS_TRUE(HasFlag(withoutCore, XamlLaunchObservationFlags::CoreUnavailableAtCallbackReturn));
    }

    void XamlLaunchTraceUnitTests::WarmActivationAndLaterFramesDoNotRestart()
    {
        XamlLaunchStartupScope startup;
        XamlLaunchTrace trace;
        const auto callback = trace.BeginOnLaunched();
        XamlLaunchTrace::EndOnLaunched(callback, &trace, S_OK);
        Draw(trace);

        const auto laterCallback = trace.BeginOnLaunched();
        VERIFY_ARE_EQUAL(0u, laterCallback.ordinal);
        XamlLaunchTrace::EndOnLaunched(laterCallback, &trace, static_cast<uint32_t>(E_FAIL));
        Draw(trace);
        VERIFY_ARE_EQUAL(6u, trace.GetLastObservation().ordinal);
        VERIFY_ARE_EQUAL(0u, trace.GetLastObservation().flags);
        VERIFY_ARE_EQUAL(callback.coreId, trace.GetLastObservation().coreId);
    }

    void XamlLaunchTraceUnitTests::DrawCleanupFailureIsPreserved()
    {
        XamlLaunchStartupScope startup;
        XamlLaunchTrace trace;
        const auto callback = trace.BeginOnLaunched();
        XamlLaunchTrace::EndOnLaunched(callback, &trace, S_OK);
        const auto attempt = trace.BeginDrawAttempt();
        trace.BeginLayout(1, attempt);
        trace.BeginProduction(1, attempt);
        trace.EndDraw(1, attempt, true, static_cast<uint32_t>(E_FAIL));
        VERIFY_ARE_EQUAL(6u, trace.GetLastObservation().ordinal);
        VERIFY_ARE_EQUAL(static_cast<uint32_t>(E_FAIL), trace.GetLastObservation().result);
    }

} } } } }
