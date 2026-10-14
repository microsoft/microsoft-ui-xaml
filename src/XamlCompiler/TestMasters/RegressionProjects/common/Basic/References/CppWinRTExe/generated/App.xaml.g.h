#pragma once
#ifndef XAML_IMPL_MODULE
#include <unknwn.h>
#undef GetCurrentTime
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include "XamlTypeInfo.xaml.g.h"
#include "XamlMetaDataProvider.h"
#endif

#ifdef XAML_IMPL_MODULE
#define XAML_EXPORT export extern "C++"
#else
#define XAML_EXPORT
#endif

XAML_EXPORT namespace winrt::CppWinRTExe::implementation
{
    struct XamlMetaDataProvider;
    template <typename D> struct XamlAppMetadataProvider
    {
        using type = XamlMetaDataProvider;
    };
}

XAML_EXPORT namespace winrt::CppWinRTExe::implementation
{
    template <typename D, typename ... Interfaces>
    struct AppT: public ::winrt::Microsoft::UI::Xaml::ApplicationT<D, ::winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider, Interfaces...>
    {
        using XamlMetaDataProvider = typename ::winrt::CppWinRTExe::implementation::XamlAppMetadataProvider<D>::type;
        using IXamlType = ::winrt::Microsoft::UI::Xaml::Markup::IXamlType;

        void InitializeComponent()
        {
            if (_contentLoaded)
                return;
           
            _contentLoaded = true;

            ::winrt::Windows::Foundation::Uri resourceLocator{ L"ms-appx:///App.xaml" };
            ::winrt::Microsoft::UI::Xaml::Application::LoadComponent(*this, resourceLocator);
        }


        IXamlType GetXamlType(::winrt::Windows::UI::Xaml::Interop::TypeName const& type)
        {
            return AppProvider()->GetXamlType(type);
        }

        IXamlType GetXamlType(::winrt::hstring const& fullName)
        {
            return AppProvider()->GetXamlType(fullName);
        }

        ::winrt::com_array<::winrt::Microsoft::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions()
        {
            return AppProvider()->GetXmlnsDefinitions();
        }

    private:
        bool _contentLoaded{false};
        winrt::com_ptr<XamlMetaDataProvider> _appProvider;
        winrt::com_ptr<XamlMetaDataProvider> AppProvider()
        {
            if (!_appProvider)
            {
                _appProvider = winrt::make_self<XamlMetaDataProvider>();
            }
            return _appProvider;
        }
    };
}

#undef XAML_EXPORT

