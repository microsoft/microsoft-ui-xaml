#include "pch.h"
#include "DocumentInfo.h"
#if __has_include("DocumentInfo.g.cpp")
#include "DocumentInfo.g.cpp"
#endif

namespace winrt
{
    using namespace winrt::Microsoft::UI::Xaml::Data;
}

namespace winrt::TabViewTearOutApp::implementation
{
    void DocumentInfo::Name(winrt::hstring const& value)
    {
        if (value != m_name)
        {
            m_name = value;
            m_propertyChangedEvent(*this, winrt::PropertyChangedEventArgs(L"Name"));
        }
    }

    void DocumentInfo::Text(winrt::hstring const& value)
    {
        if (value != m_text)
        {
            m_text = value;
            m_propertyChangedEvent(*this, winrt::PropertyChangedEventArgs(L"Text"));
        }
    }

    winrt::event_token DocumentInfo::PropertyChanged(winrt::PropertyChangedEventHandler const& value)
    {
        return m_propertyChangedEvent.add(value);
    }

    void DocumentInfo::PropertyChanged(winrt::event_token const& token)
    {
        m_propertyChangedEvent.remove(token);
    }
}
