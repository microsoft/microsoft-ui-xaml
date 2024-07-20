// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft::UI::Xaml::Hosting {

class NavigationFocusEventArgs : public ::Microsoft::WRL::RuntimeClass<
    ::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::WinRtClassicComMix>,
    xaml_hosting::IDesktopWindowXamlSourceTakeFocusRequestedEventArgs>
{
protected:
    NavigationFocusEventArgs(_In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request)
        : m_Request(request)
    {
    }

public:
    IFACEMETHOD(get_Request)(_Outptr_ xaml_hosting::IXamlSourceFocusNavigationRequest** result) override
    {
        IFC_RETURN(m_Request.CopyTo(result));
        return S_OK;
    }
    
private:
    wrl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> m_Request;
};

class NavigationLosingFocusEventArgs final : public NavigationFocusEventArgs
{
    InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Hosting_DesktopWindowXamlSourceTakeFocusRequestedEventArgs, TrustLevel::BaseTrust);
public:
    NavigationLosingFocusEventArgs(_In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request)
        : NavigationFocusEventArgs(request)
    {
    }
};

class NavigationGotFocusEventArgs final : public NavigationFocusEventArgs
{
    InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Hosting_DesktopWindowXamlSourceGotFocusEventArgs, TrustLevel::BaseTrust);
public:
    NavigationGotFocusEventArgs(_In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request)
        : NavigationFocusEventArgs(request)
    {
    }
};

} XAML_ABI_NAMESPACE_END
