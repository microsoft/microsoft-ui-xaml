// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlOptionalChanges.g.h"
#include <OptionalChangeState.h>

using namespace DirectUI;

// ---------------------------------------------------------------------------
// Process-wide state
//
// The storage lives in OptionalChangeState.cpp (components layer) so that
// both core and DXaml code can access it.  We alias them here for
// readability.  No atomics — writes are serialized by the SRW lock and
// complete before XAML init spawns threads that read.
// ---------------------------------------------------------------------------
static auto& s_enabledChanges = OptionalChangeState::g_enabledChanges;
static auto& s_locked         = OptionalChangeState::g_locked;
static auto& s_srwLock        = OptionalChangeState::g_srwLock;

// ---------------------------------------------------------------------------
// GetBitIndex
//
// Maps a XamlChangeId to its bit position in s_enabledChanges.
// Returns -1 for any value not recognized by this build, which causes
// EnableChange/DisableChange to silently no-op and IsChangeEnabled to
// return false.
//
// When adding a new XamlChangeId value to the model, add a corresponding
// case here and a BitIndex_ constant in OptionalChangeState.h.
// ---------------------------------------------------------------------------
static int GetBitIndex(xaml_settings::XamlChangeId id)
{
    switch (id)
    {
    case xaml_settings::XamlChangeId_IconNoGridOptimization:
        return OptionalChangeState::BitIndex_IconNoGridOptimization;
    case xaml_settings::XamlChangeId_DelayApplyStyleOptimization:
        return OptionalChangeState::BitIndex_DelayApplyStyleOptimization;
    default:
        return -1;
    }
}

// ---------------------------------------------------------------------------
// XamlOptionalChangesFactory — IXamlOptionalChangesStatics Impl methods
// ---------------------------------------------------------------------------

_Check_return_ HRESULT XamlOptionalChangesFactory::EnableChangeImpl(_In_ xaml_settings::XamlChangeId changeId)
{
    int bit = GetBitIndex(changeId);

    // Silently ignore unrecognized values regardless of lock state.
    if ((bit < 0) || (bit >= 64))
    {
        return S_OK;
    }

    bool wasLocked;
    {
        auto lock = wil::AcquireSRWLockExclusive(&s_srwLock);
        wasLocked = s_locked;
        if (!wasLocked)
        {
            s_enabledChanges |= (1ULL << bit);
        }
    }

    if (wasLocked)
    {
        IFC_NOTRACE_RETURN(ErrorHelper::OriginateError(
            E_ILLEGAL_STATE_CHANGE,
            wrl_wrappers::HStringReference(
                L"XamlOptionalChanges cannot be modified after XAML has been initialized. "
                L"Call EnableChange/DisableChange before Application.Start() or "
                L"WindowsXamlManager.InitializeForCurrentThread().").Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlOptionalChangesFactory::DisableChangeImpl(_In_ xaml_settings::XamlChangeId changeId)
{
    int bit = GetBitIndex(changeId);

    // Silently ignore unrecognized values regardless of lock state.
    if ((bit < 0) || (bit >= 64))
    {
        return S_OK;
    }

    bool wasLocked;
    {
        auto lock = wil::AcquireSRWLockExclusive(&s_srwLock);
        wasLocked = s_locked;
        if (!wasLocked)
        {
            s_enabledChanges &= ~(1ULL << bit);
        }
    }

    if (wasLocked)
    {
        IFC_NOTRACE_RETURN(ErrorHelper::OriginateError(
            E_ILLEGAL_STATE_CHANGE,
            wrl_wrappers::HStringReference(
                L"XamlOptionalChanges cannot be modified after XAML has been initialized. "
                L"Call EnableChange/DisableChange before Application.Start() or "
                L"WindowsXamlManager.InitializeForCurrentThread().").Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlOptionalChangesFactory::IsChangeEnabledImpl(_In_ xaml_settings::XamlChangeId changeId, _Out_ BOOLEAN* pResult)
{
    // The public ABI method is safe to call at any time (before or after lock).
    // Do NOT delegate to IsChangeEnabledInternal() which FAIL_FAST_ASSERTs that
    // the state is locked — that check is for internal platform callers only.
    int bit = GetBitIndex(changeId);
    *pResult = (bit >= 0 && (s_enabledChanges & (1ULL << bit)) != 0) ? TRUE : FALSE;
    return S_OK;
}

_Check_return_ HRESULT XamlOptionalChangesFactory::LockImpl(_Out_ BOOLEAN* pResult)
{
    *pResult = XamlOptionalChanges::LockInternal() ? TRUE : FALSE;
    return S_OK;
}

_Check_return_ HRESULT XamlOptionalChangesFactory::IsLockedImpl(_Out_ BOOLEAN* pResult)
{
    *pResult = XamlOptionalChanges::IsLockedInternal() ? TRUE : FALSE;
    return S_OK;
}

// ---------------------------------------------------------------------------
// XamlOptionalChanges — Internal C++ API
// ---------------------------------------------------------------------------

/*static*/ bool XamlOptionalChanges::IsChangeEnabledInternal(_In_ xaml_settings::XamlChangeId changeId)
{
    // Platform-internal queries must happen after XAML initialization has
    // locked the optional-changes set.  Querying before that is a bug —
    // the answer could change out from under the caller.
    FAIL_FAST_ASSERT(IsLockedInternal());

    int bit = GetBitIndex(changeId);
    if (bit < 0)
    {
        return false;
    }
    return (s_enabledChanges & (1ULL << bit)) != 0;
}

/*static*/ bool XamlOptionalChanges::IsLockedInternal()
{
    return s_locked;
}

/*static*/ bool XamlOptionalChanges::LockInternal()
{
    // Fast path: already locked (common case once XAML is initialized).
    if (s_locked)
    {
        return false;
    }

    bool performedLock;
    {
        auto lock = wil::AcquireSRWLockExclusive(&s_srwLock);
        performedLock = !s_locked;
        if (performedLock)
        {
            s_locked = true;
        }
    }

    return performedLock;
}

/*static*/ void XamlOptionalChanges::ResetInternal()
{
    {
        auto lock = wil::AcquireSRWLockExclusive(&s_srwLock);
        s_enabledChanges = 0;
        s_locked = false;
    }
}
