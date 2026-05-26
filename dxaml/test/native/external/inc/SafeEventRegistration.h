// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <EventHandlerInfo.h>
#include <wex.common.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    delegate void GenericHandler(Platform::Object^ sender, Platform::Object^ args);

    //
    // Helper class to make it easier to manage event registrations.
    // Attached event handlers are unregistered when the object goes out of scope or
    // you explicitly call the Detach() method.
    //
    // To create a registration object, use the helper macro CreateSafeEventRegistration.
    // You can then attach a handler and target object to it using the Attach() method.
    //
    // Ex:
    //      auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
    //      clickRegistration.Attach(myButton, myClickHandler);
    //
    // To reference the type of a particular SafeEventRegistration, you can use the helper
    // macro SafeEventRegistrationType.
    //
    // Ex:
    //      std::vector<SafeEventRegistrationType(xaml_controls::TextBox, GotFocus)> gotFocusRegistrations;
    //
    //      for (int i = 0; i < 5; ++i)
    //      {
    //          auto gotFocusReg = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
    //          gotFocusReg.Attach(myTextBoxVector[i], myGotFocusHandler);
    //          gotFocusRegistrations.push_back(std::move(gotFocusReg));
    //      }
    //
    template <typename TTargetClass, typename THandler>
    class SafeEventRegistration
    {
    public:
        typedef std::function<::Windows::Foundation::EventRegistrationToken (TTargetClass^, THandler^)> AddFunc;
        typedef std::function<void (TTargetClass^, ::Windows::Foundation::EventRegistrationToken)> RemoveFunc;

    private:

        THandler^ CreateProtectedHandler(THandler^ internalHandler)
        {
            auto handler = ref new THandler([this, internalHandler](Platform::Object^ sender, Platform::Object^ args)
                {
                    if (!m_isDetaching)
                    {
                        auto newHandler = reinterpret_cast<GenericHandler^>(internalHandler);
                        newHandler->Invoke(sender, args);
                    }
                    else
                    {
                        WEX::Logging::Log::Warning(L"Handler is getting invoked but skipped as it is detaching");
                    }
                });
            return handler;
        }

        THandler^ CreateTypeSafeProtectedHandler(THandler^ internalHandler)
        {
            auto handler = ref new THandler([this, internalHandler](EventHandlerInfo<THandler>::Sender^ sender, EventHandlerInfo<THandler>::Args^ args)
                {
                    if (!m_isDetaching)
                    {
                        internalHandler(sender, args);
                    }
                    else
                    {
                        WEX::Logging::Log::Warning(L"Handler is getting invoked but skipped as it is detaching");
                    }
                });
            return handler;
        }

    public:
        SafeEventRegistration(AddFunc addFunc, RemoveFunc removeFunc)
            : m_target(nullptr)
            , m_isDetaching(false)
            , m_addFunc(addFunc)
            , m_removeFunc(removeFunc)
            , m_token()
        {
        }

        SafeEventRegistration(SafeEventRegistration&& other)
        {
            WEX::Common::Throw::If(other.m_isDetaching, E_UNEXPECTED, L"Cannot move while detaching");

            m_addFunc = other.m_addFunc;
            m_removeFunc = other.m_removeFunc;
            m_isDetaching = false;
            if (other.IsAttached())
            {
                Attach(other.m_target, other.m_handler);

                other.Detach();
            }
        }

        ~SafeEventRegistration()
        {
            if (IsAttached())
            {
                Detach();
            }
        }

        SafeEventRegistration& operator=(SafeEventRegistration&& other)
        {
            if (this != &other)
            {
                WEX::Common::Throw::If(other.m_isDetaching, E_UNEXPECTED, L"Cannot move while detaching");

                m_addFunc = other.m_addFunc;
                m_removeFunc = other.m_removeFunc;
                m_isDetaching = false;
                if (other.IsAttached())
                {
                    Attach(other.m_target, other.m_handler);

                    other.Detach();
                }
            }

            return *this;
        }

        void Attach(TTargetClass^ target, THandler^ internalHandler)
        {
            WEX::Common::Throw::If(m_target != nullptr || internalHandler == nullptr, E_INVALIDARG);

            m_handler = internalHandler;
            // CreateProtectedHandler requires reinterpret_cast
            // We can use CreateTypeSafeProtectedHandler, but it requires a foward declaration
            // of all possible events used in out test code
            // See EventHandlerInfo.h
            auto handler = CreateProtectedHandler(m_handler);

            RunOnUIThread([this, target, handler]()
            {
                m_token = m_addFunc(target, handler);
            });

            m_target = target;
        }

        void Attach(TTargetClass^ target, std::function<void()> func)
        {
            WEX::Common::Throw::If(m_target != nullptr || func == nullptr, E_INVALIDARG);

            auto handler = ref new THandler([func](Platform::Object^, Platform::Object^)
                {
                    func();
                });

            Attach(target, handler);
        }

        void Detach()
        {
            WEX::Common::Throw::If(m_target == nullptr, E_INVALIDARG);

            m_isDetaching = true;

            RunOnUIThread([this]()
            {
                m_removeFunc(m_target, m_token);
                m_isDetaching = false;
            });

            m_target = nullptr;
            m_handler = nullptr;
            m_token.Value = 0;
        }

        bool IsAttached() const
        {
            return m_target != nullptr;
        }

    private:
        SafeEventRegistration(const SafeEventRegistration& other) = delete;
        SafeEventRegistration& operator=(SafeEventRegistration& other) = delete;

        // Prevent dynamic allocations because the whole point of this class
        // is to provide a safe way to unregister from events by the object
        // going out of scope and dynamically allocating works around that.
        void * operator new (size_t) = delete;

        AddFunc                                     m_addFunc;
        RemoveFunc                                  m_removeFunc;
        TTargetClass^                               m_target;
        ::Windows::Foundation::EventRegistrationToken m_token;

        // During destruction is critical to prevent any handler queued in the
        // UI thread to execute, doing so may cause memory corruption as the
        // handler will access variables in the stack that are no longer available.
        // To prevent this we set m_isDetaching during destruction
        bool                                        m_isDetaching = false;

        THandler^                                   m_handler = nullptr;

    }; // class SafeEventRegistration

    template <typename T>
    struct __RemoveHat_ {};

    template <typename T>
    struct __RemoveHat_<T^>
    {
        typedef T type_no_hat;
    };

    #define EVENTADDFUNC(targetclass, event) \
        [](targetclass^ target, Microsoft::UI::Xaml::Tests::Common::__RemoveHat_<decltype(targetclass##::event)>::type_no_hat^ handler) -> ::Windows::Foundation::EventRegistrationToken { return target->##event += handler; }

    #define EVENTREMOVEFUNC(targetclass, event) \
        [](targetclass^ target, ::Windows::Foundation::EventRegistrationToken token){ target->##event -= token; }

    #define SafeEventRegistrationType(targetclass, event) \
        Microsoft::UI::Xaml::Tests::Common::SafeEventRegistration<targetclass, Microsoft::UI::Xaml::Tests::Common::__RemoveHat_<decltype(targetclass##::event)>::type_no_hat>

    #define CreateSafeEventRegistration(targetclass, event) \
        Microsoft::UI::Xaml::Tests::Common::_CreateSafeEventRegistration<targetclass, Microsoft::UI::Xaml::Tests::Common::__RemoveHat_<decltype(targetclass##::event)>::type_no_hat>( \
            EVENTADDFUNC(targetclass, event), \
            EVENTREMOVEFUNC(targetclass, event))

    template <typename TTargetClass, typename THandler>
    SafeEventRegistration<TTargetClass, THandler>
    _CreateSafeEventRegistration(
        typename SafeEventRegistration<TTargetClass, THandler>::AddFunc addFunc,
        typename SafeEventRegistration<TTargetClass, THandler>::RemoveFunc removeFunc
        )
    {
        return SafeEventRegistration<TTargetClass, THandler>(addFunc, removeFunc);
    }

    //
    // Helper class to make it easier to manage event registrations, even when an event has
    // been handled.
    // Attached event handlers are unregistered when the object goes out of scope or
    // you explicitly call the Detach() method.
    //
    // To create a registration object, use the helper macro CreateSafeEventRegistrationForHandledEvents.
    // You can then attach a handler and target object to it using the Attach() method.
    //
    // Ex:
    //      auto keyDownRegistration = CreateSafeEventRegistrationForHandledEvents(Microsoft::UI::Xaml::UIElement, KeyDownEvent);
    //      keyDownRegistration.Attach(myButton, myKeyDownHandler);
    //
    //
    template <typename TTargetClass>
    class SafeEventRegistrationForHandledEvents
    {
    public:
        typedef std::function<void(TTargetClass^, Platform::Object^)> AddFunc;
        typedef std::function<void(TTargetClass^, Platform::Object^)> RemoveFunc;

    public:
        SafeEventRegistrationForHandledEvents(AddFunc addFunc, RemoveFunc removeFunc)
            : m_target(nullptr)
            , m_addFunc(addFunc)
            , m_removeFunc(removeFunc)
            , m_handler()
        {
        }

        SafeEventRegistrationForHandledEvents(SafeEventRegistrationForHandledEvents&& other)
        {
            m_target = other.m_target;
            m_addFunc = other.m_addFunc;
            m_removeFunc = other.m_removeFunc;
            m_handler = other.m_handler;

            other.m_target = nullptr;
            other.m_addFunc = nullptr;
            other.m_removeFunc = nullptr;
            other.m_handler = nullptr;
        }

        ~SafeEventRegistrationForHandledEvents()
        {
            if (IsAttached())
            {
                Detach();
            }
        }

        SafeEventRegistrationForHandledEvents& operator=(SafeEventRegistrationForHandledEvents&& other)
        {
            if (this != &other)
            {
                m_target = other.m_target;
                m_addFunc = other.m_addFunc;
                m_removeFunc = other.m_removeFunc;
                m_handler = other.m_handler;

                other.m_target = nullptr;
                other.m_addFunc = nullptr;
                other.m_removeFunc = nullptr;
                other.m_handler = nullptr;
            }

            return *this;
        }

        void Attach(TTargetClass^ target, Platform::Object^ handler)
        {
            WEX::Common::Throw::If(m_target != nullptr || handler == nullptr, E_INVALIDARG);

            m_target = target;
            m_handler = handler;

            RunOnUIThread([this, handler]()
            {
                m_addFunc(m_target, m_handler);
            });
        }

        void Detach()
        {
            WEX::Common::Throw::If(m_target == nullptr, E_INVALIDARG);

            RunOnUIThread([this]()
            {
                m_removeFunc(m_target, m_handler);
            });

            m_target = nullptr;
            m_handler = nullptr;
        }

        bool IsAttached() const
        {
            return m_target != nullptr;
        }

    private:
        SafeEventRegistrationForHandledEvents(const SafeEventRegistrationForHandledEvents& other);
        SafeEventRegistrationForHandledEvents& operator=(SafeEventRegistrationForHandledEvents& other);

        // Prevent dynamic allocations because the whole point of this class
        // is to provide a safe way to unregister from events by the object
        // going out of scope and dynamically allocating works around that.
        void * operator new (size_t);

        AddFunc                                     m_addFunc;
        RemoveFunc                                  m_removeFunc;
        TTargetClass^                               m_target;
        Platform::Object^                           m_handler;

    }; // class SafeEventRegistrationForHandledEvents



#define HANDLEREVENTADDFUNC(targetclass, event) \
        [](targetclass^ target, Platform::Object^ handler) { return target->AddHandler(targetclass::##event, handler, true/*handledEventsToo*/); }

#define HANDLEREVENTREMOVEFUNC(targetclass, event) \
        [](targetclass^ target, Platform::Object^ handler){ target->RemoveHandler(targetclass::##event, handler); }

#define SafeEventRegistrationForHandledEventsType(targetclass, event) \
        Microsoft::UI::Xaml::Tests::Common::SafeEventRegistrationForHandledEvents<targetclass, Platform::Object^>

#define CreateSafeEventRegistrationForHandledEvents(targetclass, event) \
        Microsoft::UI::Xaml::Tests::Common::_CreateSafeEventRegistrationForHandledEvents<targetclass>( \
            HANDLEREVENTADDFUNC(targetclass, event), \
            HANDLEREVENTREMOVEFUNC(targetclass, event))

    template <typename TTargetClass>
    SafeEventRegistrationForHandledEvents<TTargetClass>
        _CreateSafeEventRegistrationForHandledEvents(
        typename SafeEventRegistrationForHandledEvents<TTargetClass>::AddFunc addFunc,
        typename SafeEventRegistrationForHandledEvents<TTargetClass>::RemoveFunc removeFunc
        )
    {
        return SafeEventRegistrationForHandledEvents<TTargetClass>(addFunc, removeFunc);
    }

} } } } } // namespace Microsoft::UI::Xaml::Tests::Common

