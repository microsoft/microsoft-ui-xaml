// WARNING: Please don't edit this file...

void* winrt_make_Simple_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::MainPage>());
}
namespace winrt::Simple
{
    MainPage::MainPage() :
        MainPage(make<Simple::implementation::MainPage>())
    {
    }
}
