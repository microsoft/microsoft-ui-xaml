// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    // Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener, declared exactly as in the FrameworkUdk package's
    // BackButtonPressedListener.h (that include path is not part of the controls test project). Controls such as AppBar
    // and ContentDialog implement it; the FrameworkUdk back-button integration calls OnBackButtonPressed on the
    // registered listeners when the system back button is pressed. Controls only register when the host supports the
    // system back button (CCoreServices::BackButtonSupported, false for islands-only hosting), so tests deliver the press
    // directly through this interface.
    MIDL_INTERFACE("9804facf-195d-5ea9-9fc0-218b11aea7eb")
    IBackButtonPressedListener : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnBackButtonPressed(_Out_ ::boolean* result) = 0;
    };

    class BackButtonHelper
    {
    public:
        // Delivers one back-button press to the listener and returns whether it handled the press.
        // Must be called on the UI thread.
        static bool PressBackButton(Platform::Object^ listener)
        {
            wrl::ComPtr<IBackButtonPressedListener> backButtonListener;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(listener)->QueryInterface(IID_PPV_ARGS(&backButtonListener)));
            ::boolean handled = true;
            VERIFY_SUCCEEDED(backButtonListener->OnBackButtonPressed(&handled));
            return !!handled;
        }
    };

} } } } }
