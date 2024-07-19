// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Macros to make it easier to declare / instantiate instances of the template class for specific handle data type
#define DECLARE_AUTOHANDLE_CLASS(className, handlePolicy) \
    typedef MUXControls::Common::AutoHandle<handlePolicy> className; \

namespace MUXControls { namespace Common {

    template <typename THandlePolicy>
    class AutoHandle
    {
    public:
        using THandleType = typename THandlePolicy::THandleType;

        AutoHandle();
        explicit AutoHandle(THandleType handle); // Conversion constructor
        AutoHandle(AutoHandle&& other);
        ~AutoHandle();

        AutoHandle& operator=(AutoHandle&& other);
        AutoHandle& operator=(const THandleType& other);

        bool operator==(THandleType handle) const;
        bool operator!=(THandleType handle) const;

        THandleType* operator&()
        {
            return &m_handle;
        }

        operator THandleType() const;

        void Attach(THandleType handle);

        THandleType Detach()
        {
            THandleType handle = m_handle;
            THandlePolicy::Invalidate(m_handle);
            return handle;
        }

        void Release();

        bool IsValid() const;

    private:
        AutoHandle(const AutoHandle&); // not implemented
        AutoHandle& operator=(const AutoHandle&); // not implemented

        THandleType m_handle;
    };

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>::AutoHandle()
        : m_handle(THandlePolicy::Invalidate(m_handle))
    {
    }

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>::AutoHandle(THandleType handle)
        : m_handle(handle)
    {
    }

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>::AutoHandle(AutoHandle&& other)
        : m_handle(other.Detach())
    {
    }

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>::~AutoHandle()
    {
        Release();
    }

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>& AutoHandle<THandlePolicy>::operator=(const THandleType& other)
    {
        Attach(other);
        return *this;
    }

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>& AutoHandle<THandlePolicy>::operator=(AutoHandle&& other)
    {
        Attach(other.Detach());
        return *this;
    }

    template <typename THandlePolicy>
    bool AutoHandle<THandlePolicy>::operator==(THandleType handle) const
    {
        return (m_handle == handle);
    }

    template <typename THandlePolicy>
    bool AutoHandle<THandlePolicy>::operator!=(THandleType handle) const
    {
        return (m_handle != handle);
    }

    template <typename THandlePolicy>
    AutoHandle<THandlePolicy>::operator THandleType() const
    {
        return m_handle;
    }

    template <typename THandlePolicy>
    void AutoHandle<THandlePolicy>::Attach(THandleType handle)
    {
        if (m_handle != handle)
        {
            Release();
        }

        m_handle = handle;
    }

    template <typename THandlePolicy>
    void AutoHandle<THandlePolicy>::Release()
    {
        if (THandlePolicy::IsValid(m_handle))
        {
            THandlePolicy::Close(m_handle);
        }

        THandlePolicy::Invalidate(m_handle);
    }

    template <typename THandlePolicy>
    bool AutoHandle<THandlePolicy>::IsValid() const
    {
        return THandlePolicy::IsValid(m_handle);
    }

    class HandlePolicy
    {
    public:
        using THandleType = HANDLE;

        static void Close(HANDLE& handle)
        {
            ::CloseHandle(handle);
        }

        static bool IsValid(const HANDLE& handle)
        {
            return (INVALID_HANDLE_VALUE != handle) && handle;
        }

        static HANDLE Invalidate(HANDLE& handle)
        {
            handle = INVALID_HANDLE_VALUE;
            return handle;
        }
    };

    // Declare most used AutoHandles
    DECLARE_AUTOHANDLE_CLASS(Handle, HandlePolicy);

} } // namespace MUXControls::Common
