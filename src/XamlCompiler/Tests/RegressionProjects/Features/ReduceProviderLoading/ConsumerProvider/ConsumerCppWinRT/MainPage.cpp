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
            auto app = Application::Current().as<ConsumerCppWinRT::App>();
            winrt::get_self<implementation::App>(app)->AddOtherProvider(provider);
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
