// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::MainPage>());
}
namespace winrt::BindTestbed
{
    MainPage::MainPage() :
        MainPage(make<BindTestbed::implementation::MainPage>())
    {
    }
}
