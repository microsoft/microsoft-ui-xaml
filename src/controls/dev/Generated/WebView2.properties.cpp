// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "..\WebView2\WebView2.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithDPFactory(WebView2)
}

#include "WebView2.g.cpp"

GlobalDependencyProperty WebView2Properties::s_CanGoBackProperty{ nullptr };
GlobalDependencyProperty WebView2Properties::s_CanGoForwardProperty{ nullptr };
GlobalDependencyProperty WebView2Properties::s_DefaultBackgroundColorProperty{ nullptr };
GlobalDependencyProperty WebView2Properties::s_SourceProperty{ nullptr };

WebView2Properties::WebView2Properties()
    : m_coreProcessFailedEventSource{static_cast<WebView2*>(this)}
    , m_coreWebView2InitializedEventSource{static_cast<WebView2*>(this)}
    , m_navigationCompletedEventSource{static_cast<WebView2*>(this)}
    , m_navigationStartingEventSource{static_cast<WebView2*>(this)}
    , m_webMessageReceivedEventSource{static_cast<WebView2*>(this)}
{
    EnsureProperties();
}

void WebView2Properties::EnsureProperties()
{
    if (!s_CanGoBackProperty)
    {
        s_CanGoBackProperty =
            InitializeDependencyProperty(
                L"CanGoBack",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::WebView2>(),
                false /* isAttached */,
                ValueHelper<bool>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnCanGoBackPropertyChanged));
    }
    if (!s_CanGoForwardProperty)
    {
        s_CanGoForwardProperty =
            InitializeDependencyProperty(
                L"CanGoForward",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::WebView2>(),
                false /* isAttached */,
                ValueHelper<bool>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnCanGoForwardPropertyChanged));
    }
    if (!s_DefaultBackgroundColorProperty)
    {
        s_DefaultBackgroundColorProperty =
            InitializeDependencyProperty(
                L"DefaultBackgroundColor",
                winrt::name_of<winrt::Color>(),
                winrt::name_of<winrt::WebView2>(),
                false /* isAttached */,
                ValueHelper<winrt::Color>::BoxValueIfNecessary(WebView2::sc_controllerDefaultBackgroundColor),
                winrt::PropertyChangedCallback(&OnDefaultBackgroundColorPropertyChanged));
    }
    if (!s_SourceProperty)
    {
        s_SourceProperty =
            InitializeDependencyProperty(
                L"Source",
                winrt::name_of<winrt::Uri>(),
                winrt::name_of<winrt::WebView2>(),
                false /* isAttached */,
                ValueHelper<winrt::Uri>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnSourcePropertyChanged));
    }
}

void WebView2Properties::ClearProperties()
{
    s_CanGoBackProperty = nullptr;
    s_CanGoForwardProperty = nullptr;
    s_DefaultBackgroundColorProperty = nullptr;
    s_SourceProperty = nullptr;
}

void WebView2Properties::OnCanGoBackPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::WebView2>();
    winrt::get_self<WebView2>(owner)->OnPropertyChanged(args);
}

void WebView2Properties::OnCanGoForwardPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::WebView2>();
    winrt::get_self<WebView2>(owner)->OnPropertyChanged(args);
}

void WebView2Properties::OnDefaultBackgroundColorPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::WebView2>();
    winrt::get_self<WebView2>(owner)->OnPropertyChanged(args);
}

void WebView2Properties::OnSourcePropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::WebView2>();
    winrt::get_self<WebView2>(owner)->OnPropertyChanged(args);
}

void WebView2Properties::CanGoBack(bool value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<WebView2*>(this)->SetValue(s_CanGoBackProperty, ValueHelper<bool>::BoxValueIfNecessary(value));
    }
}

bool WebView2Properties::CanGoBack()
{
    return ValueHelper<bool>::CastOrUnbox(static_cast<WebView2*>(this)->GetValue(s_CanGoBackProperty));
}

void WebView2Properties::CanGoForward(bool value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<WebView2*>(this)->SetValue(s_CanGoForwardProperty, ValueHelper<bool>::BoxValueIfNecessary(value));
    }
}

bool WebView2Properties::CanGoForward()
{
    return ValueHelper<bool>::CastOrUnbox(static_cast<WebView2*>(this)->GetValue(s_CanGoForwardProperty));
}

void WebView2Properties::DefaultBackgroundColor(winrt::Color const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<WebView2*>(this)->SetValue(s_DefaultBackgroundColorProperty, ValueHelper<winrt::Color>::BoxValueIfNecessary(value));
    }
}

winrt::Color WebView2Properties::DefaultBackgroundColor()
{
    return ValueHelper<winrt::Color>::CastOrUnbox(static_cast<WebView2*>(this)->GetValue(s_DefaultBackgroundColorProperty));
}

void WebView2Properties::Source(winrt::Uri const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<WebView2*>(this)->SetValue(s_SourceProperty, ValueHelper<winrt::Uri>::BoxValueIfNecessary(value));
    }
}

winrt::Uri WebView2Properties::Source()
{
    return ValueHelper<winrt::Uri>::CastOrUnbox(static_cast<WebView2*>(this)->GetValue(s_SourceProperty));
}

winrt::event_token WebView2Properties::CoreProcessFailed(winrt::TypedEventHandler<winrt::WebView2, winrt::CoreWebView2ProcessFailedEventArgs> const& value)
{
    return m_coreProcessFailedEventSource.add(value);
}

void WebView2Properties::CoreProcessFailed(winrt::event_token const& token)
{
    m_coreProcessFailedEventSource.remove(token);
}

winrt::event_token WebView2Properties::CoreWebView2Initialized(winrt::TypedEventHandler<winrt::WebView2, winrt::CoreWebView2InitializedEventArgs> const& value)
{
    return m_coreWebView2InitializedEventSource.add(value);
}

void WebView2Properties::CoreWebView2Initialized(winrt::event_token const& token)
{
    m_coreWebView2InitializedEventSource.remove(token);
}

winrt::event_token WebView2Properties::NavigationCompleted(winrt::TypedEventHandler<winrt::WebView2, winrt::CoreWebView2NavigationCompletedEventArgs> const& value)
{
    return m_navigationCompletedEventSource.add(value);
}

void WebView2Properties::NavigationCompleted(winrt::event_token const& token)
{
    m_navigationCompletedEventSource.remove(token);
}

winrt::event_token WebView2Properties::NavigationStarting(winrt::TypedEventHandler<winrt::WebView2, winrt::CoreWebView2NavigationStartingEventArgs> const& value)
{
    return m_navigationStartingEventSource.add(value);
}

void WebView2Properties::NavigationStarting(winrt::event_token const& token)
{
    m_navigationStartingEventSource.remove(token);
}

winrt::event_token WebView2Properties::WebMessageReceived(winrt::TypedEventHandler<winrt::WebView2, winrt::CoreWebView2WebMessageReceivedEventArgs> const& value)
{
    return m_webMessageReceivedEventSource.add(value);
}

void WebView2Properties::WebMessageReceived(winrt::event_token const& token)
{
    m_webMessageReceivedEventSource.remove(token);
}
