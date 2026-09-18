// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>

// Forward-declare HWND so this widely-included header does not have to pull in <windows.h>.
#ifndef _WINDEF_
struct HWND__;
typedef struct HWND__* HWND;
#endif

// ImeFocusPark makes XAML's TSF text-input focus signal explicit so that legacy (IMM/CUAS) IMEs can be
// re-enabled (by NOT calling ImmDisableLegacyIME) without the "focus ambiguity" regression: when no
// text-editable control is focused, the park holds TSF thread-manager focus on a permanent
// keyboard-disabled document, so TSF never falls back to the CUAS default input context and IMEs cannot
// consume keystrokes or open composition UI over non-text UI. While a text-editable control is focused,
// the park stands down and RichEdit owns TSF focus (restoring the legacy IME status bar).
//
// This mirrors the well-known Chromium / WPF "park focus on a benign document" pattern. It is installed
// only when the per-app opt-in (RuntimeEnabledFeature::EnableLegacyImeAuxiliaryUi) is set; default builds
// keep calling ImmDisableLegacyIME() and never create this object.
//
// Thread affinity: all methods must run on the owning UI thread (the TSF thread manager is thread-bound).
// Lifetime is owned by CInputServices (init in SetCoreWindow, teardown in Reset). The TSF details are
// hidden behind a pimpl so msctf.h does not leak into the widely-included InputServices.h.
class ImeFocusPark
{
public:
    ImeFocusPark();
    ~ImeFocusPark();

    ImeFocusPark(const ImeFocusPark&) = delete;
    ImeFocusPark& operator=(const ImeFocusPark&) = delete;

    // Acquires the per-thread TSF thread manager and creates the permanent keyboard-disabled document.
    // Idempotent; safe to call once per CInputServices. Returns a failure HRESULT if TSF is unavailable,
    // in which case the park is simply not installed (IsInitialized() stays false).
    _Check_return_ HRESULT Initialize();

    // Releases the parked focus and all TSF objects. Safe to call when not initialized.
    void Shutdown();

    bool IsInitialized() const;

    // Drives the park from the central focus-changed notification. When the newly focused element is NOT
    // text-editable, parks TSF thread focus on the disabled document AND binds that document to the focus
    // window (inputHwnd) via ITfThreadMgr::AssociateFocus, so TSF/CUAS reports the window as
    // keyboard-disabled on every WM_SETFOCUS (a one-time SetFocus is overridden by the HWND association).
    // When it IS text-editable, removes that association and stands down so RichEdit owns TSF focus.
    // inputHwnd is the island input HWND that owns TSF text-input for this focus (may be null). No-op when
    // the park is not initialized.
    void OnFocusedElementChanged(bool isFocusedElementTextEditable, HWND inputHwnd);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
