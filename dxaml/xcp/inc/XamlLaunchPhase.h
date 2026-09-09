// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

// Labels for observed boundaries, not a state machine. See docs/design-notes/launch-phase-markers.md.
enum class XamlLaunchPhase : uint32_t
{
    None = 0,
    FrameworkInitialization = 1,
    ApplicationInitialization = 2,
    FirstFrameScheduling = 3,
    InitialLayout = 4,
    FirstFrameProduction = 5,
    Complete = 6,
    Aborted = 7
};

enum class XamlLaunchEntryKind : uint32_t
{
    ApplicationStart = 1,
    CoreInitialization = 2
};

enum class XamlLaunchObservationFlags : uint32_t
{
    None = 0,
    NoApplicationStart = 0x1,
    NonCanonicalOrder = 0x2,
    CallbackFailed = 0x4,
    MultipleDrawAttempts = 0x8,
    CoreUnavailableAtCallbackReturn = 0x10
};

struct XamlLaunchObservation
{
    uint64_t startupId = 0;
    uint64_t coreId = 0;
    XamlLaunchEntryKind entryKind = XamlLaunchEntryKind::CoreInitialization;
    XamlLaunchPhase previousPhase = XamlLaunchPhase::None;
    XamlLaunchPhase nextPhase = XamlLaunchPhase::None;
    uint32_t ordinal = 0;
    uint32_t frameNumber = 0;
    uint64_t drawAttemptId = 0;
    uint32_t flags = 0;
    uint32_t result = 0;
};

// Associates only the first core created on this thread with this Application.Start.
// Other threads and subsequent cores have independent, explicitly partial sequences.
class XamlLaunchStartupScope final
{
public:
    XamlLaunchStartupScope() noexcept;
    ~XamlLaunchStartupScope();

    XamlLaunchStartupScope(const XamlLaunchStartupScope&) = delete;
    XamlLaunchStartupScope& operator=(const XamlLaunchStartupScope&) = delete;

private:
    friend class XamlLaunchTrace;
    static thread_local XamlLaunchStartupScope* s_current;
    XamlLaunchStartupScope* m_previous;
    uint64_t m_startupId;
    bool m_claimed = false;
};

// UI-thread-owned observations for one core lifetime. Callback and frame completion
// are independent: a nested message pump can produce a frame before callback return.
class XamlLaunchTrace final
{
public:
    XamlLaunchTrace() noexcept;
    XamlLaunchTrace(const XamlLaunchTrace&) = delete;
    XamlLaunchTrace& operator=(const XamlLaunchTrace&) = delete;

    XamlLaunchObservation BeginOnLaunched() noexcept;
    static XamlLaunchObservation EndOnLaunched(
        const XamlLaunchObservation& callbackStart,
        XamlLaunchTrace* currentCoreTrace,
        uint32_t result) noexcept;

    uint64_t BeginDrawAttempt() noexcept;
    void BeginLayout(uint32_t frameNumber, uint64_t drawAttemptId) noexcept;
    void BeginProduction(uint32_t frameNumber, uint64_t drawAttemptId) noexcept;
    void EndDraw(uint32_t frameNumber, uint64_t drawAttemptId, bool frameDrawn, uint32_t result) noexcept;

    const XamlLaunchObservation& GetLastObservation() const noexcept { return m_observation; }

private:
    void WriteBoundary(
        XamlLaunchPhase previousPhase,
        XamlLaunchPhase nextPhase,
        uint32_t ordinal,
        uint32_t frameNumber = 0,
        uint64_t drawAttemptId = 0,
        uint32_t result = 0) noexcept;

    XamlLaunchObservation m_observation;
    uint64_t m_drawAttemptId = 0;
    uint64_t m_layoutAttemptId = 0;
    uint32_t m_lastOrdinal = 1;
    bool m_callbackStarted = false;
    bool m_productionStarted = false;
    bool m_frameCompleted = false;
};
