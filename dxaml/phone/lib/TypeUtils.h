// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    //------------------------------------------------------------------------------
    // A helper function used when dealing with time intervals in XAML.
    // Enables STL durations to be used as WinRT Duration type.
    //------------------------------------------------------------------------------
    template <
        typename Rep,
        typename Ratio>
    xaml::Duration GetDuration(std::chrono::duration<Rep, Ratio> const& duration)
    {
        // xaml time is measured in 100nano intervals:
        typedef std::chrono::duration<
            std::intmax_t,
            std::ratio<100 * std::nano::num, std::nano::den>
        > baseline;

        xaml::Duration xamlDuration =
        {
            std::chrono::duration_cast<baseline>(duration).count(), /* TimeSpan */
            xaml::DurationType::DurationType_TimeSpan /* DurationType */
        };

        return xamlDuration;
    }

    _Check_return_ static HRESULT
        GetCurrentWindow(_Outptr_ xaml::IWindow** ppWindowOut)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IWindow> spCurrentWindow;
        wrl::ComPtr<xaml::IWindowStatics> spWindowStatics;

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Window).Get(),
            &spWindowStatics));
        IFC(spWindowStatics->get_Current(&spCurrentWindow));
        IFC(spCurrentWindow.CopyTo(ppWindowOut));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ static HRESULT
        GetCurrentWindowBounds(_Out_ wf::Rect* pBoundsOut)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IWindow> spCurrentWindow;

        IFC(Private::GetCurrentWindow(spCurrentWindow.GetAddressOf()));
        IFC(spCurrentWindow->get_Bounds(pBoundsOut));

    Cleanup:
        RRETURN(hr);
    }

    //
    // AddRef/Release helpers. will increment/decrement refcounts for everything
    // that has those methods defined. This will reduce the number of compilation
    // errors caused by error C4540, where it really doesn't matter which IInspectable
    // we're incrementing within one inheritance graph. For other types, they're noops.
    //

    template <typename Type>
    static auto TryIncrementRefCount(Type t)
        -> decltype(std::declval<Type>()->AddRef(), void())
    {
        t->AddRef();
    }

    template <typename Type>
    static auto TryRelease(Type t)
        -> decltype(std::declval<Type>()->Release(), void())
    {
        t->Release();
    }

    static void TryIncrementRefCount(...) {} // noop
    static void TryRelease(...) {} // noop
}
