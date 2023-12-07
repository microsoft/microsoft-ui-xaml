// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace ctl
{
    // Base used by event_handler, weak_event_handler, and tokenless_event_handler.
    template <class TINTERFACE, class TSENDER, class TARGS, class TEVENTTRAITS>
    class __declspec(novtable) event_handler_base:
        public implements<TINTERFACE>
    {
    public:
        using HandlerFunction = std::function<HRESULT (TSENDER *, TARGS *)>;

    protected:
        typedef typename TEVENTTRAITS::event_interface TSOURCE;

        _Check_return_ HRESULT Initialize(HandlerFunction func)
        {
            m_handler = func;
            RRETURN(S_OK);
        }

        IFACEMETHODIMP Invoke(TSENDER *pSender, TARGS *pArgs) override
        {
            // If the event handler has been disabled then
            // cleanup ourselves from the event source
            if (!m_handler)
            {
                VERIFYHR(RemoveEventHandler(pSender));
                return S_OK;
            }

            IFC_RETURN(m_handler(pSender, pArgs));

            return S_OK;
        }

        // This method will disable the event handler so future invocations
        // will just do the cleanup. By doing the removal of the event handler this way
        // we work arround an issue in which during cleanup of the event handler owner
        // we might not have access to the event source to remove the event.
        void DetachHandler()
        {
            // Disable this handler so we don't call back
            m_handler = nullptr;
        }

        // This method will actively remove the event handler from the source
        // it should not be called from a destructor or a destructor path
        _Check_return_ HRESULT DisconnectHandler(_In_ IInspectable *pSource)
        {
            DetachHandler();

            // TODO: Check that it is called in the right context
            RRETURN(RemoveEventHandler(pSource));
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        event_handler_base() = default;

        virtual _Check_return_ HRESULT RemoveEventHandler(_In_ IInspectable *pSource) = 0;

    private:
        HandlerFunction m_handler;

        template <typename T>
        friend class EventPtrBase;

        template <typename T>
        friend class EventPtr;

        template <typename T>
        friend class WeakEventPtr;
    };

// token_event_handler adds the EventRegistrationToken logic to event_handler_base
template <class TINTERFACE, class TSENDER, class TARGS, class TEVENTTRAITS>
class __declspec(novtable) token_event_handler  :
    public event_handler_base<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>
{
public:
    using typename event_handler_base<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>::HandlerFunction;

private:
    _Check_return_ HRESULT AttachHandler(_In_ typename TEVENTTRAITS::event_interface *pSource)
    {
        IFC_RETURN(TEVENTTRAITS::attach_handler(pSource, this, &m_token));
        return S_OK;
    }

protected:
    _Check_return_ HRESULT RemoveEventHandler(_In_ IInspectable *pSource) final
    {
        HRESULT hr = S_OK;
        typename TEVENTTRAITS::event_interface *pEventSource = NULL;

        IFC(ctl::do_query_interface(pEventSource, pSource));
        VERIFYHR(TEVENTTRAITS::detach_handler(pEventSource, m_token));

    Cleanup:
        ReleaseInterface(pEventSource);
        RRETURN(hr);
    }

private:
    EventRegistrationToken m_token;

    template <typename T>
    friend class EventPtrBase;
};

// event_handler is a final version of token_event_handler
template <class TINTERFACE, class TSENDER, class TARGS, class TEVENTTRAITS>
class event_handler  final :
    public token_event_handler<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>,
    public Internal::EventPtr_compatible_tag
{
public:
    using typename token_event_handler<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>::HandlerFunction;
};

// weak_event_handler is like event_handler, except it keeps a weak reference to the target and
// fails gracefully if the target goes away.
template <class TINTERFACE, class TSENDER, class TARGS, class TEVENTTRAITS>
class weak_event_handler final :
    public token_event_handler<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>,
    public Internal::WeakEventPtr_compatible_tag
{
    using base_type = token_event_handler<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>;

public:
    using typename base_type::HandlerFunction;

    _Check_return_ HRESULT Initialize(ComPtr<IInspectable> target, HandlerFunction func)
    {
        IFC_RETURN( base_type::Initialize(func));
        IFC_RETURN( target.AsWeak( &m_targetWeakReference ));
        return S_OK;
    }

    IFACEMETHODIMP Invoke(TSENDER *sender, TARGS *args) override
    {
        // If the target is gone, null out m_handler
        auto target = m_targetWeakReference.AsOrNull<IInspectable>();
        if( target == nullptr)
        {
            this->DetachHandler();
        }

        return base_type::Invoke(sender, args );
    }

private:
    WeakRefPtr m_targetWeakReference;
};

// tokenless_event_handler, unlike token_event_handler (and event_handler) doesn't keep a
// EventRegistrationToken
template <class TINTERFACE, class TSENDER, class TARGS, class TEVENTTRAITS>
class tokenless_event_handler final :
    public event_handler_base<TINTERFACE, TSENDER, TARGS, TEVENTTRAITS>,
    public Internal::EventPtr_compatible_tag
{
private:
    _Check_return_ HRESULT AttachHandler(_In_ typename TEVENTTRAITS::event_interface *pSource)
    {
        IFC_RETURN(TEVENTTRAITS::attach_handler(pSource, this));
        return S_OK;
    }

protected:
    _Check_return_ HRESULT RemoveEventHandler(_In_ IInspectable *pSource) final
    {
        HRESULT hr = S_OK;
        typename TEVENTTRAITS::event_interface *pEventSource = NULL;

        IFC(ctl::do_query_interface(pEventSource, pSource));
        VERIFYHR(TEVENTTRAITS::detach_handler(pEventSource, this));

    Cleanup:
        ReleaseInterface(pEventSource);
        RRETURN(hr);
    }

    template <typename T>
    friend class EventPtrBase;
};

template <typename TEVENTHANDLER>
class EventPtrBase
{
public:
    EventPtrBase(): m_pHandler(nullptr)
    { }

    ~EventPtrBase()
    {
        DetachEvent();
    }

    // Allow this class to be moveable. Much less restrictive than being copyable. For instance, this allows them to be
    // stored in a std::vector
    EventPtrBase(_Inout_ EventPtrBase&& other)
        : m_pHandler(other.m_pHandler)
    {
        other.m_pHandler = nullptr;
    }

    EventPtrBase& operator=(_Inout_ EventPtrBase&& other)
    {
        if (this != &other)
        {
            DetachEvent();
            m_pHandler = other.m_pHandler;
            other.m_pHandler = nullptr;
        }
        return *this;
    }

protected:
    // Keep the TEVENTHANDLER and call AttachHandler on it.  The handler is initialized by the caller (subclass).
    _Check_return_ HRESULT AttachEventHandlerInternal(typename TEVENTHANDLER::TSOURCE *pSource, TEVENTHANDLER *handler)
    {
        HRESULT hr = S_OK;

        ASSERT(!m_pHandler);
        m_pHandler = handler;

        IFC(m_pHandler->AttachHandler(pSource));

    Cleanup:
        if (FAILED(hr))
        {
            ctl::release_interface(m_pHandler);
        }
        RRETURN(hr);
    }

public:
    // Clear the m_pHandler
    _Check_return_ HRESULT DetachEventHandler(_In_ IInspectable *pSource)
    {
        HRESULT hr = S_OK;

        if (m_pHandler)
        {
            IFC(m_pHandler->DisconnectHandler(pSource));
        }

    Cleanup:
        ctl::release_interface(m_pHandler);
        RRETURN(hr);
    }

    operator bool() const
    {
        return m_pHandler != nullptr;
    }

private:
    void DetachEvent()
    {
        if (m_pHandler)
        {
            // We don't have access to the source, all we can do
            // is neuter the handler
            m_pHandler->DetachHandler();
            ctl::release_interface(m_pHandler);
        }
    }

    // Disable copying for this class
    EventPtrBase(const EventPtrBase&);
    EventPtrBase& operator =(const EventPtrBase&);

    // Disable moving for this class
    // Nothing to do, no automatic move methods generated

protected:
    TEVENTHANDLER* m_pHandler;
};

// Similar to a ComPtr, points to something in the event_handler family.
template <typename TEVENTHANDLER>
class EventPtr : public EventPtrBase<TEVENTHANDLER>
{
public:
    EventPtr() { }

    static_assert(IsEventPtrCompatible<TEVENTHANDLER>::value, "The template EventPtr can only be used with (tokenless_)event_handler, for weak_event_handler use WeakEventPtr");

    _Check_return_ HRESULT AttachEventHandler(typename TEVENTHANDLER::TSOURCE *pSource, typename TEVENTHANDLER::HandlerFunction handler)
    {
        auto eventHandler = new TEVENTHANDLER();
        IFC_RETURN(eventHandler->Initialize(handler));
        IFC_RETURN(EventPtrBase<TEVENTHANDLER>::AttachEventHandlerInternal( pSource, eventHandler ));
        return S_OK;
    }
};

// Similar to an EventPtr, but keep a weak reference on the target, and fail gracefully during Invoke if it's gone.
template <typename TEVENTHANDLER>
class WeakEventPtr : public EventPtrBase<TEVENTHANDLER>
{
    static_assert(IsWeakEventPtrCompatible<TEVENTHANDLER>::value, "The template WeakEventPtr can only be used with weak_event_handler, for event_handler use EventPtr");

public:
    _Check_return_ HRESULT AttachEventHandler(
        IInspectable * target,
        typename TEVENTHANDLER::TSOURCE *pSource,
        typename TEVENTHANDLER::HandlerFunction handler)
    {
        auto eventHandler = new TEVENTHANDLER();
        IFC_RETURN(eventHandler->Initialize(target, handler));
        IFC_RETURN(EventPtrBase<TEVENTHANDLER>::AttachEventHandlerInternal( pSource, eventHandler ));
        return S_OK;
    }
};
}
