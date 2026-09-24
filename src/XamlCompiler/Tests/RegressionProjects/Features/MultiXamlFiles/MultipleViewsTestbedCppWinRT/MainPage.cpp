#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace ::Windows::Foundation;

namespace winrt::MultipleViewsTestbedCppWinRT::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();

        auto weakThis = ::winrt::make_weak<class_type>(*this);
        Window::Current().SizeChanged([weakThis](::winrt::Windows::Foundation::IInspectable const& p0, WindowSizeChangedEventArgs const& p1) {
            if (auto t = weakThis.get())
            {
                ::winrt::get_self<MainPage>(t)->WindowSizeChanged(p0, p1);
            }
        });

        this->LoadCorrectXamlFile();
    }

    int32_t MainPage::Dummy()
    {
        throw hresult_not_implemented();
    }

    void MainPage::Dummy(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainPage::LoadCorrectXamlFile()
    {
        hstring correctFilename = (Window::Current().Bounds().Width > Window::Current().Bounds().Height) ? L"MainPage.xaml" : L"MainPage.Portrait.xaml";

        if (correctFilename == this->filename)
        {
            return;
        }

        // Clear the results of the last file - markup compiler generated code will eventually do this part
        this->filename = correctFilename;
        this->_contentLoaded = false;
        this->Resources(nullptr);

        // Load the new file.
        InitializeComponent(Uri(L"ms-appx:///" + this->filename));

        this->DisplayText().Text(L"MyControl is a " + get_class_name(MyControl()));
    }

    void MainPage::MyControl_Click(IInspectable const&, RoutedEventArgs const&)
    {
        DisplayText().Text(DisplayText().Text() + hstring(L" (Clicked)"));
    }

    void MainPage::MyControl_Checked(IInspectable const&, RoutedEventArgs const&)
    {
        DisplayText().Text(DisplayText().Text() + hstring(L" (Checked)"));
    }

    void MainPage::WindowSizeChanged(IInspectable const&, WindowSizeChangedEventArgs const&)
    {
        LoadCorrectXamlFile();
    }
}
