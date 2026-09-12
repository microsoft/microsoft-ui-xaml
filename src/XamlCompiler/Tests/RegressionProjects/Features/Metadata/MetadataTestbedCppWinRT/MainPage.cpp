// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the MainPage class.
//
// Repeatedly resolves a type and member through the generated IXamlMetadataProvider, matching
// the public API exercised by MetadataTestbedCS.
//

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Markup;

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    Windows::Foundation::IInspectable MainPage::TestProperty()
    {
        return nullptr;
    }

    void MainPage::GetTypeMemberManyTimesClicked(IInspectable const& /* sender */, RoutedEventArgs const& /* e */)
    {
        GetTypeMemberButton().IsEnabled(false);

        for (int i = 0; i < 50000; i++)
        {
            m_provider = Application::Current().try_as<IXamlMetadataProvider>();
            GetTypeMemberTest();
        }

        GetTypeMemberButton().IsEnabled(true);
    }

    void MainPage::GetTypeMemberTest()
    {
        auto type = m_provider.GetXamlType(hstring(L"MetadataTestbedCppWinRT.MainPage"));
        if (type == nullptr)
        {
            throw hresult_error(E_FAIL, L"Test failed");
        }

        auto member = type.GetMember(hstring(L"TestProperty"));
        if (member == nullptr)
        {
            throw hresult_error(E_FAIL, L"Test failed");
        }
    }
}
