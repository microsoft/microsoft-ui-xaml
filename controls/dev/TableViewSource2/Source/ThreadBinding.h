// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>

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
