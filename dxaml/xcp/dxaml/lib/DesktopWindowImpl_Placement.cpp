// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Window placement persistence for desktop windows.
//
// See docs/design-notes/Window-PlacementPersistence.md. Section 3.1 is the restore load path,
// 3.2.2 is the policy applied before handing a placement to PlacementEx, and 3.3 is the save
// path. This file holds the DesktopWindowImpl members that implement those three sections;
// the rest of the class lives in DesktopWindowImpl.cpp.
//
// Nothing here may fail a caller for a persistence reason. A window must open when its
// placement cannot be read and must close when its placement cannot be written. The one
// exception is an out-of-range value in app-supplied InitialShowOptions, which is a
// programming error and returns E_INVALIDARG without consuming the placement attempt.

#include "precomp.h"

#include "Window.g.h"
#include <DesktopWindowImpl.h>
#include "WindowInitialShowOptions.g.h"
#include "WindowPlacementStore.h"

#include <WindowPlacementBlob.h>
#include <WindowPlacementValueName.h>

// PlacementEx reads the window's virtual desktop through the shell's virtual desktop APIs.
// Those are only compiled in when this is defined, and it must come before the include.
#define USE_VIRTUAL_DESKTOP_APIS
#include <PlacementEx/User32Utils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

namespace {

namespace wpp = WindowPlacementPersistence;

// Translates the durable subset of PlacementEx's flags into the wire bits WinUI persists.
// Call-time-only and structural flags are deliberately dropped (design 3.2.3).
wpp::DurableFlags ToDurableWireFlags(PlacementFlags flags)
{
    wpp::DurableFlags wire = wpp::DurableFlags::None;

    if (WI_IsFlagSet(flags, PlacementFlags::RestoreToMaximized))
    {
        wire |= wpp::DurableFlags::RestoreToMaximized;
    }
    if (WI_IsFlagSet(flags, PlacementFlags::Arranged))
    {
        wire |= wpp::DurableFlags::Arranged;
    }
    if (WI_IsFlagSet(flags, PlacementFlags::AllowPartiallyOffScreen))
    {
        wire |= wpp::DurableFlags::AllowPartiallyOffScreen;
    }
    if (WI_IsFlagSet(flags, PlacementFlags::AllowSizing))
    {
        wire |= wpp::DurableFlags::AllowSizing;
    }
    if (WI_IsFlagSet(flags, PlacementFlags::RestoreToArranged))
    {
        wire |= wpp::DurableFlags::RestoreToArranged;
    }

    return wire;
}

PlacementFlags FromDurableWireFlags(wpp::DurableFlags wire)
{
    PlacementFlags flags = PlacementFlags::None;

    if (wpp::HasFlag(wire, wpp::DurableFlags::RestoreToMaximized))
    {
        WI_SetFlag(flags, PlacementFlags::RestoreToMaximized);
    }
    if (wpp::HasFlag(wire, wpp::DurableFlags::Arranged))
    {
        WI_SetFlag(flags, PlacementFlags::Arranged);
    }
    if (wpp::HasFlag(wire, wpp::DurableFlags::AllowPartiallyOffScreen))
    {
        WI_SetFlag(flags, PlacementFlags::AllowPartiallyOffScreen);
    }
    if (wpp::HasFlag(wire, wpp::DurableFlags::AllowSizing))
    {
        WI_SetFlag(flags, PlacementFlags::AllowSizing);
    }
    if (wpp::HasFlag(wire, wpp::DurableFlags::RestoreToArranged))
    {
        WI_SetFlag(flags, PlacementFlags::RestoreToArranged);
    }

    return flags;
}

void ToPlacementEx(const wpp::PlacementBlobData& data, _Out_ PlacementEx* placement)
{
    placement->Clear();

    placement->normalRect = data.normalRect;
    placement->workArea = data.workArea;
    placement->dpi = data.dpi;
    placement->showCmd = data.showCmd;
    placement->flags = FromDurableWireFlags(data.flags);

    if (data.hasArrangeRect)
    {
        placement->arrangeRect = data.arrangeRect;
    }

    // Both buffers are CCHDEVICENAME and the reader already bounded the string to fit.
    wcscpy_s(placement->deviceName, ARRAYSIZE(placement->deviceName), data.deviceName);

    if (data.hasVirtualDesktopId)
    {
        placement->virtualDesktopId = data.virtualDesktopId;
        WI_SetFlag(placement->flags, PlacementFlags::VirtualDesktopId);
    }
}

void FromPlacementEx(const PlacementEx& placement, _Out_ wpp::PlacementBlobData* data)
{
    *data = {};

    data->normalRect = placement.normalRect;
    data->workArea = placement.workArea;
    data->dpi = placement.dpi;
    data->showCmd = wpp::CanonicalizeShowCommand(placement.showCmd);
    data->flags = ToDurableWireFlags(placement.flags);

    // The arrange rect only means anything alongside a snap flag, and the writer rejects a
    // snapped placement that has no arrange rect.
    if (wpp::HasFlag(data->flags, wpp::DurableFlags::Arranged) ||
        wpp::HasFlag(data->flags, wpp::DurableFlags::RestoreToArranged))
    {
        data->hasArrangeRect = true;
        data->arrangeRect = placement.arrangeRect;
    }

    wcscpy_s(data->deviceName, ARRAYSIZE(data->deviceName), placement.deviceName);

    if (WI_IsFlagSet(placement.flags, PlacementFlags::VirtualDesktopId))
    {
        data->hasVirtualDesktopId = true;
        data->virtualDesktopId = placement.virtualDesktopId;
    }
}

// A window minimized while snapped saves RestoreToArranged. Reopening it should land it back
// in its snapped position rather than minimized, so we convert the deferred snap into a live
// one before PlacementEx sees it (design 3.2.2 item 2). PlacementEx already handles the
// equivalent RestoreToMaximized conversion itself.
void ConvertMinimizedFromArranged(_Inout_ PlacementEx* placement)
{
    if (placement->showCmd == SW_SHOWMINIMIZED &&
        WI_IsFlagSet(placement->flags, PlacementFlags::RestoreToArranged))
    {
        WI_ClearFlag(placement->flags, PlacementFlags::RestoreToArranged);
        WI_SetFlag(placement->flags, PlacementFlags::Arranged);
    }
}

} // anonymous namespace

_Check_return_ HRESULT DesktopWindowImpl::SnapshotInitialShowOptions(
    bool forceActivate,
    _Out_ PlacementOptionsSnapshot* snapshot)
{
    *snapshot = {};

    if (m_initialShowOptions)
    {
        xaml::WindowShowReason reason = xaml::WindowShowReason_Default;
        xaml::WindowActivationBehavior activationBehavior = xaml::WindowActivationBehavior_Activate;
        BOOLEAN keepHidden = FALSE;

        IFC_RETURN(m_initialShowOptions->get_Reason(&reason));
        IFC_RETURN(m_initialShowOptions->get_ActivationBehavior(&activationBehavior));
        IFC_RETURN(m_initialShowOptions->get_KeepHidden(&keepHidden));

        // Validate before anything observable happens. An app that passes a value outside the
        // enum gets E_INVALIDARG and keeps its placement attempt, so it can fix the value and
        // show again (design 3.1 step 2).
        if (reason != xaml::WindowShowReason_Default &&
            reason != xaml::WindowShowReason_Launch &&
            reason != xaml::WindowShowReason_ApplicationRestart)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        if (activationBehavior != xaml::WindowActivationBehavior_Activate &&
            activationBehavior != xaml::WindowActivationBehavior_DoNotActivate)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        snapshot->reason = reason;
        snapshot->activationBehavior = activationBehavior;
        snapshot->keepHidden = !!keepHidden;
    }

    // Activate() is an explicit request to show and focus the window. It overrides the
    // activation and visibility options but still honors Reason, because the reason describes
    // why the app is showing the window, not how (design 3.2.2 item 4).
    if (forceActivate)
    {
        snapshot->activationBehavior = xaml::WindowActivationBehavior_Activate;
        snapshot->keepHidden = false;
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowImpl::RunInitialPlacementAttempt(bool forceActivate, _Out_ PlacementOptionsSnapshot* snapshotOut)
{
    ASSERT(AreNewWindowingApisEnabled());

    *snapshotOut = {};

    if (m_placementAttemptState != PlacementAttemptState::NotAttempted)
    {
        return S_OK;
    }

    // Validate first. This is the only path that can fail the caller, and it must not have
    // consumed the attempt or changed window state when it does.
    PlacementOptionsSnapshot snapshot;
    IFC_RETURN(SnapshotInitialShowOptions(forceActivate, &snapshot));
    *snapshotOut = snapshot;

    m_placementAttemptState = PlacementAttemptState::InProgress;

    // From here on the attempt is consumed no matter what happens. Everything below is
    // best-effort: an empty id, an unreachable store, a corrupt blob, and a failed apply all
    // leave the window on its default placement.
    auto markAttempted = wil::scope_exit([this]()
    {
        if (m_placementAttemptState == PlacementAttemptState::InProgress)
        {
            m_placementAttemptState = PlacementAttemptState::AttemptedWithoutPlacement;
        }
    });

    // Pending Width/Height are applied first and stay as the fallback. A placement we
    // successfully apply supersedes them below.
    IFC_RETURN(ApplyPendingClientSizeIfNeeded());

    if (m_hwnd == nullptr)
    {
        return S_OK;
    }

    // The id is read live, so an app can set it in a Loaded handler and still get a restore.
    const UINT32 idLength = m_persistPlacementId.Length();
    if (idLength == 0)
    {
        return S_OK;
    }

    const std::wstring rawId(m_persistPlacementId.GetRawBuffer(nullptr), idLength);

    std::wstring valueName;
    if (!wpp::TryGetPlacementValueName(rawId.c_str(), rawId.length(), valueName))
    {
        return S_OK;
    }

    PlacementEx placement;
    bool havePlacement = false;

    std::wstring encoded;
    if (wpp::Store::TryLoad(valueName, encoded))
    {
        wpp::PlacementBlobData data;
        if (wpp::TryDecodePlacement(encoded.c_str(), encoded.length(), data))
        {
            ToPlacementEx(data, &placement);
            havePlacement = placement.IsValid();
        }
    }

    if (!havePlacement && snapshot.reason == xaml::WindowShowReason_Launch)
    {
        // No saved placement, but the shell may still have told us which monitor to launch on.
        // Seed from the window's current geometry so the monitor hint has something valid to
        // migrate (design 3.1 step 11). Skip the virtual desktop query: we are on the show
        // path and do not need desktop identity for a launch.
        if (PlacementEx::GetPlacement(m_hwnd.get(), &placement, CaptureFlags::SkipVirtualDesktopId))
        {
            placement.showCmd = wpp::CanonicalizeShowCommand(placement.showCmd);
            WI_ClearAllFlags(placement.flags,
                PlacementFlags::FullScreen |
                PlacementFlags::KeepHidden |
                PlacementFlags::NoActivate |
                PlacementFlags::NoApplyWindowAction |
                PlacementFlags::VirtualDesktopId);
            havePlacement = placement.IsValid();
        }
    }

    if (!havePlacement)
    {
        return S_OK;
    }

    // ---- Policy before apply (design 3.2.2) ----

    // ApplicationRestart that is not activating is the only case that preserves a saved
    // minimized state and tries to return the window to its saved virtual desktop. Everything
    // else normalizes through AdjustForMainWindow, which also strips desktop identity.
    const bool nonActivatingRestart =
        snapshot.reason == xaml::WindowShowReason_ApplicationRestart &&
        snapshot.activationBehavior == xaml::WindowActivationBehavior_DoNotActivate;

    if (!nonActivatingRestart)
    {
        ConvertMinimizedFromArranged(&placement);

        // Launch is the only reason that consumes the shell's monitor hint. We never pass
        // ShowCommand: Win32 already applied STARTUPINFO.wShowWindow to the process's first
        // ShowWindow, and reapplying it here would double-apply it (design 3.2.2 item 7).
        const StartupInfoFlags startupFlags =
            (snapshot.reason == xaml::WindowShowReason_Launch)
                ? StartupInfoFlags::MonitorHint
                : StartupInfoFlags::None;

        placement.AdjustForMainWindow(nullptr, startupFlags);

        // Desktop identity is only honored for a non-activating restart. Activating a window
        // onto a desktop the user is not looking at would yank them across desktops.
        WI_ClearFlag(placement.flags, PlacementFlags::VirtualDesktopId);
    }

    // Call-time instructions. These control visibility and focus for this one apply and are
    // never persisted (design 3.2.2 item 10).
    if (snapshot.keepHidden)
    {
        WI_SetFlag(placement.flags, PlacementFlags::KeepHidden);
        WI_SetFlag(placement.flags, PlacementFlags::NoActivate);
    }
    else if (snapshot.activationBehavior == xaml::WindowActivationBehavior_DoNotActivate ||
             WI_IsFlagSet(placement.flags, PlacementFlags::VirtualDesktopId))
    {
        WI_SetFlag(placement.flags, PlacementFlags::NoActivate);
    }

    if (!PlacementEx::SetPlacement(m_hwnd.get(), &placement))
    {
        // Could not place the window. Continue on the normal first-show path.
        return S_OK;
    }

    // The placement won, so drop the pending size. Otherwise a later presenter transition
    // could reapply a stale Width/Height over the restored geometry (design 3.2.2 item 1).
    m_pendingClientWidthDips.reset();
    m_pendingClientHeightDips.reset();

    m_lastNonHiddenShowCmd = wpp::CanonicalizeShowCommand(placement.showCmd);
    m_placementAttemptState = PlacementAttemptState::Applied;

    return S_OK;
}

void DesktopWindowImpl::SavePlacement(bool skipVirtualDesktopQuery)
{
    if (!AreNewWindowingApisEnabled())
    {
        return;
    }

    // A window that was constructed but never shown has nothing worth saving, and writing its
    // default geometry would clobber good data (design 3.3 step 1).
    if (m_placementAttemptState == PlacementAttemptState::NotAttempted && !m_hwndEverDisplayed)
    {
        return;
    }

    if (m_hwnd == nullptr)
    {
        return;
    }

    // The id is read live, so an app that renamed its window between restore and close writes
    // to the new slot and leaves the old one alone.
    const UINT32 idLength = m_persistPlacementId.Length();
    if (idLength == 0)
    {
        return;
    }

    const std::wstring rawId(m_persistPlacementId.GetRawBuffer(nullptr), idLength);

    std::wstring valueName;
    if (!wpp::TryGetPlacementValueName(rawId.c_str(), rawId.length(), valueName))
    {
        return;
    }

    PlacementEx placement;
    const CaptureFlags captureFlags = skipVirtualDesktopQuery
        ? CaptureFlags::SkipVirtualDesktopId
        : CaptureFlags::None;

    if (!PlacementEx::GetPlacement(m_hwnd.get(), &placement, captureFlags))
    {
        return;
    }

    // A hidden window reports SW_HIDE, which we never persist. Fall back to the last show
    // state we saw it in (design 3.3 step 6).
    if (!::IsWindowVisible(m_hwnd.get()))
    {
        placement.showCmd = m_lastNonHiddenShowCmd;
    }
    else
    {
        placement.showCmd = wpp::CanonicalizeShowCommand(placement.showCmd);
    }

    // v1 does not persist presenter state. A window in FullScreen or CompactOverlay saves the
    // overlapped geometry it will return to, not its current bounds (design 3.3 step 5).
    WI_ClearFlag(placement.flags, PlacementFlags::FullScreen);

    wpp::PlacementBlobData data;
    FromPlacementEx(placement, &data);

    std::wstring encoded;
    if (!wpp::TryEncodePlacement(data, encoded))
    {
        return;
    }

    wpp::Store::TrySave(valueName, encoded);
}
