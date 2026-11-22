// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>

#include <roerrorapi.h>

#include <winrt/Microsoft.UI.Dispatching.h>

// The UI thread a SourceTable is bound to (see SourceTable.h, Threading). Written once: at
// SourceTable::Create() on a dispatcher thread, or by the first BindOwnerThread() while unbound;
// never changed after. Shared (std::shared_ptr) so the root-collection, child-collection and item
// lambdas read it without locking the table off-thread. threadId == 0 means unbound; it is stored
// after queue with release order and loaded with acquire, so a reader that sees it also sees
// queue. Holds an atomic, so it is shared, never copied.
struct ThreadBinding
{
    winrt::Microsoft::UI::Dispatching::DispatcherQueue queue{ nullptr };
    std::atomic<DWORD> threadId{ 0 };
};

// Collection-change lambdas (root and child), before locking the table: throws
// hresult_wrong_thread (asserting in debug) when bound and the current thread is not the bound one.
inline void ThrowIfOffBoundThread(ThreadBinding const& binding)
{
    DWORD const bound = binding.threadId.load(std::memory_order_acquire);
    if (bound != 0 && bound != ::GetCurrentThreadId())
    {
        MUX_ASSERT_MSG(false, "A source collection change was raised off the thread TableViewSource2 is bound to.");
        throw winrt::hresult_wrong_thread();
    }
}

// Called from the catch (...) of a posted DispatcherQueue callback. A throw escaping such a
// callback fail-fasts the process in DispatcherQueue without raising Application.UnhandledException,
// so posted work catches and reports here instead: RoReportUnhandledError raises the last-chance
// events (Application.UnhandledException) as XAML's own ErrorHelper::ReportUnhandledError does, and
// XAML fail-fasts if no handler marks the error handled. Returns only when it was handled.
inline void ReportPostedError() noexcept
{
    // Restores the exception's IRestrictedErrorInfo on the thread (originating one if it had none).
    HRESULT const hr = winrt::to_hresult();

    winrt::com_ptr<IRestrictedErrorInfo> info;
    if (FAILED(::GetRestrictedErrorInfo(info.put())) || !info)
    {
        (void)::RoOriginateError(hr, nullptr);
        (void)::GetRestrictedErrorInfo(info.put());
    }

    if (!info || FAILED(::RoReportUnhandledError(info.get())))
    {
        ::RoFailFastWithErrorContext(hr);
    }
}
