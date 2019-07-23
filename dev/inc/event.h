// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <set>

//
// This is simple event implementation that is single-threaded and allows for customization of
// the storage type to be a TrackerPtr
//

static int64_t s_eventHandlerId = 0;

template <typename ImplT, typename T, typename StorageT = T>
class event_base
{
protected:
    std::shared_ptr<std::map<int64_t, StorageT>> m_handlers;

public:

    event_base(const event_base &) = delete;
    event_base & operator=(const event_base &) = delete;

    event_base() = default;

    ~event_base()
    {
    }

    winrt::event_token add(const T & value)
    {
        auto token = InterlockedIncrement64(&s_eventHandlerId);
        
        // Make a new map because add/remove during event call-out can happen and we want to
        // swap in a new container instead of modifying the one that we're iterating over.
        // This is a tradeoff -- either we copy the map every time during invoke in case add/remove
        // happens or we make a new copy during add/remove. We (and the C++/WinRT implementation
        // we're copied from) choose the latter.
        auto handlers = std::make_shared<std::map<int64_t, StorageT>>();

        if (auto * before = m_handlers.get())
        {
            handlers->insert(before->begin(), before->end());
        }
        
        auto holder = Impl()->wrap(value);
        handlers->insert({ token, holder });
        m_handlers = std::move(handlers);
        return winrt::event_token{ token };
    }

    void remove(const winrt::event_token token)
    {
        // See comment in add(), we make a copy of the map on add/remove just like the C++/WinRT event implementation.
        auto handlers = std::make_shared<std::map<int64_t, StorageT>>();

        if (auto * before = m_handlers.get())
        {
            handlers->insert(before->begin(), before->end());
        }

        handlers->erase(token.value);
        m_handlers = std::move(handlers);
    }

    template <typename... A> void operator()(A const & ... args) const
    {
        auto handlers = m_handlers;

        if (auto * map = handlers.get())
        {
            for (const auto & pair : *map)
            {
                auto handler = Impl()->unwrap(pair.second);
                handler(args...);
            }
        }
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_handlers) && m_handlers.get()->size() > 0;
    }

private:
    const ImplT* Impl() const { return static_cast<const ImplT*>(this); }
};

template <typename T>
class event_source :
    public event_base<event_source<T>, T, tracker_ref<T>>,
    public ReferenceTrackerContainerBase,
    public ReferenceTrackerStorageHelper<event_source<T>, T>
{
public:
    explicit event_source(ITrackerHandleManager* owner)
        : ReferenceTrackerContainerBase{ owner }
    {
    }
};

// Bug #11634051 if we don't have winrt::Implements we can remove this.
template <typename T>
class event :
    public event_base<event<T>, T, T>,
    public NonReferenceTrackerStorageHelper<T>
{
};