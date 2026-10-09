// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <weakreference.h>
#include <wil/com.h>
#include <cstdint>
#include <utility>

namespace TableViewTelemetry
{
    // Weak publisher ownership avoids retaining an ancestor/control cycle. Removal never projects the control.
    class Registration
    {
    public:
        using Remove = HRESULT (*)(::IInspectable*, ::IInspectable*, int64_t) noexcept;

        Registration() = default;
        ~Registration() { Reset(); }
        Registration(Registration const&) = delete;
        Registration& operator=(Registration const&) = delete;
        Registration(Registration&& other) noexcept { MoveFrom(other); }
        Registration& operator=(Registration&& other) noexcept
        {
            if (this != &other) { Reset(); MoveFrom(other); }
            return *this;
        }

        bool Prepare(::IUnknown* publisher, GUID const& iid, ::IInspectable* argument, Remove remove) noexcept
        {
            Reset();
            wil::com_ptr_nothrow<::IWeakReferenceSource> source;
            if (!publisher || FAILED(publisher->QueryInterface(IID_PPV_ARGS(source.put()))) ||
                FAILED(source->GetWeakReference(m_publisher.put()))) { return false; }
            m_argument = argument;
            m_iid = iid;
            m_remove = remove;
            return true;
        }

        void Arm(int64_t token) noexcept { m_token = token; m_armed = true; }
        bool IsArmed() const noexcept { return m_armed; }

        void Reset() noexcept
        {
            auto publisher = std::move(m_publisher);
            auto argument = std::move(m_argument);
            auto const armed = std::exchange(m_armed, false);
            auto const token = std::exchange(m_token, 0);
            auto const remove = std::exchange(m_remove, nullptr);
            if (!armed || !publisher || !remove) { return; }
            wil::com_ptr_nothrow<::IInspectable> resolved;
            try
            {
                if (SUCCEEDED(publisher->Resolve(m_iid, resolved.put())) && resolved)
                {
                    (void)remove(resolved.get(), argument.get(), token);
                }
            }
            catch (...) {}
        }

    private:
        void MoveFrom(Registration& other) noexcept
        {
            m_publisher = std::move(other.m_publisher);
            m_argument = std::move(other.m_argument);
            m_iid = other.m_iid;
            m_remove = std::exchange(other.m_remove, nullptr);
            m_token = std::exchange(other.m_token, 0);
            m_armed = std::exchange(other.m_armed, false);
        }

        wil::com_ptr_nothrow<::IWeakReference> m_publisher;
        wil::com_ptr_nothrow<::IInspectable> m_argument;
        GUID m_iid{};
        Remove m_remove{};
        int64_t m_token{};
        bool m_armed{};
    };
}
