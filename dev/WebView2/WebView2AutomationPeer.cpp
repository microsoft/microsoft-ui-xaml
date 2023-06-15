// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "WebView2AutomationPeer.h"

#include <uiautomationclient.h>

#pragma warning( disable : 26496 26462 26461 6387 28196)

WebView2AutomationPeer::WebView2AutomationPeer(winrt::WebView2 const& owner)
    : ReferenceTracker(owner)
{}

com_ptr<WebView2> WebView2AutomationPeer::GetImpl()
{
    com_ptr<WebView2> impl;
    winrt::WebView2 webview = Owner().try_as<winrt::WebView2>();

    if (webview)
    {
        impl = winrt::get_self<WebView2>(webview)->get_strong();
    }

    return impl;
}

hstring WebView2AutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::WebView2>();
}

winrt::AutomationControlType WebView2AutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}

winrt::Rect WebView2AutomationPeer::GetBoundingRectangleCore()
{
    winrt::Rect boundingRect = GetImpl()->GetBoundingRectangle();
    return boundingRect;
}

HRESULT WebView2AutomationPeer::GetRawElementProviderSimple(_Outptr_opt_ IRawElementProviderSimple** value)
{
    if (value == nullptr) return S_OK;
    
    *value = nullptr;

    InitProvider();

    auto wrapperSimple = m_providerWrapper.try_as<IRawElementProviderSimple>();
    if (wrapperSimple)
    {
        wrapperSimple.copy_to(value);
    }

    return S_OK;
}

HRESULT WebView2AutomationPeer::IsCorrectPeerForHwnd(HWND hwnd, _Out_ bool* value)
{
    *value = false;

    if (!InitProvider())
    {
        return S_OK;
    }

    auto hwndProvider = GetImpl()->GetProviderForHwnd(hwnd).try_as<IRawElementProviderSimple>();
    if (hwndProvider && hwndProvider == m_provider)
    {
        *value = true;
    }

    return S_OK;
}

struct ProviderWrapper : winrt::implements<ProviderWrapper, winrt::IInspectable, ::IRawElementProviderSimple, ::IRawElementProviderSimple2, ::IRawElementProviderFragment, ::IRawElementProviderFragmentRoot>
{
public:
    ProviderWrapper(winrt::com_ptr<IRawElementProviderSimple> const& child, winrt::FrameworkElementAutomationPeer parent) : m_child(child), m_parent(parent)
    {
    }

    HRESULT STDMETHODCALLTYPE get_ProviderOptions(
        __RPC__out enum ProviderOptions* pRetVal) noexcept override
    {
        *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading
            | ProviderOptions_OverrideProvider;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPatternProvider(
        PATTERNID patternId,
        __RPC__deref_out_opt IUnknown** pRetVal) noexcept override
    {
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PROPERTYID propertyId,
        __RPC__out VARIANT* pRetVal) noexcept override
    {
        if (propertyId == UIA_IsControlElementPropertyId) {
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
        }
        else if (propertyId == UIA_IsContentElementPropertyId) {
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
        }
        else {
            return E_NOTIMPL;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(
        __RPC__deref_out_opt IRawElementProviderSimple** pRetVal) noexcept override
    {
        return m_child->get_HostRawElementProvider(pRetVal);
    }

    HRESULT STDMETHODCALLTYPE ShowContextMenu(void) noexcept override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Navigate(
        /* [in] */ enum NavigateDirection direction,
        /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragment** pRetVal) noexcept override
    {
        if (direction == NavigateDirection_Parent)
        {
            *pRetVal = m_parent.as<::IRawElementProviderFragment>().detach();
        }
        else
        {
            *pRetVal = nullptr;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetRuntimeId(
        /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal) noexcept override
    {
        *pRetVal = nullptr;

        SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 2);
        if (psa == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        auto id = winrt::AutomationPeer::GenerateRawElementProviderRuntimeId();

        LONG index = 0;
        VARIANT data;
        data.vt = VT_I4;
        data.intVal = id.Part1;
        winrt::check_hresult(SafeArrayPutElement(psa, &index, &data));

        index++;
        data.vt = VT_I4;
        data.intVal = id.Part2;
        winrt::check_hresult(SafeArrayPutElement(psa, &index, &data));

        *pRetVal = psa;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(
        /* [retval][out] */ __RPC__out struct UiaRect* pRetVal) noexcept override
    {
        auto bounds = m_parent.GetBoundingRectangle();
        pRetVal->left = bounds.X;
        pRetVal->top = bounds.Y;
        pRetVal->width = bounds.Width;
        pRetVal->height = bounds.Height;

        // XAML's bounding rectangle is in client coordinates. Translate to screen.
        auto scale = winrt::DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();
        auto windowBounds = winrt::Window::Current().Bounds();
        pRetVal->left += windowBounds.X * scale;
        pRetVal->top += windowBounds.Y * scale;
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(
        /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal) noexcept override
    {
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetFocus(void) override
    {
        return S_OK; 
    }

    /* [propget] */ HRESULT STDMETHODCALLTYPE get_FragmentRoot(
        /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragmentRoot** pRetVal) noexcept override
    {
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(
        /* [in] */ double x,
        /* [in] */ double y,
        /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragment** pRetVal) noexcept override
    {
        *pRetVal = get_strong().detach();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFocus(
        /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragment** pRetVal) noexcept override
    {
            // We don't answer the question about a current focus. We return NULL here since if we return
            // an error UIA will bail out the entire focus operation rather than continueing its search.
            *pRetVal = nullptr;
            return S_OK;
    }

private:
    winrt::com_ptr<IRawElementProviderSimple> m_child;
    winrt::FrameworkElementAutomationPeer m_parent;
};

winrt::IInspectable WebView2AutomationPeer::GetFocusedElementCore()
{
    InitProvider();

    return m_providerWrapper;
}

winrt::IInspectable WebView2AutomationPeer::NavigateCore(winrt::AutomationNavigationDirection direction)
{
    if (direction == winrt::AutomationNavigationDirection::FirstChild || direction == winrt::AutomationNavigationDirection::LastChild)
    {
        InitProvider();

        return m_providerWrapper;
    }
    else
    {
        return base_type::NavigateCore(direction);
    }
}

winrt::IInspectable WebView2AutomationPeer::GetElementFromPointCore(winrt::Point pointInWindowCoordinates)
{
    InitProvider();

    return m_providerWrapper;
}

bool WebView2AutomationPeer::InitProvider()
{
    if (!m_provider)
    {
        m_provider = GetImpl()->GetWebView2Provider().try_as<IRawElementProviderSimple>();
        if (m_provider)
        {
            com_ptr<IRawElementProviderSimple> spHost;
            m_provider->get_HostRawElementProvider(spHost.put());

            auto spFragRoot = spHost.try_as< IRawElementProviderFragmentRoot>();

            m_providerWrapper = *winrt::make_self<ProviderWrapper>(m_provider, *this);
        }
    }
    return !!m_provider;
}
