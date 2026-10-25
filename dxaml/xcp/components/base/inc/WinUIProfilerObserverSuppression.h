// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

namespace WinUIProfiler::ObserverSuppression
{
    // Constant-initialized POD TLS: no allocation, PAL access, or TLS destructor.
    inline thread_local bool suppressed = false;
    inline bool IsSuppressed() noexcept { return suppressed; }
    inline bool Exchange(bool value) noexcept
    {
        const bool previous = suppressed;
        suppressed = value;
        return previous;
    }
    class Scope final
    {
        bool m_previous;
    public:
        Scope() noexcept : m_previous(Exchange(true)) {}
        ~Scope() noexcept { Exchange(m_previous); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };
}
