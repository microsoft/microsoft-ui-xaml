// WARNING: Please don't edit this file...

void* winrt_make_EventHandling_968976_MyUserControl()
{
    return winrt::detach_abi(winrt::make<winrt::EventHandling_968976::factory_implementation::MyUserControl>());
}
WINRT_EXPORT namespace winrt::EventHandling_968976
{
    MyUserControl::MyUserControl() :
        MyUserControl(make<EventHandling_968976::implementation::MyUserControl>())
    {
    }
}
