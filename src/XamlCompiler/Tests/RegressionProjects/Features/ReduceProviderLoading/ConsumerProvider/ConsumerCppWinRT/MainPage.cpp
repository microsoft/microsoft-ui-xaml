// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the MainPage class.
//
// Because the project sets XamlCodeGenerationControlFlags=DoNotGenerateOtherProviders,
// the type info for the referenced ProviderCppWinRT component is not registered with the
// app's metadata provider until AddOtherProvider() is called explicitly.
//

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "App.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Markup;

namespace winrt::ConsumerCppWinRT::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    void MainPage::Button_Click(IInspectable const& /* sender */, RoutedEventArgs const& /* e */)
    {
        auto applicationProvider = Application::Current().try_as<IXamlMetadataProvider>();
        auto type = applicationProvider.GetXamlType(hstring(L"ProviderCppWinRT.MainPage"));
        if (!m_otherProviderLoaded && type != nullptr)
        {
            throw hresult_error(E_FAIL);
        }

        if (!m_otherProviderLoaded)
        {
            IXamlMetadataProvider provider = ProviderCppWinRT::XamlMetaDataProvider();
            // App is not a projected type - App.idl declares an empty namespace, as it does in
            // every C++/WinRT app project here. The generated AppT<D> base does implement
            // IXamlMetadataProvider directly, though, so the provider interface already handed
            // back by Application::Current() is a valid entry point to the implementation.
            winrt::get_self<implementation::App>(applicationProvider)->AddOtherProvider(provider);
            m_otherProviderLoaded = true;
        }

        // Retry the operation
        type = applicationProvider.GetXamlType(hstring(L"ProviderCppWinRT.MainPage"));
        if (type == nullptr)
        {
            throw hresult_error(E_FAIL);
        }

        ProviderCppWinRT::MainPage::DoSomething();
        textBlock1().Text(ProviderCppWinRT::MainPage::GetTextToShow());
    }
}
