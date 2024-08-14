#pragma once

#include "DocumentInfo.g.h"

namespace winrt::TabViewTearOutApp::implementation
{
    struct DocumentInfo : DocumentInfoT<DocumentInfo>
    {
        DocumentInfo() = default;

        DocumentInfo(winrt::hstring const& name, winrt::hstring const& text)
        {
            m_name = name;
            m_text = text;
        }

        winrt::hstring Name() const { return m_name; }
        void Name(winrt::hstring const& value);
        winrt::hstring Text() const { return m_text; }
        void Text(winrt::hstring const& value);

        // INotifyPropertyChanged
        winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value);
        void PropertyChanged(winrt::event_token const& token);

    private:
        winrt::hstring m_name;
        winrt::hstring m_text;

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChangedEvent;
    };
}

namespace winrt::TabViewTearOutApp::factory_implementation
{
    struct DocumentInfo : DocumentInfoT<DocumentInfo, implementation::DocumentInfo>
    {
    };
}
