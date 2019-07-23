// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "tracker_ref.h"
#include "DispatcherHelper.h"


// This type is a helper to make ReferenceTracker work with winrt::implements intead of a concrete implementation type
template <typename D, typename WinRTClassType, template <typename, typename ...> class ImplT, typename ... I>
struct reference_tracker_implements : 
    public ImplT<D, WinRTClassType, I ..., winrt::composable, winrt::composing>
{
    using class_type = typename WinRTClassType;

    reference_tracker_implements()
    {
    }
};

template <typename WinRTClassType>
struct reference_tracker_implements_t
{
    template <typename D, typename ... I>
    using type = reference_tracker_implements<D, WinRTClassType, winrt::implements, I ...>;
};

template <typename WinRTClassType, template <typename, typename ...> class ImplT>
struct reference_tracker_custom_implements_t
{
    template <typename D, typename ... I>
    using type = reference_tracker_implements<D, WinRTClassType, ImplT, I ...>;
};

template <typename D, template <typename, typename ...> class ImplT, typename ... I>
struct ReferenceTracker : public ImplT<D, I ..., ::IReferenceTrackerExtension>, public ITrackerHandleManager
{
    using impl_type = typename ImplT<D, I ..., ::IReferenceTrackerExtension>;

    template <typename... Args>
    ReferenceTracker(Args&&... args) : impl_type(std::forward<Args>(args) ...)
    {
        m_owningThreadId = ::GetCurrentThreadId();

        EnsureReferenceTrackerInterfaces();
    }

    void abi_enter()
    {
        CheckThread();
    }


    bool IsOnThread()
    {
        return ::GetCurrentThreadId() == m_owningThreadId;
    }

    void CheckThread()
    {
        if (!IsOnThread())
        {
            throw winrt::hresult_wrong_thread();
        }
    }

    template <typename BaseType>
    auto GetBase()
    {
        return m_inner.as<BaseType>();
    }

    // Generally callers should use This(), but inner is a non-delegating inspectable so
    // it is necessary to use for calling through to base methods in Override situations.
    winrt::IInspectable GetInner()
    {
        return m_inner;
    }

    HRESULT __stdcall NonDelegatingQueryInterface(GUID const& riid, void** value) noexcept
    {
        // In order for the reference tracking mechanism to work, we actually need to hand out XAML's
        // implementation of IWeakReferenceSource. However there are some bugs on RS2 where XAML calls
        // back out to the outer during initialization for IWeakReferenceSource and so our m_inner is null.
        // In that case we allow our "self" implementation to leak out (if we returned null, XAML would crash).
        if (InlineIsEqualGUID(riid, __uuidof(::IWeakReferenceSource)) && m_inner)
        {
            return winrt::get_unknown(m_inner)->QueryInterface(riid, value);
        }
        else
        {
            // We only implement IReferenceTrackerExtension if we're not composed (subclassed)
            // by another class; this interface has to be implemented by the controlling unknown,
            // because that's the object that controls the ref count.  Implementing this interface
            // communicates that you return valid ref counts from Release().
            if (InlineIsEqualGUID(riid, __uuidof(::IReferenceTrackerExtension)) && !outer())
            {
                *value = static_cast<::IReferenceTrackerExtension*>(this);
                static_cast<IUnknown*>(*value)->AddRef();
            }
            else
            {               
                return impl_type::NonDelegatingQueryInterface(riid, value);
            }
        }

        return S_OK;
    }

    // TODO: Remove once CppWinRT always calls shim for NonDelegatingAddRef/Release

    // TEMP-BEGIN

    HRESULT __stdcall QueryInterface(GUID const& id, void** object) noexcept
    {
        if (this->outer())
        {
            return this->outer()->QueryInterface(id, object);
        }

        return NonDelegatingQueryInterface(id, object);
    }

    // TEMP-END    

    static void final_release(std::unique_ptr<D>&& self)
    {
        DeleteInstanceOnUIThread(std::move(self));
    }

    // Post a call to DeleteInstance() to the UI thread.  If we're already on the UI thread, then just
    // return false.  If we're off the UI thread but can't get to it, then do the DeleteInstance() here (asynchronously).
    static void DeleteInstanceOnUIThread(std::unique_ptr<D>&& self) noexcept
    {
        bool queued = false;
        
        // See if we're on the UI thread
        if(!self->IsOnThread())
        {
            // We're not on the UI thread
            static_cast<ReferenceTracker<D, ImplT, I...>*>(self.get())->m_dispatcherHelper.RunAsync(
                [instance = self.release()]()
                {
                    delete instance;
                },
                true /*fallbackToThisThread*/);

            queued = true;
        }
        

        if (!queued)
        {
            self.reset();
        }
    }

    void EnsureReferenceTrackerInterfaces()
    {
        // Should be called once after the object is initialized.
#if _DEBUG
        MUX_ASSERT_NOASSUME(!m_wasEnsureCalled);
#endif
        if (!m_inner) // We need to derive from DependencyObject. Do so if it didn't happen yet.
        {
            // Internally derive from DependencyObject to get ReferenceTracker behavior.
            winrt::get_activation_factory<winrt::DependencyObject, winrt::IDependencyObjectFactory>().CreateInstance(*this, this->m_inner);
        }
        if (m_inner)
        {
            if (auto trackerOwnerInner = m_inner.try_as<::ITrackerOwner>()) // Only exists on RS2+
            {
                m_trackerOwnerInnerNoRef = static_cast<::ITrackerOwner*>(winrt::get_abi(trackerOwnerInner));
            }
        }

#if _DEBUG
        m_wasEnsureCalled = true;
#endif
    }

    DispatcherHelper m_dispatcherHelper;

private:
    DWORD m_owningThreadId{};
};

template<typename Factory>
inline HRESULT STDMETHODCALLTYPE CppWinRTCreateActivationFactory(_In_ unsigned int *flags, _In_ const ::Microsoft::WRL::Details::CreatorMap* entry, REFIID riid, _Outptr_ IUnknown **ppFactory) throw()
{
    try
    {
        auto activationFactory = winrt::make<Factory>();

        *flags |= DisableCaching;

        winrt::check_hresult(winrt::get_unknown(activationFactory)->QueryInterface(riid, (void**)ppFactory));
        __analysis_assume(*ppFactory);

        CATCH_RETURN;
    }
}

#define CppWinRTInternalWrlCreateCreatorMap(className, runtimeClassName, trustLevel, creatorFunction, section) \
    __declspec(selectany) ::Microsoft::WRL::Details::FactoryCache __objectFactory__##className = { nullptr, 0 }; \
    extern __declspec(selectany) const ::Microsoft::WRL::Details::CreatorMap __object_##className = { \
        creatorFunction, \
        runtimeClassName, \
        trustLevel, \
        &__objectFactory__##className,\
        nullptr}; \
    extern "C" __declspec(allocate(section)) __declspec(selectany) const ::Microsoft::WRL::Details::CreatorMap* const __minATLObjMap_##className = &__object_##className; \
    WrlCreatorMapIncludePragma(className)

namespace CppWinRTTemp
{
    static TrustLevel __stdcall GetTrustLevel_BaseTrust() { return BaseTrust;  }
}

#define CppWinRTActivatableClassWithFactory(className, factory) \
    namespace CppWinRTTemp { static auto __stdcall RuntimeClassName__##className() { return winrt::name_of<className::class_type>().data(); } } \
    CppWinRTInternalWrlCreateCreatorMap(\
        className, \
        reinterpret_cast<const IID*>(&CppWinRTTemp::RuntimeClassName__##className), \
        &CppWinRTTemp::GetTrustLevel_BaseTrust, \
        CppWinRTCreateActivationFactory<factory>, \
        "minATL$__r")

#define CppWinRTActivatableClass(className) \
    CppWinRTActivatableClassWithFactory(className, className##Factory)

#define CppWinRTActivatableClassWithBasicFactory(className) \
    struct className##Factory : public winrt::factory_implementation::className##T<className##Factory, className> {}; \
    CppWinRTActivatableClassWithFactory(className, className##Factory)

#define CppWinRTActivatableClassWithDPFactory(className) \
    struct className##Factory : public winrt::factory_implementation::className##T<className##Factory, className> \
    { \
        className##Factory() { EnsureProperties(); } \
        static void ClearProperties() { className::ClearProperties(); }\
        static void EnsureProperties() { className::EnsureProperties(); }\
    }; \
    CppWinRTActivatableClassWithFactory(className, className##Factory)

// A workaround for BUG 17986411
// If we defined ReferenceTracker on a base class, then subclass of it should ForwardRefToBaseReferenceTracker
// For example, we need to ForwardRefToBaseReferenceTracker(NavigationViewItemBase) in NavigationViewItem
//      class NavigationViewItem :  public winrt::implementation::NavigationViewItemT<NavigationViewItem, NavigationViewItemBase>
//      class NavigationViewItemBase : public ReferenceTracker<NavigationViewItemBase, winrt::implementation::NavigationViewItemBaseT, winrt::composable>
#define ForwardRefToBaseReferenceTracker(baseClass) \
    HRESULT __stdcall QueryInterface(GUID const& id, void** object) noexcept override \
    { \
        return baseClass##::QueryInterface(id, object); \
    } \
