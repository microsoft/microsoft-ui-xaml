// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class PlatformHelpers
    {
    public:
        _Check_return_ static HRESULT LookupThemeResource(_In_ HSTRING themeResourceName, _Outptr_ IInspectable** ppThemeResource);

        _Check_return_ static HRESULT GetKeyboardModifiers(_Out_ wsy::VirtualKeyModifiers* pnKeyboardModifiers);

        _Check_return_ static HRESULT RequestInteractionSoundForElement(_In_ xaml::ElementSoundKind soundToPlay, _In_ xaml::IDependencyObject* element);

        _Check_return_ static HRESULT GetEffectiveSoundMode(_In_ xaml::IDependencyObject* element, _Out_ xaml::ElementSoundMode* soundMode);

        static inline IUnknown* IUnknownCast(_In_opt_ IUnknown* unk)
        {
            return unk;
        }

        template <typename T, typename U>
        static wrl::ComPtr<T> QueryInterfaceCast(_In_ U* in)
        {
            HRESULT hr = S_OK;
            wrl::ComPtr<T> result;

            if (in)
            {
                hr = IUnknownCast(in)->QueryInterface(__uuidof(T), (void **)result.ReleaseAndGetAddressOf());
                if (FAILED(hr))
                {
                    result.Reset();
                }
            }

            return result;
        }

        static bool AreSameObject(_In_ IUnknown* first, _In_ IUnknown* second);

    private:
        _Check_return_ static HRESULT GetCurrentApp();

        static wrl::ComPtr<xaml::IApplication> s_spApp;

};
}}}} XAML_ABI_NAMESPACE_END
