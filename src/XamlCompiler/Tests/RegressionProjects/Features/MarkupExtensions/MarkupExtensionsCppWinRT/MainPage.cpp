// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace MarkupExtensionsComponents;

namespace winrt::MarkupExtensionsCppWinRT::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
        auto extension = FakeMarkupExtension((Microsoft::UI::Xaml::Markup::IXamlMetadataProvider)Application::Current().as<Microsoft::UI::Xaml::Markup::IXamlMetadataProvider>());
    }

    int32_t MainPage::Dummy()
    {
        throw hresult_not_implemented();
    }

    void MainPage::Dummy(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
