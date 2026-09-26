// WARNING: Please don't edit this file...

void* winrt_make_EventHandling_968976_MainPage()
{
    return winrt::detach_abi(winrt::make<winrt::EventHandling_968976::factory_implementation::MainPage>());
}
WINRT_EXPORT namespace winrt::EventHandling_968976
{
    MainPage::MainPage() :
        MainPage(make<EventHandling_968976::implementation::MainPage>())
    {
    }
}
