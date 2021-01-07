// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
using winrt::com_ptr;
using winrt::weak_ref;
using winrt::hstring;
using std::wstring_view;

namespace winrt
{
    template <typename T>
    inline hstring hstring_name_of() noexcept
    {
        return hstring(winrt::name_of<T>());
    }
}

inline winrt::IInspectable& to_winrt(::IInspectable*& instance)
{
    return reinterpret_cast<winrt::IInspectable&>(instance);
}

inline winrt::DependencyProperty InitializeDependencyProperty(
    wstring_view const& propertyNameString,
    wstring_view const& propertyTypeNameString,
    wstring_view const& ownerTypeNameString,
    bool isAttached,
    winrt::IInspectable const& defaultValue,
    winrt::PropertyChangedCallback const& propertyChangedCallback = nullptr)
{
    auto propertyType = winrt::Interop::TypeName();
    propertyType.Name = propertyTypeNameString;
    propertyType.Kind = winrt::Interop::TypeKind::Metadata;

    auto ownerType = winrt::Interop::TypeName();
    ownerType.Name = ownerTypeNameString;
    ownerType.Kind = winrt::Interop::TypeKind::Metadata;

    auto propertyMetadata = winrt::PropertyMetadata(defaultValue, propertyChangedCallback);

    if (isAttached)
    {
        return winrt::DependencyProperty::RegisterAttached(propertyNameString, propertyType, ownerType, propertyMetadata);
    }
    else
    {
        return winrt::DependencyProperty::Register(propertyNameString, propertyType, ownerType, propertyMetadata);
    }
}

// Helper to provide default values and boxing without differences at the call sites
template <typename T, typename Enable = void>
struct ValueHelper
{
    static T GetDefaultValue()
    {
#pragma warning(push)
#pragma warning(disable : 26444) 
        return T{};    
#pragma warning(pop)
    }

    static winrt::IInspectable BoxValueIfNecessary(T const& value)
    {
        return winrt::box_value(value);
    }

    static T CastOrUnbox(winrt::IInspectable const& value)
    {
        return winrt::unbox_value<T>(value);
    }

    static winrt::IInspectable BoxedDefaultValue()
    {
        return BoxValueIfNecessary(GetDefaultValue());
    }
};

template <typename T>
struct ValueHelper<T, std::enable_if_t<std::is_base_of_v<winrt::Windows::Foundation::IUnknown, T>>>
{
    static T GetDefaultValue()
    {
        return T{ nullptr };
    }

    static winrt::IInspectable BoxValueIfNecessary(T const& value)
    {
        return value;
    }

    static T CastOrUnbox(winrt::IInspectable const& value)
    {
        return value.as<T>();
    }

    static winrt::IInspectable BoxedDefaultValue()
    {
        return GetDefaultValue();
    }
};

template <>
struct ValueHelper<winrt::hstring, void>
{
    static winrt::hstring GetDefaultValue()
    {
        return winrt::hstring{ L"" };
    }

    static winrt::IInspectable BoxValueIfNecessary(winrt::hstring const& value)
    {
        return winrt::box_value(value);
    }

    static winrt::hstring CastOrUnbox(winrt::IInspectable const& value)
    {
        return winrt::unbox_value<winrt::hstring>(value);
    }

    static winrt::IInspectable BoxedDefaultValue()
    {
        return winrt::box_value(L"");
    }
};

void SetDefaultStyleKeyWorker(winrt::IControlProtected const& controlProtected, std::wstring_view const& className);

template <typename TThis>
void SetDefaultStyleKey(TThis *pThis)
{
    SetDefaultStyleKeyWorker(*pThis, pThis->GetRuntimeClassName());
}

template<typename WinRTReturn>
WinRTReturn GetTemplateChildT(std::wstring_view const& childName, const winrt::IControlProtected& controlProtected)
{
    winrt::DependencyObject childAsDO(controlProtected.GetTemplateChild(childName));

    if (childAsDO)
    {
        return childAsDO.try_as<WinRTReturn>();
    }
    return nullptr;
}

struct ScopedBatchCompleted_revoker
{
    ScopedBatchCompleted_revoker() noexcept = default;
    ScopedBatchCompleted_revoker(ScopedBatchCompleted_revoker const&) = delete;
    ScopedBatchCompleted_revoker& operator=(ScopedBatchCompleted_revoker const&) = delete;
    ScopedBatchCompleted_revoker(ScopedBatchCompleted_revoker&& other) noexcept
    {
        move_from(other);
    }

    ScopedBatchCompleted_revoker& operator=(ScopedBatchCompleted_revoker&& other) noexcept
    {
        move_from(other);
        return *this;
    }

    ScopedBatchCompleted_revoker(winrt::CompositionScopedBatch const& batch, winrt::event_token token) :
        m_batch(batch),
        m_token(token)
    {}

    ~ScopedBatchCompleted_revoker() noexcept
    {
        revoke();
    }

    void revoke() noexcept
    {
        if (!m_batch)
        {
            return;
        }
        else
        {
            m_batch.Completed(m_token);
        }

        m_batch = nullptr;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_batch);
    }
private:
    void move_from(ScopedBatchCompleted_revoker& other)
    {
        if (this != &other)
        {
            revoke();
            m_batch = other.m_batch;
            m_token = other.m_token;
            other.m_batch = nullptr;
            other.m_token.value = 0;
        }
    }

    winrt::CompositionScopedBatch m_batch{nullptr};
    winrt::event_token m_token{};
};

inline ScopedBatchCompleted_revoker RegisterScopedBatchCompleted(winrt::CompositionScopedBatch const& object, winrt::TypedEventHandler<winrt::IInspectable, winrt::CompositionBatchCompletedEventArgs> const& handler)
{
    const auto value = object.Completed(handler);
    return { object, value };
}

struct PropertyChanged_revoker
{
   PropertyChanged_revoker() noexcept = default;
   PropertyChanged_revoker(PropertyChanged_revoker const&) = delete;
   PropertyChanged_revoker& operator=(PropertyChanged_revoker const&) = delete;
   PropertyChanged_revoker(PropertyChanged_revoker&& other) noexcept
   {
       move_from(other);
   }

   PropertyChanged_revoker& operator=(PropertyChanged_revoker&& other) noexcept
   {
       move_from(other);
       return *this;
   }

   PropertyChanged_revoker(winrt::DependencyObject const& object, const winrt::DependencyProperty&  dp, int64_t token) :
        m_object(object),
        m_property(dp),
        m_token(token)
    {}

    ~PropertyChanged_revoker() noexcept
    {
        revoke();
    }

    void revoke() noexcept
    {
        if (!m_object)
        {
            return;
        }

        if (auto object = m_object.get())
        {
            object.UnregisterPropertyChangedCallback(m_property, m_token);
        }

        m_object = nullptr;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_object);
    }
private:
    void move_from(PropertyChanged_revoker& other)
    {
        if (this != &other)
        {
            revoke();
            m_object = other.m_object;
            m_token = other.m_token;
            m_property = other.m_property;
            other.m_object = nullptr;
            other.m_token = 0;
            other.m_property = nullptr;
        }
    }

    weak_ref<winrt::DependencyObject> m_object;
    winrt::DependencyProperty m_property{ nullptr };
    int64_t m_token{};
};



inline PropertyChanged_revoker RegisterPropertyChanged(winrt::DependencyObject const& object, winrt::DependencyProperty const& dp, winrt::DependencyPropertyChangedCallback const& callback)
{
    const auto value = object.RegisterPropertyChangedCallback(dp, callback);
    return { object, dp, value };
}


inline bool SetFocus(winrt::DependencyObject const& object, winrt::FocusState focusState)
{
    if (object)
    {
        // Use TryFocusAsync if it's available.
        if (auto focusManager5 = winrt::get_activation_factory<winrt::FocusManager, winrt::IFocusManagerStatics>().try_as<winrt::IFocusManagerStatics5>())
        {
            auto result = focusManager5.TryFocusAsync(object, focusState);
            if (result.Status() == winrt::AsyncStatus::Completed)
            {
                return result.GetResults().Succeeded();
            }
            // Operation was async, let's assume it worked.
            return true;
        }

        if (auto control = object.try_as<winrt::Control>())
        {
            return control.Focus(focusState);
        }
        else if (auto hyperlink = object.try_as<winrt::Hyperlink>())
        {
            return hyperlink.Focus(focusState);
        }
        else if (auto contentlink = object.try_as<winrt::ContentLink>())
        {
            return contentlink.Focus(focusState);
        }
        else if (auto webview = object.try_as<winrt::WebView>())
        {
            return webview.Focus(focusState);
        }
    }

    return false;
}

// This type exists for types that in metadata derive from FrameworkElement but internally want to derive from Panel
// to get "protected" Children.
// Using it is just like any other winrt::implementation::FooT type *except* that you must pass an additional parameter
// which is the winrt::Foo type. Most times you will be using ReferenceTracker too so that pattern would look like:
// class YourType : ReferenceTracker<YourType, DeriveFromPanelHelper_base, winrt::YourType>  <--- note the extra winrt::YourType here that's normally not needed.
template <typename D, typename T, typename ... I>
struct __declspec(empty_bases) DeriveFromPanelHelper_base : winrt::Windows::UI::Xaml::Controls::PanelT<D, winrt::default_interface<T>, winrt::composable, I...>
{
    using composable = T;
    using class_type = typename T;

    operator class_type() const noexcept
    {
        return static_cast<winrt::IInspectable>(*this).as<class_type>();
    }

    hstring GetRuntimeClassName() const
    {
        return hstring{ winrt::name_of<T>() };
    }
};

template <typename D, typename T, typename ... I>
struct __declspec(empty_bases)DeriveFromBitmapIconHelper_base : winrt::Windows::UI::Xaml::Controls::BitmapIconT<D, winrt::default_interface<T>, winrt::composable, I...>
{
    using composable = T;
    using class_type = typename T;

    operator class_type() const noexcept
    {
        return static_cast<winrt::IInspectable>(*this).as<class_type>();
    }

    hstring GetRuntimeClassName() const
    {
        return hstring{ winrt::name_of<T>() };
    }
};

template <typename D, typename T, typename ... I>
struct __declspec(empty_bases)DeriveFromPathIconHelper_base : winrt::Windows::UI::Xaml::Controls::PathIconT<D, winrt::default_interface<T>, winrt::composable, I...>
{
    using composable = T;
    using class_type = typename T;

    operator class_type() const noexcept
    {
        return static_cast<winrt::IInspectable>(*this).as<class_type>();
    }

    hstring GetRuntimeClassName() const
    {
        return hstring{ winrt::name_of<T>() };
    }
};

//
// An awaitable object. Completes when the CompleteAwaits() method is called.
// cf. .NET's TaskCompletionSource.
//
struct Awaitable
{
    Awaitable()
    {
        m_signal.attach(::CreateEvent(nullptr, true, false, nullptr));
    }

    // Awaitable contract.
    // Blocks until the awaitable is completed, or error.
    bool await_ready() const noexcept
    {
        return ::WaitForSingleObject(m_signal.get(), 0) == 0;
    }

    // Registers a callback that will be called when the awaitable is completed.
    void await_suspend(std::experimental::coroutine_handle<> resume)
    {
        m_wait.attach(
            winrt::check_pointer(
                ::CreateThreadpoolWait(CoroutineCompletedCallback, resume.address(), nullptr)
            )
        );
        ::SetThreadpoolWait(m_wait.get(), m_signal.get(), nullptr);
    }

    // Called to get the value of the awaitable. 
    // This awaitable has no value, so nothing to do.
    void await_resume() const noexcept { }

protected:
    void CompleteAwaits() const noexcept
    {
        ::SetEvent(m_signal.get());
    }

private:
    static void __stdcall CoroutineCompletedCallback(PTP_CALLBACK_INSTANCE, void* context, PTP_WAIT, TP_WAIT_RESULT) noexcept
    {
        // Resumes anyone waiting on the awaitable.
        std::experimental::coroutine_handle<>::from_address(context)();
    }

    //
    // Describes a PTP_WAIT.
    //
    struct PTP_WAIT_traits
    {
        using type = PTP_WAIT;

        static void close(type value) noexcept
        {
            ::CloseThreadpoolWait(value);
        }

        static constexpr type invalid() noexcept
        {
            return nullptr;
        }
    };
    winrt::handle m_signal;
    winrt::handle_type<PTP_WAIT_traits> m_wait;
};
